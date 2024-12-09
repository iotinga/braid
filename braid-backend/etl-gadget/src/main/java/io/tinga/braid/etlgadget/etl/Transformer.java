package io.tinga.braid.etlgadget.etl;

public interface Transformer<S, T> {
    T transform(S data) throws EtlException;
}
