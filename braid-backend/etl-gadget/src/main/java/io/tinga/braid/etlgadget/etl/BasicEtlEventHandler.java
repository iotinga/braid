package io.tinga.braid.etlgadget.etl;

import java.util.List;

import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.fasterxml.jackson.databind.JsonNode;

import jakarta.inject.Inject;

public class BasicEtlEventHandler implements EtlEventHandler {
    private static final Logger log = LoggerFactory.getLogger(BasicEtlEventHandler.class);

    private Extractor<ShadowEvent, ShadowEvent> extractor;
    private Transformer<ShadowEvent, List<ShadowRecord>> transformer;
    private Loader<List<ShadowRecord>> loader;

    @Inject
    public BasicEtlEventHandler(Extractor<ShadowEvent, ShadowEvent> extractor,
            Transformer<ShadowEvent, List<ShadowRecord>> transformer,
            Loader<List<ShadowRecord>> loader) {
        this.extractor = extractor;
        this.transformer = transformer;
        this.loader = loader;
    }

    @Override
    public String getName() {
        return this.getClass().getName();
    }

    @Override
    public Class<JsonNode> getEventClass() {
        return JsonNode.class;
    }

    @Override
    public boolean handle(String topic, JsonNode event) throws Exception {
        log.debug("Messagge received {}", event);

        try {
            ShadowEvent extractedData = this.extractor.extractFromEvent(new ShadowEvent(topic, this.getName(), event));
            List<ShadowRecord> transformedData = this.transformer.transform(extractedData);
            this.loader.load(transformedData);
        } catch (EtlException e) {
            log.error("Error in process ETL event", e);
        }

        return true;
    }
}
