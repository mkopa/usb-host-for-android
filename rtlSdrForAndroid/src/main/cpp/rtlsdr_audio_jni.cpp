/*
 * RTL2832U/R82xx FM example adapter.
 *
 * The R82xx driver is compiled from the official rtl-sdr submodule and remains licensed under
 * GPL-2.0-or-later. This adapter is part of the example application, not the MIT SDK artifact.
 */

#include <jni.h>
#include <android/log.h>
#include <dlfcn.h>

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <usbhost/transport.h>

extern "C" {
#include "tuner_r82xx.h"
}

namespace {

constexpr const char *kLogTag = "RtlSdrAudio";
constexpr uint8_t kControlIn = 0xc0;
constexpr uint8_t kControlOut = 0x40;
constexpr uint32_t kControlTimeoutMs = 500;
constexpr uint32_t kBulkTimeoutMs = 500;
constexpr uint32_t kRtlXtalHz = 28'800'000;
constexpr uint8_t kUsbBlock = 1;
constexpr uint8_t kSystemBlock = 2;
constexpr uint8_t kI2cBlock = 6;
constexpr uint16_t kUsbSystemControl = 0x2000;
constexpr uint16_t kUsbEndpointControl = 0x2148;
constexpr uint16_t kUsbEndpointMaximumPacket = 0x2158;
constexpr uint16_t kDemodControl = 0x3000;
constexpr uint16_t kDemodControl1 = 0x300b;
constexpr uint8_t kBulkEndpointIn = 0x81;

constexpr int kFir[16] = {
    -54, -36, -41, -40, -32, -14, 14, 53,
    101, 156, 215, 273, 327, 372, 404, 421,
};

struct RtlDevice {
    usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
    r82xx_config tuner_config{};
    r82xx_priv tuner{};
    uint32_t sample_rate = 0;
    uint32_t frequency = 0;
    bool interface_claimed = false;
    bool tuner_initialized = false;
    const char *tuner_name = "unknown";
};

struct UsbApi {
    void *library = nullptr;
    usbhost_status (*open_fd)(int, usbhost_transport_session *) = nullptr;
    usbhost_status (*claim_interface)(usbhost_transport_session, uint8_t) = nullptr;
    usbhost_status (*release_interface)(usbhost_transport_session, uint8_t) = nullptr;
    usbhost_status (*close)(usbhost_transport_session) = nullptr;
    usbhost_status (*control_transfer)(usbhost_transport_session, uint8_t, uint8_t,
                                       uint16_t, uint16_t, uint8_t *, uint32_t,
                                       uint32_t, uint32_t *) = nullptr;
    usbhost_status (*bulk_transfer)(usbhost_transport_session, uint8_t, uint8_t *,
                                    uint32_t, uint32_t, uint32_t *) = nullptr;
    const char *(*status_name)(usbhost_status) = nullptr;
};

UsbApi g_usb;

bool load_usb_api() {
    if (g_usb.library != nullptr) return true;
    void *library = dlopen("libusbhost.so", RTLD_NOW | RTLD_LOCAL);
    if (library == nullptr) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "dlopen libusbhost.so failed: %s", dlerror());
        return false;
    }
#define LOAD_USB_SYMBOL(field, symbol) \
    g_usb.field = reinterpret_cast<decltype(g_usb.field)>(dlsym(library, symbol)); \
    if (g_usb.field == nullptr) { dlclose(library); return false; }
    LOAD_USB_SYMBOL(open_fd, "usbhost_transport_open_fd")
    LOAD_USB_SYMBOL(claim_interface, "usbhost_transport_claim_interface")
    LOAD_USB_SYMBOL(release_interface, "usbhost_transport_release_interface")
    LOAD_USB_SYMBOL(close, "usbhost_transport_close")
    LOAD_USB_SYMBOL(control_transfer, "usbhost_transport_control_transfer")
    LOAD_USB_SYMBOL(bulk_transfer, "usbhost_transport_bulk_transfer")
    LOAD_USB_SYMBOL(status_name, "usbhost_status_name")
