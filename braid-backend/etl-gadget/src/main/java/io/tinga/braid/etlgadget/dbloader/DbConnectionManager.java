package io.tinga.braid.etlgadget.dbloader;

import java.sql.Connection;

public interface DbConnectionManager {
    public Connection getConnection();

    public void close();
}