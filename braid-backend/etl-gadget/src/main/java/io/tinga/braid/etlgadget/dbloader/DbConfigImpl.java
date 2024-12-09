package io.tinga.braid.etlgadget.dbloader;

import com.fasterxml.jackson.annotation.JsonProperty;


public class DbConfigImpl implements DbConfig {

    @JsonProperty(ETL_DB_USER)
    private String dbUser;

    @JsonProperty(ETL_DB_PASSWORD)
    private String dbPassword;

    @JsonProperty(ETL_DB_NAME)
    private String dbName;

    @JsonProperty(ETL_DB_HOSTNAME)
    private String hostname;

    @JsonProperty(ETL_DB_PORT)
    private int port;

    @Override
    public String getDbUrl() {
        return String.format("jdbc:postgresql://%s:%d/%s", this.hostname, this.port, this.dbName);
    }

    @Override
    public String dbUser() {
        return this.dbUser;
    }

    @Override
    public String dbPassword() {
        return this.dbPassword;
    }

    @Override
    public String dbName() {
        return this.dbName;
    }

    @Override
    public String hostname() {
        return this.hostname;
    }

    @Override
    public int port() {
        return this.port;
    }

}
