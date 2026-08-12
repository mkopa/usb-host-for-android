#include <libusb.h>

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdlib.h>

#include "stlink/stlink_transport_adapter.hpp"
#include "stlink/stlink_usb_android.h"

static _Thread_local int usbhost_transport_error = LIBUSB_SUCCESS;

struct usbhost_libusb_transport {
    libusb_context *context;
    libusb_device_handle *handle;
};

static usbhost_status usbhost_status_from_libusb(int result) {
    switch (result) {
        case LIBUSB_SUCCESS: return USBHOST_OK;
        case LIBUSB_ERROR_INVALID_PARAM: return USBHOST_INVALID_ARGUMENT;
        case LIBUSB_ERROR_ACCESS: return USBHOST_PERMISSION_DENIED;
        case LIBUSB_ERROR_BUSY: return USBHOST_BUSY;
        case LIBUSB_ERROR_TIMEOUT: return USBHOST_TIMEOUT;
        case LIBUSB_ERROR_PIPE: return USBHOST_STALL;
        case LIBUSB_ERROR_INTERRUPTED: return USBHOST_CANCELLED;
        case LIBUSB_ERROR_NO_DEVICE:
        case LIBUSB_ERROR_NOT_FOUND: return USBHOST_DISCONNECTED;
        case LIBUSB_ERROR_NOT_SUPPORTED: return USBHOST_UNSUPPORTED_OPERATION;
        default: return USBHOST_USB_ERROR;
    }
}

static int usbhost_status_to_libusb(usbhost_status status) {
    switch (status) {
        case USBHOST_OK: return LIBUSB_SUCCESS;
        case USBHOST_INVALID_ARGUMENT: return LIBUSB_ERROR_INVALID_PARAM;
        case USBHOST_PERMISSION_DENIED: return LIBUSB_ERROR_ACCESS;
        case USBHOST_BUSY: return LIBUSB_ERROR_BUSY;
        case USBHOST_TIMEOUT: return LIBUSB_ERROR_TIMEOUT;
        case USBHOST_STALL: return LIBUSB_ERROR_PIPE;
        case USBHOST_CANCELLED: return LIBUSB_ERROR_INTERRUPTED;
        case USBHOST_DISCONNECTED: return LIBUSB_ERROR_NO_DEVICE;
        case USBHOST_UNSUPPORTED_OPERATION: return LIBUSB_ERROR_NOT_SUPPORTED;
        default: return LIBUSB_ERROR_IO;
    }
}

static usbhost_status usbhost_libusb_claim(void *context, uint8_t interface_number) {
    struct usbhost_libusb_transport *transport = context;
    const int result = libusb_claim_interface(transport->handle, interface_number);
    if (result != LIBUSB_SUCCESS) usbhost_transport_error = result;
    return usbhost_status_from_libusb(result);
}

static usbhost_status usbhost_libusb_bulk(
        void *context, uint8_t endpoint, uint8_t *buffer, uint32_t length,
        uint32_t timeout, uint32_t *actual_length) {
    struct usbhost_libusb_transport *transport = context;
    int actual = 0;
    const int result = libusb_bulk_transfer(
        transport->handle, endpoint, buffer, (int)length, &actual, timeout);
    *actual_length = actual < 0 ? 0u : (uint32_t)actual;
    if (result != LIBUSB_SUCCESS) usbhost_transport_error = result;
    return usbhost_status_from_libusb(result);
}

static usbhost_status usbhost_libusb_release(void *context, uint8_t interface_number) {
    struct usbhost_libusb_transport *transport = context;
    const int result = libusb_release_interface(transport->handle, interface_number);
    if (result != LIBUSB_SUCCESS && result != LIBUSB_ERROR_NO_DEVICE)
        usbhost_transport_error = result;
    return usbhost_status_from_libusb(result);
}

static void usbhost_libusb_close(void *context) {
    struct usbhost_libusb_transport *transport = context;
    if (transport == NULL) return;
    if (transport->handle != NULL) libusb_close(transport->handle);
    if (transport->context != NULL) libusb_exit(transport->context);
    free(transport);
}