#undef LOAD_USB_SYMBOL
    g_usb.library = library;
    return true;
}

void throw_java(JNIEnv *env, const char *message) {
    __android_log_print(ANDROID_LOG_ERROR, kLogTag, "%s", message);
    jclass type = env->FindClass("java/lang/IllegalStateException");
    env->ThrowNew(type, message);
}

bool control(RtlDevice *device, uint8_t request_type, uint16_t value, uint16_t index,
             uint8_t *data, uint32_t length) {
    uint32_t actual = 0;
    const usbhost_status status = g_usb.control_transfer(
        device->session, request_type, 0, value, index, data, length,
        kControlTimeoutMs, &actual);
    if (status != USBHOST_OK || actual != length) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag,
                            "control failed status=%s actual=%u expected=%u",
                            g_usb.status_name(status), actual, length);
        return false;
    }
    return true;
}

int write_array(RtlDevice *device, uint8_t block, uint16_t address,
                uint8_t *data, uint8_t length) {
    const uint16_t index = static_cast<uint16_t>((block << 8) | 0x10);
    return control(device, kControlOut, address, index, data, length) ? length : -1;
}

int read_array(RtlDevice *device, uint8_t block, uint16_t address,
               uint8_t *data, uint8_t length) {
    const uint16_t index = static_cast<uint16_t>(block << 8);
    return control(device, kControlIn, address, index, data, length) ? length : -1;
}

bool write_register(RtlDevice *device, uint8_t block, uint16_t address,
                    uint16_t value, uint8_t length) {
    uint8_t data[2]{};
    data[0] = length == 1 ? static_cast<uint8_t>(value)
                          : static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value);
    return write_array(device, block, address, data, length) == length;
}

bool demod_read(RtlDevice *device, uint8_t page, uint16_t address,
                uint8_t *data, uint8_t length) {
    const uint16_t value = static_cast<uint16_t>((address << 8) | 0x20);
    return control(device, kControlIn, value, page, data, length);
}

bool demod_write(RtlDevice *device, uint8_t page, uint16_t address,
                 uint16_t value, uint8_t length) {
    uint8_t data[2]{};
    data[0] = length == 1 ? static_cast<uint8_t>(value)
                          : static_cast<uint8_t>(value >> 8);
    data[1] = static_cast<uint8_t>(value);
    const uint16_t request_value = static_cast<uint16_t>((address << 8) | 0x20);
    if (!control(device, kControlOut, request_value,
                 static_cast<uint16_t>(0x10 | page), data, length)) {
        return false;
    }
    uint8_t flush = 0;
    return demod_read(device, 0x0a, 0x01, &flush, 1);
}

bool set_fir(RtlDevice *device) {
    uint8_t fir[20]{};
    for (int i = 0; i < 8; ++i) fir[i] = static_cast<uint8_t>(kFir[i]);
    for (int i = 0; i < 8; i += 2) {
        const int value0 = kFir[8 + i];
        const int value1 = kFir[8 + i + 1];
        fir[8 + i * 3 / 2] = static_cast<uint8_t>(value0 >> 4);
        fir[8 + i * 3 / 2 + 1] = static_cast<uint8_t>(
            (value0 << 4) | ((value1 >> 8) & 0x0f));
        fir[8 + i * 3 / 2 + 2] = static_cast<uint8_t>(value1);
    }
    for (int i = 0; i < 20; ++i) {
        if (!demod_write(device, 1, static_cast<uint16_t>(0x1c + i), fir[i], 1)) {
            return false;
        }
    }
    return true;
}

