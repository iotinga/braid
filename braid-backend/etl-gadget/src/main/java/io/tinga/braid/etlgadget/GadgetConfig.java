package io.tinga.braid.etlgadget;

public interface GadgetConfig {

    public static final String ETL_TOPIC_FILTER = "TOPIC_FILTER";

    public String topicFilter();
}