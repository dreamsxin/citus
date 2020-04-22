/*-------------------------------------------------------------------------
 *
 * master_modify_multiple_shards.c
 *	  UDF to run multi shard update/delete queries
 *
 * This file contains master_modify_multiple_shards function, which takes a update
 * or delete query and runs it worker shards of the distributed table. The distributed
 * modify operation can be done within a distributed transaction and committed in
 * one-phase or two-phase fashion, depending on the citus.multi_shard_commit_protocol
 * setting.
 *
 * Copyright (c) Citus Data, Inc.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "distributed/pg_version_constants.h"

#include "funcapi.h"
#include "libpq-fe.h"
#include "miscadmin.h"


#include "catalog/pg_class.h"
#include "commands/dbcommands.h"
#include "commands/event_trigger.h"
#include "distributed/utils/citus_clauses.h"
#include "distributed/deparser/citus_ruleutils.h"
#include "distributed/commands.h"
#include "distributed/utils/listutils.h"
#include "distributed/master/master_metadata_utility.h"
#include "distributed/master_protocol.h"
#include "distributed/metadata/metadata_cache.h"
#include "distributed/metadata/metadata_sync.h"
#include "distributed/executor/multi_client_executor.h"
#include "distributed/executor/multi_executor.h"
#include "distributed/planner/multi_physical_planner.h"
#include "distributed/executor/multi_server_executor.h"
#include "distributed/planner/distributed_planner.h"
#include "distributed/pg_dist_shard.h"
#include "distributed/pg_dist_partition.h"
#include "distributed/utils/resource_lock.h"
#include "distributed/utils/shardinterval_utils.h"
#include "distributed/planner/shard_pruning.h"
#include "distributed/version_compat.h"
#include "distributed/worker_protocol.h"
#include "distributed/transaction/worker_transaction.h"
#include "optimizer/clauses.h"
#if PG_VERSION_NUM >= PG_VERSION_12
#include "optimizer/optimizer.h"
#else
#include "optimizer/predtest.h"
#include "optimizer/var.h"
#endif
#include "optimizer/restrictinfo.h"
#include "nodes/makefuncs.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"


PG_FUNCTION_INFO_V1(master_modify_multiple_shards);


/*
 * master_modify_multiple_shards takes in a DELETE or UPDATE query string and
 * executes it. This is mainly provided for backwards compatibility, users
 * should use regular UPDATE and DELETE commands.
 */
Datum
master_modify_multiple_shards(PG_FUNCTION_ARGS)
{
	text *queryText = PG_GETARG_TEXT_P(0);
	char *queryString = text_to_cstring(queryText);
	RawStmt *rawStmt = (RawStmt *) ParseTreeRawStmt(queryString);
	Node *queryTreeNode = rawStmt->stmt;

	CheckCitusVersion(ERROR);

	if (!IsA(queryTreeNode, DeleteStmt) && !IsA(queryTreeNode, UpdateStmt))
	{
		ereport(ERROR, (errmsg("query \"%s\" is not a delete or update "
							   "statement", ApplyLogRedaction(queryString))));
	}

	ereport(WARNING, (errmsg("master_modify_multiple_shards is deprecated and will be "
							 "removed in a future release."),
					  errhint("Run the command directly")));

	ExecuteQueryStringIntoDestReceiver(queryString, NULL, None_Receiver);

	PG_RETURN_INT32(0);
}
