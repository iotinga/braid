package io.tinga.braid.protocol;

public interface RawMessageValidator {
    public void validate(RawMessage<?> message) throws RawMessageValidationException;
    public void validateUpdate(RawMessage<?> from, RawMessage<?> to) throws RawMessageValidationException;
}
