-- rotate_table is a prototype to test what is required to safely load the data of an
-- existing table into a table with a different schema. For now it is pretty hard coded to
-- rotate into a separate access method.
-- It is unlikely this UDF will become part of Citus, more likely to be added
-- functionality in pg_partman.
CREATE OR REPLACE FUNCTION rotate_table(
    tableam text
)

    RETURNS boolean
    LANGUAGE plpgsql
AS $$
DECLARE

    v_parent_table_name     text;
    v_table_name            text;
    v_rotating_table_name   text;
    v_sql                   text;

BEGIN

    v_parent_table_name := 'github_events';
    v_table_name := 'github_events_2016';
    v_rotating_table_name := v_table_name || '_rotating';

    -- create new table to rotate data into
    v_sql := format('CREATE TABLE %I (LIKE %I) USING %I', v_rotating_table_name, v_table_name, tableam);
    EXECUTE v_sql;

    -- copy all existing data
    v_sql := format('INSERT INTO %I SELECT * FROM %I', v_rotating_table_name, v_table_name);
    EXECUTE v_sql;

    -- deattach old partition, reads will be blocked from here
    v_sql := format('ALTER TABLE %I DETACH PARTITION %I', v_parent_table_name, v_table_name);
    EXECUTE v_sql;

    -- attach new partition
    v_sql := format('ALTER TABLE %I ATTACH PARTITION %I FOR VALUES FROM (''2016-01-01'') TO (''2016-12-31'')', v_parent_table_name, v_rotating_table_name);
    EXECUTE v_sql;

    -- drop old partition
    v_sql := format('DROP TABLE %I', v_table_name);
    EXECUTE v_sql;

    -- rename rotating tablename to old table name
    v_sql := format('ALTER TABLE %I RENAME TO %I', v_rotating_table_name, v_table_name);
    EXECUTE v_sql;

    RETURN true;

END
$$;
