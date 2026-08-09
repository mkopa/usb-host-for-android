#include <jni.h>

#include <cstdint>
#include <vector>

#include "usbhost/usbhost.h"

namespace {

jlongArray makeLongArray(JNIEnv *env, const std::vector<jlong> &values) {
    jlongArray result = env->NewLongArray(static_cast<jsize>(values.size()));
    if (result) {
        env->SetLongArrayRegion(result, 0, static_cast<jsize>(values.size()), values.data());
    }
    return result;
}

}  // namespace

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_NativeBridge_open(JNIEnv *env, jclass, jint fd, jint vendorId,
                                            jint productId, jint swdFrequencyKhz) {
    usbhost_session session = 0;
    usbhost_programmer_info programmer{};
    programmer.struct_size = sizeof(programmer);
    const usbhost_status status = usbhost_open_stlink_v3_fd(
        fd, static_cast<uint16_t>(vendorId), static_cast<uint16_t>(productId),
        static_cast<uint32_t>(swdFrequencyKhz), &session, &programmer);
    return makeLongArray(env, {status, static_cast<jlong>(session), programmer.stlink_version,
                               programmer.jtag_version, programmer.swim_version,
                               programmer.jtag_api_version});
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_info_marcin_usbhost_NativeBridge_connectTarget(JNIEnv *env, jclass, jlong handle) {
    usbhost_target_info target{};
    target.struct_size = sizeof(target);
    const usbhost_status status = usbhost_connect_target(
        static_cast<usbhost_session>(handle), &target);
    return makeLongArray(env, {status, target.chip_id, target.flash_base, target.flash_size,
                               target.flash_page_size, target.sram_base, target.sram_size,
                               target.target_voltage_mv});
}

extern "C" JNIEXPORT jbyteArray JNICALL
Java_info_marcin_usbhost_NativeBridge_readMemory(JNIEnv *env, jclass, jlong handle,
                                                  jlong address, jint length) {
    if (length <= 0) {
        return nullptr;
    }
    std::vector<uint8_t> bytes(static_cast<size_t>(length));
    const usbhost_status status = usbhost_read_memory(
        static_cast<usbhost_session>(handle), static_cast<uint32_t>(address),
        bytes.data(), static_cast<uint32_t>(length));
    if (status != USBHOST_OK) {
        return nullptr;
    }
    jbyteArray result = env->NewByteArray(length);
    if (result) {
        env->SetByteArrayRegion(result, 0, length,
                                reinterpret_cast<const jbyte *>(bytes.data()));
    }
    return result;
}

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_NativeBridge_close(JNIEnv *, jclass, jlong handle) {
    return usbhost_close(static_cast<usbhost_session>(handle));
}

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_NativeBridge_lastStatus(JNIEnv *, jclass) {
    return usbhost_last_status();
}

extern "C" JNIEXPORT jstring JNICALL
Java_info_marcin_usbhost_NativeBridge_lastError(JNIEnv *env, jclass) {
    return env->NewStringUTF(usbhost_last_error());
}
