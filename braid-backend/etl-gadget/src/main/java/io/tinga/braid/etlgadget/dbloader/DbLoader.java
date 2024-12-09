package io.tinga.braid.etlgadget.dbloader;

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.SQLException;
import java.sql.Timestamp;
import java.util.List;

import io.tinga.braid.etlgadget.etl.EtlException;
import io.tinga.braid.etlgadget.etl.Loader;
import io.tinga.braid.etlgadget.etl.ShadowRecord;
import jakarta.inject.Inject;

public class DbLoader implements Loader<List<ShadowRecord>> {
    private DbConnectionManager connectionManager;

    @Inject
    public DbLoader(DbConnectionManager connectionManager) {
        this.connectionManager = connectionManager;
    }

    @Override
    public void load(List<ShadowRecord> data) throws EtlException {
        try {
            Connection db = this.connectionManager.getConnection();
            db.setAutoCommit(false);

            PreparedStatement insertStmt = db.prepareStatement(DbSqlQueries.INSERT_SHADOW_QUERY);
            for (ShadowRecord record : data) {
                insertStmt.setTimestamp(1, new Timestamp(record.timeMs()));
                insertStmt.setString(2, record.agentId());
                insertStmt.setString(3, record.topic());
                insertStmt.setObject(4, record.shadow().toString());
                insertStmt.addBatch();
            }

            insertStmt.executeBatch();
            db.commit();
            insertStmt.close();
        } catch (SQLException e) {
            throw new EtlException(e.getMessage());
        }
    }

}
