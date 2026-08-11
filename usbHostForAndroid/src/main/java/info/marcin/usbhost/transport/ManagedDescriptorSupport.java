package info.marcin.usbhost.transport;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Objects;

final class ManagedDescriptorSupport {
    private ManagedDescriptorSupport() {}

    static int unsigned(int value, int maximum, String name) {
        if (value < 0 || value > maximum) {
            throw new IllegalArgumentException(name + " must be between 0 and " + maximum);
        }
        return value;
    }

    static long generation(long value) {
        if (value <= 0) throw new IllegalArgumentException("snapshotGeneration must be positive");
        return value;
    }

    static <T> List<T> immutableList(List<T> values, String name) {
        Objects.requireNonNull(values, name);
        ArrayList<T> copy = new ArrayList<>(values.size());
        for (T value : values) copy.add(Objects.requireNonNull(value, name + " element"));
        return Collections.unmodifiableList(copy);
    }
}
