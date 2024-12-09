package io.tinga.braid.etlgadget.dbloader;

public class DbSqlQueries {
    public final static String CREATE_SHADOW_TABLE_QUERY = """
            CREATE TABLE IF NOT EXISTS "shadow_datalake" (
                time     timestamp with time zone default now() not null,
                agent_id text                                   not null,
                topic    text                                   not null,
                shadow   jsonb                                  not null
            );
            SELECT create_hypertable('shadow_datalake', by_range('time'), if_not_exists := true);
            """;

    public final static String INSERT_SHADOW_QUERY = """
            INSERT INTO "shadow_datalake" (time, agent_id, topic, shadow) VALUES (?, ?, ?, ?::jsonb)
            """;
}
