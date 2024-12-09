package io.tinga.braid.etlgadget.etl;

import java.time.Instant;
import java.util.ArrayList;
import java.util.List;

public class BasicTransformer implements Transformer<ShadowEvent, List<ShadowRecord>> {

    @Override
    public List<ShadowRecord> transform(ShadowEvent data) throws EtlException {
        long timestampMs = Instant.now().toEpochMilli();
        List<ShadowRecord> records = new ArrayList<>();
        records.add(new ShadowRecord(timestampMs, data.topic(), data.agentId(), data.event()));
        return records;
    }

}
