package io.tinga.braid.etlgadget;

import com.fasterxml.jackson.annotation.JsonProperty;

public class EtlGadgetCommand {

    public static final String NAME_OPT = "n";
    public static final String TOPIC_FILTER_OPT = "t";
    
    @JsonProperty(NAME_OPT)
    private String name;
    
    @JsonProperty(TOPIC_FILTER_OPT)
    private String topicFilter;

    public EtlGadgetCommand() {}

    public EtlGadgetCommand(String name, String topicFilter) {
        this.name = name;
        this.topicFilter = topicFilter;
    }

    public String name() {
        return this.name;
    };
    public String topicFilter() {
        return this.topicFilter;
    };
}