bool initialize_baseband(RtlDevice *device) {
    if (!write_register(device, kUsbBlock, kUsbSystemControl, 0x09, 1) ||
        !write_register(device, kUsbBlock, kUsbEndpointMaximumPacket, 0x0002, 2) ||
        !write_register(device, kUsbBlock, kUsbEndpointControl, 0x1002, 2) ||
        !write_register(device, kSystemBlock, kDemodControl1, 0x22, 1) ||
        !write_register(device, kSystemBlock, kDemodControl, 0xe8, 1)) {
        return false;
    }
    if (!demod_write(device, 1, 0x01, 0x14, 1) ||
        !demod_write(device, 1, 0x01, 0x10, 1) ||
        !demod_write(device, 1, 0x15, 0x00, 1) ||
        !demod_write(device, 1, 0x16, 0x0000, 2)) {
        return false;
    }
    for (uint16_t address = 0x16; address < 0x1c; ++address) {
        if (!demod_write(device, 1, address, 0, 1)) return false;
    }
    return set_fir(device) &&
        demod_write(device, 0, 0x19, 0x05, 1) &&
        demod_write(device, 1, 0x93, 0xf0, 1) &&
        demod_write(device, 1, 0x94, 0x0f, 1) &&
        demod_write(device, 1, 0x11, 0x00, 1) &&
        demod_write(device, 1, 0x04, 0x00, 1) &&
        demod_write(device, 0, 0x61, 0x60, 1) &&
        demod_write(device, 0, 0x06, 0x80, 1) &&
        demod_write(device, 1, 0xb1, 0x1b, 1) &&
        demod_write(device, 0, 0x0d, 0x83, 1);
}

bool set_i2c_repeater(RtlDevice *device, bool enabled) {
    return demod_write(device, 1, 0x01, enabled ? 0x18 : 0x10, 1);
}

uint8_t read_i2c_register(RtlDevice *device, uint8_t address, uint8_t reg) {
    uint8_t value = 0;
    if (write_array(device, kI2cBlock, address, &reg, 1) != 1 ||
        read_array(device, kI2cBlock, address, &value, 1) != 1) {
        return 0;
    }
    return value;
}

bool set_if_frequency(RtlDevice *device, uint32_t frequency) {
    const int32_t value = static_cast<int32_t>(
        -(static_cast<double>(frequency) * static_cast<double>(1u << 22) / kRtlXtalHz));
    return demod_write(device, 1, 0x19, (value >> 16) & 0x3f, 1) &&
        demod_write(device, 1, 0x1a, (value >> 8) & 0xff, 1) &&
        demod_write(device, 1, 0x1b, value & 0xff, 1);
}

bool configure_sample_rate(RtlDevice *device, uint32_t requested_rate) {
    uint32_t ratio = static_cast<uint32_t>(
        (static_cast<uint64_t>(kRtlXtalHz) << 22) / requested_rate);
    ratio &= 0x0ffffffc;
    const uint32_t real_ratio = ratio | ((ratio & 0x08000000) << 1);
    device->sample_rate = static_cast<uint32_t>(
        (static_cast<uint64_t>(kRtlXtalHz) << 22) / real_ratio);

    if (!set_i2c_repeater(device, true)) return false;
    const int tuner_if = r82xx_set_bandwidth(
        &device->tuner, static_cast<int>(device->sample_rate), device->sample_rate);
    set_i2c_repeater(device, false);
    if (tuner_if < 0 || !set_if_frequency(device, static_cast<uint32_t>(tuner_if))) return false;

    return demod_write(device, 1, 0x9f, ratio >> 16, 2) &&
        demod_write(device, 1, 0xa1, ratio & 0xffff, 2) &&
        demod_write(device, 1, 0x3f, 0, 1) &&
        demod_write(device, 1, 0x3e, 0, 1) &&
        demod_write(device, 1, 0x01, 0x14, 1) &&
        demod_write(device, 1, 0x01, 0x10, 1);
}

