package io.tinga.braid.etlgadget.dbloader;

public interface DbConfig {

    public static final String ETL_DB_USER = "USER";
    public static final String ETL_DB_PASSWORD = "PASSWORD";
    public static final String ETL_DB_NAME = "NAME";
    public static final String ETL_DB_HOSTNAME = "HOSTNAME";
    public static final String ETL_DB_PORT = "PORT";

    String getDbUrl();
    String hostname();
    int port();
    String dbName();
    String dbUser();
    String dbPassword();
}
