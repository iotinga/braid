package io.tinga.braid.etlgadget.etl;

public class BasicExtractor implements Extractor<ShadowEvent, ShadowEvent> {

    @Override
    public ShadowEvent extractFromEvent(ShadowEvent data) throws EtlException {
        return data;
    }

}
