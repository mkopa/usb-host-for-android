#include <jni.h>

#include <array>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "jni/transport_jni_contract.hpp"
#include "usbhost/transport.h"

namespace {

jlongArray longs(JNIEnv *env, const jlong *values, jsize count) {
    jlongArray result = env->NewLongArray(count);
    if (result != nullptr) env->SetLongArrayRegion(result, 0, count, values);
    return result;
}

jlongArray statusOnly(JNIEnv *env, usbhost_status status) {
    const jlong value = status;
    return longs(env, &value, 1);
}

jlongArray transferResult(JNIEnv *env, usbhost::jni::NativeTransferResult result) {
    const std::array<jlong, usbhost::jni::kTransferRecordLength> values{
        result.status, static_cast<jlong>(result.actualLength)};
    return longs(env, values.data(), values.size());
}

bool unsignedByte(jint value) { return value >= 0 && value <= 0xff; }
bool unsignedIndex(jint value) { return value >= 0; }

template <typename Transfer>
jlongArray marshalTransfer(JNIEnv *env, jbyteArray buffer, jint offset, jint length,
                           jint timeoutMilliseconds, bool input,
                           std::uint32_t maximumLength, Transfer &&transfer) {
    if (buffer == nullptr || timeoutMilliseconds < 1 || timeoutMilliseconds > 60000 ||
        length < 0 || static_cast<std::uint64_t>(length) > maximumLength) {
        return transferResult(env, {USBHOST_INVALID_ARGUMENT, 0});
    }
    const jsize capacity = env->GetArrayLength(buffer);
    if (env->ExceptionCheck()) return nullptr;
    usbhost::jni::TransferSlice slice{};
    if (!usbhost::jni::checkedTransferSlice(capacity, offset, length, slice))
        return transferResult(env, {USBHOST_INVALID_ARGUMENT, 0});

    bool javaException = false;
    const auto outcome = usbhost::jni::executeTransferNoexcept([&] {
        std::vector<std::uint8_t> nativeBuffer(slice.length);
        if (!input && slice.length != 0) {
            env->GetByteArrayRegion(buffer, slice.offset, slice.length,
                reinterpret_cast<jbyte *>(nativeBuffer.data()));
            if (env->ExceptionCheck()) {
                javaException = true;
                return usbhost::jni::NativeTransferResult{USBHOST_INTERNAL_ERROR, 0};
            }
        }
        std::uint32_t actualLength = 0;
        const auto status = transfer(nativeBuffer.data(), slice.length,
                                     static_cast<std::uint32_t>(timeoutMilliseconds),
                                     &actualLength);
        usbhost::jni::CompletedInputCopy copy{};
        if (!usbhost::jni::completedInputCopy(slice, actualLength, copy))
            return usbhost::jni::NativeTransferResult{USBHOST_INTERNAL_ERROR, 0};
        if (input && copy.length != 0) {
            env->SetByteArrayRegion(buffer, copy.destinationOffset, copy.length,
                reinterpret_cast<const jbyte *>(nativeBuffer.data()));
            if (env->ExceptionCheck()) {
                javaException = true;
                return usbhost::jni::NativeTransferResult{USBHOST_INTERNAL_ERROR, 0};
            }
        }
        return usbhost::jni::NativeTransferResult{status, actualLength};
    });
    return javaException ? nullptr : transferResult(env, outcome);
}

}  // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_openSession(
        JNIEnv *env, jclass, jint fd) {
    usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
    const usbhost_status status = usbhost_transport_open_fd(fd, &session);
    const std::array<jlong, usbhost::jni::kOpenRecordLength> result{
        status, status == USBHOST_OK ? static_cast<jlong>(session) : 0};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getDeviceDescriptor(
        JNIEnv *env, jclass, jlong session) {
    usbhost_transport_device_descriptor value{};
    value.struct_size = sizeof(value);
    const auto status = usbhost_transport_get_device_descriptor(session, &value);
    if (status != USBHOST_OK) return statusOnly(env, status);
    const std::array<jlong, usbhost::jni::kDeviceRecordLength> result{
        status, static_cast<jlong>(value.snapshot_generation), value.usb_version_bcd,
        value.device_class, value.device_subclass, value.device_protocol,
        value.endpoint_zero_max_packet_size, value.vendor_id, value.product_id,
        value.device_release_bcd, value.configuration_count};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getConfiguration(
        JNIEnv *env, jclass, jlong session, jint configurationIndex) {
    if (!unsignedIndex(configurationIndex)) return statusOnly(env, USBHOST_INVALID_ARGUMENT);
    usbhost_transport_configuration_descriptor value{};
    value.struct_size = sizeof(value);
    const auto status = usbhost_transport_get_configuration_at(
        session, static_cast<std::uint32_t>(configurationIndex), &value);
    if (status != USBHOST_OK) return statusOnly(env, status);
    const std::array<jlong, usbhost::jni::kConfigurationRecordLength> result{
        status, static_cast<jlong>(value.snapshot_generation), value.configuration_index,
        value.configuration_value, value.attributes, value.maximum_power, value.active,
        value.interface_count, value.additional_descriptor_count};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getInterface(
        JNIEnv *env, jclass, jlong session, jint configurationIndex, jint interfaceIndex) {
    if (!unsignedIndex(configurationIndex) || !unsignedIndex(interfaceIndex))
        return statusOnly(env, USBHOST_INVALID_ARGUMENT);
    usbhost_transport_interface_descriptor value{};
    value.struct_size = sizeof(value);
    const auto status = usbhost_transport_get_interface_at(session, configurationIndex,
                                                            interfaceIndex, &value);
    if (status != USBHOST_OK) return statusOnly(env, status);
    const std::array<jlong, usbhost::jni::kInterfaceRecordLength> result{
        status, static_cast<jlong>(value.snapshot_generation), value.interface_index,
        value.interface_number, value.active_alternate_setting, value.claimed,
        value.alternate_setting_count};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getAlternateSetting(
        JNIEnv *env, jclass, jlong session, jint configurationIndex,
        jint interfaceIndex, jint alternateIndex) {
    if (!unsignedIndex(configurationIndex) || !unsignedIndex(interfaceIndex)
            || !unsignedIndex(alternateIndex)) return statusOnly(env, USBHOST_INVALID_ARGUMENT);
    usbhost_transport_alternate_setting_descriptor value{};
    value.struct_size = sizeof(value);
    const auto status = usbhost_transport_get_alternate_setting_at(
        session, configurationIndex, interfaceIndex, alternateIndex, &value);
    if (status != USBHOST_OK) return statusOnly(env, status);
    const std::array<jlong, usbhost::jni::kAlternateRecordLength> result{
        status, static_cast<jlong>(value.snapshot_generation), value.alternate_setting_index,
        value.interface_number, value.alternate_setting, value.interface_class,
        value.interface_subclass, value.interface_protocol, value.endpoint_count,
        value.additional_descriptor_count};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getEndpoint(
        JNIEnv *env, jclass, jlong session, jint configurationIndex,
        jint interfaceIndex, jint alternateIndex, jint endpointIndex) {
    if (!unsignedIndex(configurationIndex) || !unsignedIndex(interfaceIndex)
            || !unsignedIndex(alternateIndex) || !unsignedIndex(endpointIndex))
        return statusOnly(env, USBHOST_INVALID_ARGUMENT);
    usbhost_transport_endpoint_descriptor value{};
    value.struct_size = sizeof(value);
    const auto status = usbhost_transport_get_endpoint_at(
        session, configurationIndex, interfaceIndex, alternateIndex, endpointIndex, &value);
    if (status != USBHOST_OK) return statusOnly(env, status);
    const std::array<jlong, usbhost::jni::kEndpointRecordLength> result{
        status, static_cast<jlong>(value.snapshot_generation), value.endpoint_index,
        value.endpoint_address, value.endpoint_number, value.direction, value.transfer_type,
        value.maximum_packet_size, value.interval, value.additional_descriptor_count};
    return longs(env, result.data(), result.size());
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_getAdditionalDescriptor(
        JNIEnv *env, jclass, jlong session, jint scope, jlong generation,
        jint configurationIndex, jint interfaceIndex, jint alternateIndex,
        jint endpointIndex, jint additionalIndex) {
    if (scope < USBHOST_TRANSPORT_DESCRIPTOR_CONFIGURATION
            || scope > USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT || generation <= 0
            || !unsignedIndex(configurationIndex) || !unsignedIndex(interfaceIndex)
            || !unsignedIndex(alternateIndex) || !unsignedIndex(endpointIndex)
            || !unsignedIndex(additionalIndex)) {
        const jbyte status = USBHOST_INVALID_ARGUMENT;
        jbyteArray result = env->NewByteArray(1);
        if (result != nullptr) env->SetByteArrayRegion(result, 0, 1, &status);
        return result;
    }
    usbhost_transport_descriptor_location location{};
    location.struct_size = sizeof(location);
    location.scope = scope;
    location.snapshot_generation = generation;
    location.configuration_index = configurationIndex;
    location.interface_index = interfaceIndex;
    location.alternate_setting_index = alternateIndex;
    location.endpoint_index = endpointIndex;
    location.additional_descriptor_index = additionalIndex;
    std::vector<std::uint8_t> bytes(USBHOST_TRANSPORT_MAX_ADDITIONAL_DESCRIPTOR_LENGTH);
    std::uint8_t type = 0;
    std::uint32_t actual = 0;
    const auto status = usbhost_transport_get_additional_descriptor_at(
        session, &location, bytes.data(), bytes.size(), &type, &actual);
    const jsize size = status == USBHOST_OK ? static_cast<jsize>(actual + 2) : 1;
    jbyteArray result = env->NewByteArray(size);
    if (result == nullptr) return nullptr;
    const jbyte statusByte = static_cast<jbyte>(status);
    env->SetByteArrayRegion(result, 0, 1, &statusByte);
    if (status == USBHOST_OK) {
        const jbyte typeByte = static_cast<jbyte>(type);
        env->SetByteArrayRegion(result, 1, 1, &typeByte);
        if (actual != 0) env->SetByteArrayRegion(
            result, 2, actual, reinterpret_cast<const jbyte *>(bytes.data()));
    }
    return result;
}

#define USBHOST_JNI_BYTE_OPERATION(name, function) \
extern "C" JNIEXPORT jint JNICALL \
Java_info_marcin_usbhost_transport_TransportNativeBridge_##name( \
        JNIEnv *, jclass, jlong session, jint value) { \
    return unsignedByte(value) ? function(session, static_cast<std::uint8_t>(value)) \
                               : USBHOST_INVALID_ARGUMENT; \
}

USBHOST_JNI_BYTE_OPERATION(selectConfiguration, usbhost_transport_select_configuration)
USBHOST_JNI_BYTE_OPERATION(claimInterface, usbhost_transport_claim_interface)
USBHOST_JNI_BYTE_OPERATION(releaseInterface, usbhost_transport_release_interface)

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_selectAlternateSetting(
        JNIEnv *, jclass, jlong session, jint interfaceNumber, jint alternateSetting) {
    return unsignedByte(interfaceNumber) && unsignedByte(alternateSetting)
        ? usbhost_transport_select_alternate_setting(session, interfaceNumber, alternateSetting)
        : USBHOST_INVALID_ARGUMENT;
}

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_cancel(
        JNIEnv *, jclass, jlong session) { return usbhost_transport_cancel(session); }

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_close(
        JNIEnv *, jclass, jlong session) { return usbhost_transport_close(session); }

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_controlTransfer(
        JNIEnv *env, jclass, jlong session, jint requestType, jint request,
        jint value, jint index, jbyteArray buffer, jint offset, jint length,
        jint timeoutMilliseconds) {
    if (!unsignedByte(requestType) || !unsignedByte(request) || value < 0 || value > 0xffff ||
        index < 0 || index > 0xffff) {
        return transferResult(env, {USBHOST_INVALID_ARGUMENT, 0});
    }
    const bool input = (requestType & 0x80) != 0;
    return marshalTransfer(env, buffer, offset, length, timeoutMilliseconds, input, 0xffff,
        [=](std::uint8_t *bytes, std::uint32_t size, std::uint32_t timeout,
            std::uint32_t *actual) {
            return usbhost_transport_control_transfer(
                session, requestType, request, value, index, bytes, size, timeout, actual);
        });
}

template <typename Transfer>
jlongArray endpointTransfer(JNIEnv *env, jlong session, jint endpointAddress,
        jbyteArray buffer, jint offset, jint length, jint timeoutMilliseconds,
        Transfer &&transfer) {
    if (!unsignedByte(endpointAddress))
        return transferResult(env, {USBHOST_INVALID_ARGUMENT, 0});
    const bool input = (endpointAddress & 0x80) != 0;
    return marshalTransfer(env, buffer, offset, length, timeoutMilliseconds, input,
        USBHOST_MAX_READ_SIZE,
        [=](std::uint8_t *bytes, std::uint32_t size, std::uint32_t timeout,
            std::uint32_t *actual) {
            return transfer(session, static_cast<std::uint8_t>(endpointAddress),
                            bytes, size, timeout, actual);
        });
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_bulkTransfer(
        JNIEnv *env, jclass, jlong session, jint endpointAddress,
        jbyteArray buffer, jint offset, jint length, jint timeoutMilliseconds) {
    return endpointTransfer(env, session, endpointAddress, buffer, offset, length,
                            timeoutMilliseconds, usbhost_transport_bulk_transfer);
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_interruptTransfer(
        JNIEnv *env, jclass, jlong session, jint endpointAddress,
        jbyteArray buffer, jint offset, jint length, jint timeoutMilliseconds) {
    return endpointTransfer(env, session, endpointAddress, buffer, offset, length,
                            timeoutMilliseconds, usbhost_transport_interrupt_transfer);
}

extern "C" JNIEXPORT jstring JNICALL
Java_info_marcin_usbhost_transport_TransportNativeBridge_lastError(JNIEnv *env, jclass) {
    return env->NewStringUTF(usbhost_last_error());
}
