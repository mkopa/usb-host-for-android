package info.marcin.usbhost.transport;

import java.util.Objects;

/** Immutable USB control setup packet fields. Transfer data and timeout are supplied separately. */
public final class UsbControlRequest {
    private final int requestType;
    private final int request;
    private final int value;
    private final int index;
    private final UsbDirection direction;

    public UsbControlRequest(
            int requestType, int request, int value, int index, UsbDirection direction) {
        this.requestType = requireUnsigned(requestType, 0xff, "requestType");
        this.request = requireUnsigned(request, 0xff, "request");
        this.value = requireUnsigned(value, 0xffff, "value");
        this.index = requireUnsigned(index, 0xffff, "index");
        this.direction = Objects.requireNonNull(direction, "direction");
        UsbDirection encodedDirection = (requestType & 0x80) == 0
                ? UsbDirection.OUT : UsbDirection.IN;
        if (direction != encodedDirection) {
            throw new IllegalArgumentException("direction must match requestType bit 7");
        }
    }

    public int getRequestType() {
        return requestType;
    }

    public int getRequest() {
        return request;
    }

    public int getValue() {
        return value;
    }

    public int getIndex() {
        return index;
    }

    public UsbDirection getDirection() {
        return direction;
    }

    @Override
    public boolean equals(Object other) {
        if (this == other) {
            return true;
        }
        if (!(other instanceof UsbControlRequest)) {
            return false;
        }
        UsbControlRequest that = (UsbControlRequest) other;
        return requestType == that.requestType
                && request == that.request
                && value == that.value
                && index == that.index
                && direction == that.direction;
    }

    @Override
    public int hashCode() {
        return Objects.hash(requestType, request, value, index, direction);
    }

    private static int requireUnsigned(int candidate, int maximum, String name) {
        if (candidate < 0 || candidate > maximum) {
            throw new IllegalArgumentException(name + " must be between 0 and " + maximum);
        }
        return candidate;
    }
}
