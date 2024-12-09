package io.tinga.braid.protocol;

import com.fasterxml.jackson.annotation.JsonProperty;

import io.tinga.belt.output.Status;

public class RawMessage<B> {
    @JsonProperty("TS")
    private Long timestamp;

    @JsonProperty("VER")
    private Integer version;

    @JsonProperty("PROT")
    private Integer protocolVersion;

    @JsonProperty("ACTION")
    private Action action;

    @JsonProperty("STATUS")
    private Status status;

    @JsonProperty("BODY")
    private B body;

    public RawMessage() {
    }

    public RawMessage(Long timestamp, Integer version, Integer protocolVersion, Action action, Status status,
            B body) {
        this.timestamp = timestamp;
        this.version = version;
        this.protocolVersion = protocolVersion;
        this.action = action;
        this.status = status;
        this.body = body;
    }

    public RawMessage<B> response(Long timestamp, Status status, Integer version, B body) {
        return new RawMessage<B>(timestamp, version, protocolVersion, action, status, body);
    }

    public Long getTimestamp() {
        return timestamp;
    }

    public Integer getVersion() {
        return version;
    }

    public Integer getProtocolVersion() {
        return protocolVersion;
    }

    public Action getAction() {
        return action;
    }

    public Status getStatus() {
        return status;
    }

    public B getBody() {
        return body;
    }

}