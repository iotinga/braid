package io.tinga.braid.etlgadget;

import java.sql.Connection;
import java.sql.SQLException;
import java.sql.Statement;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonNode;
import com.google.inject.Inject;

import io.tinga.belt.input.GadgetCommandExecutor;
import io.tinga.belt.output.Status;
import io.tinga.braid.etlgadget.dbloader.DbConnectionManager;
import io.tinga.braid.etlgadget.dbloader.DbSqlQueries;
import io.tinga.braid.etlgadget.etl.EtlEventHandler;
import it.netgrid.bauer.ITopicFactory;
import it.netgrid.bauer.Topic;

public class EtlGadgetCommandExecutor implements GadgetCommandExecutor<EtlGadgetCommand> {

    private static final int THREAD_SLEEP_MS = 3000;
    private static final Logger log = LoggerFactory.getLogger(EtlGadgetCommandExecutor.class);

    private final Topic<JsonNode> shadowTopic;
    private final EtlEventHandler etlHandler;
    private final DbConnectionManager connectionManager;

    @Inject
    public EtlGadgetCommandExecutor(GadgetConfig config, EtlEventHandler etlHandler,
            DbConnectionManager connectionManager, ITopicFactory topicFactory) {
        this.etlHandler = etlHandler;
        this.connectionManager = connectionManager;
        this.shadowTopic = topicFactory.getTopic(config.topicFilter());
    }

    private void setUp() throws SQLException {
        Connection db = this.connectionManager.getConnection();
        Statement createStmt = db.createStatement();
        createStmt.execute(DbSqlQueries.CREATE_SHADOW_TABLE_QUERY);
        createStmt.close();
        db.close();
    }

    @Override
    public CompletableFuture<Status> submit(EtlGadgetCommand command) {
        CompletableFuture<Status> retval = new CompletableFuture<>();
        retval.completeAsync(new Supplier<Status>() {
            @Override
            public Status get() {
                try {
                    log.debug("Setting up database for ETL");
                    setUp();
                    shadowTopic.addHandler(etlHandler);

                    log.debug("Added ETL shadow topic handler");
                    while (!Thread.currentThread().isInterrupted()) {
                        try {
                            Thread.sleep(THREAD_SLEEP_MS);
                        } catch (InterruptedException e) {
                            log.info("Interrupt: %s", e.getMessage());
                        }
                    }
                } catch (Exception e) {
                    log.error(e.getLocalizedMessage());
                    return Status.INTERNAL_SERVER_ERROR;
                } finally {
                    log.info("Shutdown");
                }
                return Status.OK;
            }
        });
        return retval;
    }

}
