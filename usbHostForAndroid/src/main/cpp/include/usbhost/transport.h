#ifndef USBHOST_TRANSPORT_H
#define USBHOST_TRANSPORT_H

#include <stdint.h>

#ifndef USBHOST_USBHOST_H
#include "usbhost.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define USBHOST_TRANSPORT_INVALID_SESSION UINT64_C(0)
#define USBHOST_TRANSPORT_MAX_CONTROL_LENGTH UINT32_C(65535)
#define USBHOST_TRANSPORT_MAX_ENDPOINT_LENGTH UINT32_C(1048576)
#define USBHOST_TRANSPORT_MIN_TIMEOUT_MS UINT32_C(1)
#define USBHOST_TRANSPORT_MAX_TIMEOUT_MS UINT32_C(60000)
#define USBHOST_TRANSPORT_MAX_ADDITIONAL_DESCRIPTOR_LENGTH UINT32_C(65535)

typedef uint64_t usbhost_transport_session;

typedef enum usbhost_transport_direction {
    USBHOST_TRANSPORT_DIRECTION_OUT = 0,
    USBHOST_TRANSPORT_DIRECTION_IN = 1
} usbhost_transport_direction;

typedef enum usbhost_transport_transfer_type {
    USBHOST_TRANSPORT_TRANSFER_CONTROL = 0,
    USBHOST_TRANSPORT_TRANSFER_ISOCHRONOUS = 1,
    USBHOST_TRANSPORT_TRANSFER_BULK = 2,
    USBHOST_TRANSPORT_TRANSFER_INTERRUPT = 3
} usbhost_transport_transfer_type;

typedef enum usbhost_transport_descriptor_scope {
    USBHOST_TRANSPORT_DESCRIPTOR_CONFIGURATION = 1,
    USBHOST_TRANSPORT_DESCRIPTOR_ALTERNATE_SETTING = 2,
    USBHOST_TRANSPORT_DESCRIPTOR_ENDPOINT = 3
} usbhost_transport_descriptor_scope;

typedef struct usbhost_transport_device_descriptor {
    uint32_t struct_size;
    uint32_t reserved0;
    uint64_t snapshot_generation;
    uint16_t usb_version_bcd;
    uint8_t device_class;
    uint8_t device_subclass;
    uint8_t device_protocol;
    uint8_t endpoint_zero_max_packet_size;
    uint16_t vendor_id;
    uint16_t product_id;
    uint16_t device_release_bcd;
    uint16_t reserved1;
    uint32_t configuration_count;
} usbhost_transport_device_descriptor;

typedef struct usbhost_transport_configuration_descriptor {
    uint32_t struct_size;
    uint32_t configuration_index;
    uint64_t snapshot_generation;
    uint8_t configuration_value;
    uint8_t attributes;
    uint8_t maximum_power;
    uint8_t active;
    uint32_t interface_count;
    uint32_t additional_descriptor_count;
} usbhost_transport_configuration_descriptor;

typedef struct usbhost_transport_interface_descriptor {
    uint32_t struct_size;
    uint32_t interface_index;
    uint64_t snapshot_generation;
    uint8_t interface_number;
    uint8_t active_alternate_setting;
    uint8_t claimed;
    uint8_t reserved0;
    uint32_t alternate_setting_count;
} usbhost_transport_interface_descriptor;

typedef struct usbhost_transport_alternate_setting_descriptor {
    uint32_t struct_size;
    uint32_t alternate_setting_index;
    uint64_t snapshot_generation;
    uint8_t interface_number;
    uint8_t alternate_setting;
    uint8_t interface_class;
    uint8_t interface_subclass;
    uint8_t interface_protocol;
    uint8_t reserved0;
    uint16_t reserved1;
    uint32_t endpoint_count;
    uint32_t additional_descriptor_count;
} usbhost_transport_alternate_setting_descriptor;