bool initialize_tuner(RtlDevice *device) {
    if (!set_i2c_repeater(device, true)) return false;

    const uint8_t r820 = read_i2c_register(device, R820T_I2C_ADDR, R82XX_CHECK_ADDR);
    const uint8_t r828 = r820 == R82XX_CHECK_VAL ? 0 :
        read_i2c_register(device, R828D_I2C_ADDR, R82XX_CHECK_ADDR);
    if (r820 == R82XX_CHECK_VAL) {
        device->tuner_config.i2c_addr = R820T_I2C_ADDR;
        device->tuner_config.xtal = kRtlXtalHz;
        device->tuner_config.rafael_chip = CHIP_R820T;
        device->tuner_name = "R820T/R820T2";
    } else if (r828 == R82XX_CHECK_VAL) {
        device->tuner_config.i2c_addr = R828D_I2C_ADDR;
        device->tuner_config.xtal = R828D_XTAL_FREQ;
        device->tuner_config.rafael_chip = CHIP_R828D;
        device->tuner_name = "R828D";
    } else {
        set_i2c_repeater(device, false);
        return false;
    }
    device->tuner_config.max_i2c_msg_len = 8;
    device->tuner_config.use_predetect = 0;
    device->tuner.cfg = &device->tuner_config;
    device->tuner.rtl_dev = device;

    if (!demod_write(device, 1, 0xb1, 0x1a, 1) ||
        !demod_write(device, 0, 0x08, 0x4d, 1) ||
        !set_if_frequency(device, R82XX_IF_FREQ) ||
        !demod_write(device, 1, 0x15, 0x01, 1)) {
        set_i2c_repeater(device, false);
        return false;
    }

    const int result = r82xx_init(&device->tuner);
    device->tuner_initialized = result >= 0;
    set_i2c_repeater(device, false);
    return result >= 0;
}

bool tune(RtlDevice *device, uint32_t frequency) {
    if (!set_i2c_repeater(device, true)) return false;
    const int result = r82xx_set_freq(&device->tuner, frequency);
    set_i2c_repeater(device, false);
    if (result < 0) return false;
    device->frequency = frequency;
    return true;
}

bool reset_endpoint(RtlDevice *device) {
    return write_register(device, kUsbBlock, kUsbEndpointControl, 0x1002, 2) &&
        write_register(device, kUsbBlock, kUsbEndpointControl, 0x0000, 2);
}

void close_device(RtlDevice *device) {
    if (device == nullptr) return;
    if (device->session != USBHOST_TRANSPORT_INVALID_SESSION) {
        if (device->tuner_initialized) {
            if (set_i2c_repeater(device, true)) {
                r82xx_standby(&device->tuner);
                set_i2c_repeater(device, false);
            }
        }
        write_register(device, kSystemBlock, kDemodControl, 0x20, 1);
        if (device->interface_claimed) {
            g_usb.release_interface(device->session, 0);
        }
        g_usb.close(device->session);
    }
    free(device);
}

}  // namespace

extern "C" int rtlsdr_check_dongle_model(void *, char *, char *) {
    return 0;
}

extern "C" int rtlsdr_set_bias_tee_gpio(void *, int, int) {
    return 0;
}

extern "C" uint32_t rtlsdr_get_tuner_clock(void *opaque) {
    auto *device = static_cast<RtlDevice *>(opaque);
    return device->tuner_config.xtal;
}

extern "C" int rtlsdr_i2c_write_fn(void *opaque, uint8_t address,
                                     uint8_t *buffer, int length) {
    if (length <= 0 || length > 255) return -1;
    return write_array(static_cast<RtlDevice *>(opaque), kI2cBlock, address,
                       buffer, static_cast<uint8_t>(length));
}

extern "C" int rtlsdr_i2c_read_fn(void *opaque, uint8_t address,
                                    uint8_t *buffer, int length) {
    if (length <= 0 || length > 255) return -1;
    return read_array(static_cast<RtlDevice *>(opaque), kI2cBlock, address,
                      buffer, static_cast<uint8_t>(length));
}

