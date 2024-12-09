package io.tinga.braid.etlgadget;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class GadgetConfigImpl implements GadgetConfig {

    @JsonProperty(GadgetConfig.ETL_TOPIC_FILTER)
    private String topicFilter;

    @Override
    public String topicFilter() {
        return this.topicFilter;
    }

}
