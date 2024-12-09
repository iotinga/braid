package io.tinga.braid.etlgadget.etl;

public interface Extractor<S, T> {
    T extractFromEvent(S data) throws EtlException;
}
