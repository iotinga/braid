package io.tinga.braid.etlgadget.etl;

import com.fasterxml.jackson.databind.JsonNode;

public record ShadowRecord(
                Long timeMs,
                String topic,
                String agentId,
                JsonNode shadow) {
}