static int usbhost_injected_bulk_transfer(
        libusb_device_handle *opaque_adapter, unsigned char endpoint, unsigned char *data,
        int length, int *actual_length, unsigned int timeout) {
    if (actual_length == NULL || length < 0) return LIBUSB_ERROR_INVALID_PARAM;
    uint32_t actual = 0;
    const usbhost_status status = usbhost_stlink_transport_adapter_bulk(
        (usbhost_stlink_transport_adapter *)opaque_adapter, endpoint, data,
        (uint32_t)length, timeout, &actual);
    *actual_length = (int)actual;
    const int result = usbhost_status_to_libusb(status);
    if (result != LIBUSB_SUCCESS) usbhost_transport_error = result;
    return result;
}

static void usbhost_injected_close(libusb_device_handle *opaque_adapter) {
    usbhost_stlink_transport_adapter_destroy(
        (usbhost_stlink_transport_adapter *)opaque_adapter);
}

static void usbhost_injected_exit(libusb_context *unused) {
    (void)unused;
}

#define libusb_bulk_transfer usbhost_injected_bulk_transfer
#define libusb_close usbhost_injected_close
#define libusb_exit usbhost_injected_exit
#include "../../../../../third_party/stlink/src/stlink-lib/usb.c"
#undef libusb_exit
#undef libusb_close
#undef libusb_bulk_transfer

static int usbhost_create_libusb_hooks(
        int fd, uint16_t vendor_id, uint16_t product_id, char *serial,
        usbhost_stlink_transport_hooks *out_hooks) {
    struct usbhost_libusb_transport *transport = calloc(1, sizeof(*transport));
    if (transport == NULL) return LIBUSB_ERROR_NO_MEM;

    const struct libusb_init_option options[] = {
        {.option = LIBUSB_OPTION_NO_DEVICE_DISCOVERY, .value = {.ival = 1}},
        {.option = LIBUSB_OPTION_LOG_LEVEL, .value = {.ival = LIBUSB_LOG_LEVEL_WARNING}}
    };
    int result = libusb_init_context(&transport->context, options, 2);
    if (result != LIBUSB_SUCCESS) goto error;
    result = libusb_wrap_sys_device(
        transport->context, (intptr_t)fd, &transport->handle);
    if (result != LIBUSB_SUCCESS || transport->handle == NULL) {
        if (result == LIBUSB_SUCCESS) result = LIBUSB_ERROR_OTHER;
        goto error;
    }

    struct libusb_device_descriptor descriptor;
    result = libusb_get_device_descriptor(libusb_get_device(transport->handle), &descriptor);
    if (result != LIBUSB_SUCCESS) goto error;
    if (descriptor.idVendor != vendor_id || descriptor.idProduct != product_id) {
        result = LIBUSB_ERROR_INVALID_PARAM;
        goto error;
    }
    (void)stlink_serial(transport->handle, &descriptor, serial);

    int configuration = 0;
    result = libusb_get_configuration(transport->handle, &configuration);
    if (result != LIBUSB_SUCCESS) goto error;
    if (configuration != 1) {
        result = libusb_set_configuration(transport->handle, 1);
        if (result != LIBUSB_SUCCESS) goto error;
    }

    out_hooks->context = transport;
    out_hooks->claim_interface = usbhost_libusb_claim;
    out_hooks->bulk_transfer = usbhost_libusb_bulk;
    out_hooks->release_interface = usbhost_libusb_release;
    out_hooks->close = usbhost_libusb_close;
    return LIBUSB_SUCCESS;

error:
    usbhost_libusb_close(transport);
    return result;
}

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

    usbhost_stlink_transport_hooks hooks;
    int result = usbhost_create_libusb_hooks(fd, vendor_id, product_id, sl->serial, &hooks);
    if (result != LIBUSB_SUCCESS) {
        usbhost_transport_error = result;
        goto error;
    }
    usbhost_status open_status = USBHOST_INTERNAL_ERROR;
    usbhost_stlink_transport_adapter *adapter =
        usbhost_stlink_transport_adapter_open(&hooks, &layout, &open_status);
    if (adapter == NULL) {
        usbhost_transport_error = usbhost_status_to_libusb(open_status);
        goto error;
    }
    transport->usb_handle = (libusb_device_handle *)adapter;

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
