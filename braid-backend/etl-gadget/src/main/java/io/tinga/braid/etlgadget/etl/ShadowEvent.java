package io.tinga.braid.etlgadget.etl;

import com.fasterxml.jackson.databind.JsonNode;

public record ShadowEvent(
                String topic,
                String agentId,
                JsonNode event) {
}