typedef struct usbhost_transport_endpoint_descriptor {
    uint32_t struct_size;
    uint32_t endpoint_index;
    uint64_t snapshot_generation;
    uint8_t endpoint_address;
    uint8_t endpoint_number;
    uint8_t direction;
    uint8_t transfer_type;
    uint16_t maximum_packet_size;
    uint8_t interval;
    uint8_t reserved0;
    uint32_t additional_descriptor_count;
} usbhost_transport_endpoint_descriptor;

typedef struct usbhost_transport_descriptor_location {
    uint32_t struct_size;
    uint32_t scope;
    uint64_t snapshot_generation;
    uint32_t configuration_index;
    uint32_t interface_index;
    uint32_t alternate_setting_index;
    uint32_t endpoint_index;
    uint32_t additional_descriptor_index;
} usbhost_transport_descriptor_location;

USBHOST_API usbhost_status usbhost_transport_open_fd(
    int authorized_fd,
    usbhost_transport_session *out_session);

USBHOST_API usbhost_status usbhost_transport_cancel(
    usbhost_transport_session session);

USBHOST_API usbhost_status usbhost_transport_close(
    usbhost_transport_session session);

USBHOST_API usbhost_status usbhost_transport_get_device_descriptor(
    usbhost_transport_session session,
    usbhost_transport_device_descriptor *out_descriptor);

USBHOST_API usbhost_status usbhost_transport_get_configuration_count(
    usbhost_transport_session session,
    uint32_t *out_count);

USBHOST_API usbhost_status usbhost_transport_get_configuration_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    usbhost_transport_configuration_descriptor *out_descriptor);

USBHOST_API usbhost_status usbhost_transport_get_interface_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t *out_count);

USBHOST_API usbhost_status usbhost_transport_get_interface_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    usbhost_transport_interface_descriptor *out_descriptor);

USBHOST_API usbhost_status usbhost_transport_get_alternate_setting_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t *out_count);

USBHOST_API usbhost_status usbhost_transport_get_alternate_setting_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    usbhost_transport_alternate_setting_descriptor *out_descriptor);

USBHOST_API usbhost_status usbhost_transport_get_endpoint_count(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    uint32_t *out_count);

USBHOST_API usbhost_status usbhost_transport_get_endpoint_at(
    usbhost_transport_session session,
    uint32_t configuration_index,
    uint32_t interface_index,
    uint32_t alternate_setting_index,
    uint32_t endpoint_index,
    usbhost_transport_endpoint_descriptor *out_descriptor);

USBHOST_API usbhost_status usbhost_transport_get_additional_descriptor_at(
    usbhost_transport_session session,
    const usbhost_transport_descriptor_location *location,
    uint8_t *destination,
    uint32_t capacity,
    uint8_t *out_descriptor_type,
    uint32_t *out_actual_length);

USBHOST_API usbhost_status usbhost_transport_select_configuration(
    usbhost_transport_session session,
    uint8_t configuration_value);

USBHOST_API usbhost_status usbhost_transport_claim_interface(
    usbhost_transport_session session,
    uint8_t interface_number);

USBHOST_API usbhost_status usbhost_transport_select_alternate_setting(
    usbhost_transport_session session,
    uint8_t interface_number,
    uint8_t alternate_setting);

USBHOST_API usbhost_status usbhost_transport_release_interface(
    usbhost_transport_session session,
    uint8_t interface_number);

USBHOST_API usbhost_status usbhost_transport_control_transfer(
    usbhost_transport_session session,
    uint8_t request_type,
    uint8_t request,
    uint16_t value,
    uint16_t index,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);

USBHOST_API usbhost_status usbhost_transport_bulk_transfer(
    usbhost_transport_session session,
    uint8_t endpoint_address,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);

USBHOST_API usbhost_status usbhost_transport_interrupt_transfer(
    usbhost_transport_session session,
    uint8_t endpoint_address,
    uint8_t *buffer,
    uint32_t length,
    uint32_t timeout_ms,
    uint32_t *out_actual_length);

#ifdef __cplusplus
}
#endif

#endif
