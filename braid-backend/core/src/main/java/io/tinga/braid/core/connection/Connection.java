package io.tinga.braid.core.connection;

public interface Connection {
    void connect() throws Exception;

    void disconnect() throws Exception;

    ConnectionState getConnectionState();
}