extern "C" JNIEXPORT jlong JNICALL
Java_info_marcin_usbhost_example_rtlsdr_RtlSdrNative_nativeOpen(
    JNIEnv *env, jobject, jint fd, jlong frequency, jint sample_rate) {
    auto *device = static_cast<RtlDevice *>(calloc(1, sizeof(RtlDevice)));
    if (device == nullptr) {
        env->ThrowNew(env->FindClass("java/lang/OutOfMemoryError"), "RTL-SDR session allocation failed");
        return 0;
    }
    device->session = USBHOST_TRANSPORT_INVALID_SESSION;
    if (!load_usb_api()) {
        throw_java(env, "Stable usbhost C ABI could not be loaded");
        close_device(device);
        return 0;
    }
    if (frequency <= 0 || frequency > UINT32_MAX || sample_rate <= 0) {
        throw_java(env, "Invalid RTL-SDR frequency or sample rate");
        close_device(device);
        return 0;
    }
    usbhost_status status = g_usb.open_fd(fd, &device->session);
    if (status != USBHOST_OK) {
        char message[128];
        snprintf(message, sizeof(message), "USB transport open failed: %s",
                      g_usb.status_name(status));
        throw_java(env, message);
        close_device(device);
        return 0;
    }
    status = g_usb.claim_interface(device->session, 0);
    if (status != USBHOST_OK) {
        char message[128];
        snprintf(message, sizeof(message), "USB interface claim failed: %s",
                      g_usb.status_name(status));
        throw_java(env, message);
        close_device(device);
        return 0;
    }
    device->interface_claimed = true;
    if (!initialize_baseband(device)) {
        throw_java(env, "RTL2832U baseband initialization failed");
        close_device(device);
        return 0;
    }
    if (!initialize_tuner(device)) {
        throw_java(env, "No supported R820T/R828D tuner found");
        close_device(device);
        return 0;
    }
    if (!configure_sample_rate(device, static_cast<uint32_t>(sample_rate))) {
        throw_java(env, "RTL2832U sample-rate configuration failed");
        close_device(device);
        return 0;
    }
    if (!tune(device, static_cast<uint32_t>(frequency))) {
        throw_java(env, "R82xx tuning failed or PLL did not lock");
        close_device(device);
        return 0;
    }
    if (!reset_endpoint(device)) {
        throw_java(env, "RTL2832U endpoint reset failed");
        close_device(device);
        return 0;
    }
    __android_log_print(ANDROID_LOG_INFO, kLogTag,
                        "Ready tuner=%s frequency=%u sample_rate=%u",
                        device->tuner_name, device->frequency, device->sample_rate);
    return reinterpret_cast<jlong>(device);
}

extern "C" JNIEXPORT jint JNICALL
Java_info_marcin_usbhost_example_rtlsdr_RtlSdrNative_nativeRead(
    JNIEnv *env, jobject, jlong handle, jbyteArray destination) {
    auto *device = reinterpret_cast<RtlDevice *>(handle);
    if (device == nullptr || destination == nullptr) return -1;
    const jsize length = env->GetArrayLength(destination);
    jbyte *bytes = env->GetByteArrayElements(destination, nullptr);
    if (bytes == nullptr) return -1;
    uint32_t actual = 0;
    const usbhost_status status = g_usb.bulk_transfer(
        device->session, kBulkEndpointIn, reinterpret_cast<uint8_t *>(bytes),
        static_cast<uint32_t>(length), kBulkTimeoutMs, &actual);
    env->ReleaseByteArrayElements(destination, bytes, 0);
    if (status == USBHOST_TIMEOUT) return 0;
    if (status != USBHOST_OK) {
        __android_log_print(ANDROID_LOG_ERROR, kLogTag, "bulk read failed: %s",
                            g_usb.status_name(status));
        return -1;
    }
    return static_cast<jint>(actual);
}

extern "C" JNIEXPORT jstring JNICALL
Java_info_marcin_usbhost_example_rtlsdr_RtlSdrNative_nativeDescription(
    JNIEnv *env, jobject, jlong handle) {
    auto *device = reinterpret_cast<RtlDevice *>(handle);
    if (device == nullptr) return env->NewStringUTF("closed");
    char value[160];
    snprintf(value, sizeof(value), "%s - %u Hz - %u sample/s",
                  device->tuner_name, device->frequency, device->sample_rate);
    return env->NewStringUTF(value);
}

extern "C" JNIEXPORT void JNICALL
Java_info_marcin_usbhost_example_rtlsdr_RtlSdrNative_nativeClose(
    JNIEnv *, jobject, jlong handle) {
    close_device(reinterpret_cast<RtlDevice *>(handle));
}
