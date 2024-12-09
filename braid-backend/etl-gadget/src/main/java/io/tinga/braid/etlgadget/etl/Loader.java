package io.tinga.braid.etlgadget.etl;

public interface Loader<T> {
    void load(T data) throws EtlException;
}
