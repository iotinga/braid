package io.tinga.braid.etlgadget;

import io.tinga.belt.input.GadgetCommandOption;

public enum EtlGadgetCommandOption implements GadgetCommandOption {

    NAME(EtlGadgetCommand.NAME_OPT, true, String.class, "ENTITY", "The name of the command"),

    TOPIC(EtlGadgetCommand.TOPIC_FILTER_OPT, true, String.class, "braid/entity/test/admin",
            "The topic to simulate as source of the desired.");

    private final String opt;
    private final boolean hasArg;
    private final String description;
    private final Class<?> type;
    private final String defaultValue;

    private EtlGadgetCommandOption(String opt, boolean hasArg, Class<?> type, String defaultValue,
            String description) {
        this.opt = opt;
        this.hasArg = hasArg;
        this.type = type;
        this.defaultValue = defaultValue;
        this.description = description;
    }

    @Override
    public String opt() {
        return opt;
    }

    @Override
    public boolean hasArg() {
        return hasArg;
    }

    @Override
    public String description() {
        return description;
    }

    @Override
    public Class<?> type() {
        return type;
    }

    @Override
    public String defaultValue() {
        return defaultValue;
    }
}
