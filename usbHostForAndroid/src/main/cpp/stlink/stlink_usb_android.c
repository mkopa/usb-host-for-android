#include <libusb.h>

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "stlink/stlink_usb_android.h"

static _Thread_local int usbhost_transport_error = LIBUSB_SUCCESS;

static int usbhost_tracked_bulk_transfer(
    libusb_device_handle *dev_handle, unsigned char endpoint, unsigned char *data,
    int length, int *actual_length, unsigned int timeout) {
    int result = libusb_bulk_transfer(dev_handle, endpoint, data, length, actual_length, timeout);
    if (result != LIBUSB_SUCCESS) {
        usbhost_transport_error = result;
    }
    return result;
}

#define libusb_bulk_transfer usbhost_tracked_bulk_transfer
#include "../../../../../third_party/stlink/src/stlink-lib/usb.c"
#undef libusb_bulk_transfer

static int32_t usbhost_deny_simple(stlink_t *sl) {
    (void)sl;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_reset(stlink_t *sl, int32_t value) {
    (void)sl;
    (void)value;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_run(stlink_t *sl, enum run_type type) {
    (void)sl;
    (void)type;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_write_debug32(stlink_t *sl, uint32_t address, uint32_t value) {
    (void)sl;
    (void)address;
    (void)value;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_write_memory(stlink_t *sl, uint32_t address, uint16_t length) {
    (void)sl;
    (void)address;
    (void)length;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_write_unsupported_reg(
        stlink_t *sl, uint32_t value, int32_t index, struct stlink_reg *reg) {
    (void)sl;
    (void)value;
    (void)index;
    (void)reg;
    errno = EPERM;
    return -1;
}

static int32_t usbhost_deny_write_reg(stlink_t *sl, uint32_t value, int32_t index) {
    (void)sl;
    (void)value;
    (void)index;
    errno = EPERM;
    return -1;
}

static pthread_once_t usbhost_read_only_backend_once = PTHREAD_ONCE_INIT;

static void usbhost_install_read_only_backend(void) {
    _stlink_usb_backend.reset = usbhost_deny_simple;
    _stlink_usb_backend.jtag_reset = usbhost_deny_reset;
    _stlink_usb_backend.run = usbhost_deny_run;
    _stlink_usb_backend.write_debug32 = usbhost_deny_write_debug32;
    _stlink_usb_backend.write_mem32 = usbhost_deny_write_memory;
    _stlink_usb_backend.write_mem8 = usbhost_deny_write_memory;
    _stlink_usb_backend.write_unsupported_reg = usbhost_deny_write_unsupported_reg;
    _stlink_usb_backend.write_reg = usbhost_deny_write_reg;
    _stlink_usb_backend.step = usbhost_deny_simple;
    _stlink_usb_backend.force_debug = usbhost_deny_simple;
}

void usbhost_stlink_reset_transport_error(void) {
    usbhost_transport_error = LIBUSB_SUCCESS;
}

int usbhost_stlink_last_transport_error(void) {
    return usbhost_transport_error;
}

stlink_t *usbhost_stlink_open_fd(int fd, uint16_t vendor_id, uint16_t product_id,
                                 int32_t swd_frequency_khz) {
    usbhost_stlink_layout layout;
    if (fd < 0 || !usbhost_stlink_v3_layout(vendor_id, product_id, &layout)) {
        errno = EINVAL;
        return NULL;
    }

    if (pthread_once(&usbhost_read_only_backend_once, usbhost_install_read_only_backend) != 0) {
        errno = EIO;
        return NULL;
    }

    stlink_t *sl = calloc(1, sizeof(stlink_t));
    struct stlink_libusb *transport = calloc(1, sizeof(struct stlink_libusb));
    if (sl == NULL || transport == NULL) {
        free(sl);
        free(transport);
        errno = ENOMEM;
        return NULL;
    }

    sl->backend = &_stlink_usb_backend;
    sl->backend_data = transport;
    sl->core_stat = TARGET_UNKNOWN;
    sl->version.stlink_v = 3;
    transport->ep_req = layout.request_endpoint;
    transport->ep_rep = layout.reply_endpoint;
    transport->ep_trace = layout.trace_endpoint;
    transport->cmd_len = STLINK_CMD_SIZE;

    const struct libusb_init_option options[] = {
        {.option = LIBUSB_OPTION_NO_DEVICE_DISCOVERY, .value = {.ival = 1}},
        {.option = LIBUSB_OPTION_LOG_LEVEL, .value = {.ival = LIBUSB_LOG_LEVEL_WARNING}}
    };
    int result = libusb_init_context(&transport->libusb_ctx, options, 2);
    if (result != LIBUSB_SUCCESS) {
        usbhost_transport_error = result;
        goto error;
    }

    result = libusb_wrap_sys_device(transport->libusb_ctx, (intptr_t)fd,
                                    &transport->usb_handle);
    if (result != LIBUSB_SUCCESS || transport->usb_handle == NULL) {
        usbhost_transport_error = result == LIBUSB_SUCCESS ? LIBUSB_ERROR_OTHER : result;
        goto error;
    }

    struct libusb_device_descriptor descriptor;
    result = libusb_get_device_descriptor(libusb_get_device(transport->usb_handle), &descriptor);
    if (result != LIBUSB_SUCCESS || descriptor.idVendor != vendor_id
            || descriptor.idProduct != product_id) {
        usbhost_transport_error = result == LIBUSB_SUCCESS ? LIBUSB_ERROR_INVALID_PARAM : result;
        goto error;
    }

    (void)stlink_serial(transport->usb_handle, &descriptor, sl->serial);

    int configuration = 0;
    result = libusb_get_configuration(transport->usb_handle, &configuration);
    if (result != LIBUSB_SUCCESS) {
        usbhost_transport_error = result;
        goto error;
    }
    if (configuration != 1) {
        result = libusb_set_configuration(transport->usb_handle, 1);
        if (result != LIBUSB_SUCCESS) {
            usbhost_transport_error = result;
            goto error;
        }
    }

    result = libusb_claim_interface(transport->usb_handle, 0);
    if (result != LIBUSB_SUCCESS) {
        usbhost_transport_error = result;
        goto error;
    }

    ugly_init(UERROR);
    usbhost_stlink_reset_transport_error();
    if (stlink_version(sl) != 0) {
        goto error;
    }
    sl->freq = swd_frequency_khz;
    if (stlink_set_swdclk(sl, swd_frequency_khz) != 0) {
        goto error;
    }
    return sl;

error:
    stlink_close(sl);
    return NULL;
}
