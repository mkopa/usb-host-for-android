#include <stdint.h>
#include <string.h>

#include "usbhost/transport.h"

void usbhost_test_transport_install_fixture(int open_status);
int usbhost_test_transport_observed_fd(void);
int usbhost_test_transport_backend_close_count(void);

static int failures;

#define CHECK_C_API(condition) do { if (!(condition)) ++failures; } while (0)

static void failedOpenContract(void) {
    usbhost_transport_session session = UINT64_C(99);
    usbhost_test_transport_install_fixture(USBHOST_PERMISSION_DENIED);
    CHECK_C_API(usbhost_transport_open_fd(40, &session) == USBHOST_PERMISSION_DENIED);
    CHECK_C_API(session == USBHOST_TRANSPORT_INVALID_SESSION);
    CHECK_C_API(usbhost_transport_open_fd(40, NULL) == USBHOST_INVALID_ARGUMENT);
}

static void descriptorAndStateContract(void) {
    usbhost_test_transport_install_fixture(USBHOST_OK);
    usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
    CHECK_C_API(usbhost_transport_open_fd(41, &session) == USBHOST_OK);
    CHECK_C_API(session != USBHOST_TRANSPORT_INVALID_SESSION);
    CHECK_C_API(usbhost_test_transport_observed_fd() == 41);

    usbhost_transport_device_descriptor device = {0};
    device.struct_size = sizeof(device);
    CHECK_C_API(usbhost_transport_get_device_descriptor(session, &device) == USBHOST_OK);
    CHECK_C_API(device.vendor_id == 0x1234 && device.product_id == 0x5678);
    CHECK_C_API(device.configuration_count == 1 && device.snapshot_generation != 0);

    uint32_t count = 0;
    CHECK_C_API(usbhost_transport_get_configuration_count(session, &count) == USBHOST_OK);
    CHECK_C_API(count == 1);
    usbhost_transport_configuration_descriptor configuration = {0};
    configuration.struct_size = sizeof(configuration);
    CHECK_C_API(usbhost_transport_get_configuration_at(session, 0, &configuration) == USBHOST_OK);
    CHECK_C_API(configuration.configuration_value == 1 && configuration.active == 1);
    CHECK_C_API(configuration.interface_count == 1);

    CHECK_C_API(usbhost_transport_get_interface_count(session, 0, &count) == USBHOST_OK);
    CHECK_C_API(count == 1);
    usbhost_transport_interface_descriptor interface_descriptor = {0};
    interface_descriptor.struct_size = sizeof(interface_descriptor);
    CHECK_C_API(usbhost_transport_get_interface_at(
                    session, 0, 0, &interface_descriptor) == USBHOST_OK);
    CHECK_C_API(interface_descriptor.interface_number == 3);
    CHECK_C_API(interface_descriptor.alternate_setting_count == 2);

    CHECK_C_API(usbhost_transport_get_alternate_setting_count(
                    session, 0, 0, &count) == USBHOST_OK);
    CHECK_C_API(count == 2);
    usbhost_transport_alternate_setting_descriptor alternate = {0};
    alternate.struct_size = sizeof(alternate);
    CHECK_C_API(usbhost_transport_get_alternate_setting_at(
                    session, 0, 0, 0, &alternate) == USBHOST_OK);
    CHECK_C_API(alternate.endpoint_count == 1);
    CHECK_C_API(alternate.additional_descriptor_count == 1);

    CHECK_C_API(usbhost_transport_get_endpoint_count(session, 0, 0, 0, &count) == USBHOST_OK);
    CHECK_C_API(count == 1);
    usbhost_transport_endpoint_descriptor endpoint = {0};
    endpoint.struct_size = sizeof(endpoint);
    CHECK_C_API(usbhost_transport_get_endpoint_at(session, 0, 0, 0, 0, &endpoint)
                == USBHOST_OK);
    CHECK_C_API(endpoint.endpoint_address == 0x81);

    usbhost_transport_descriptor_location location = {0};
    location.struct_size = sizeof(location);
    location.scope = USBHOST_TRANSPORT_DESCRIPTOR_ALTERNATE_SETTING;
    location.snapshot_generation = device.snapshot_generation;
    location.configuration_index = 0;
    location.interface_index = 0;
    location.alternate_setting_index = 0;
    uint8_t descriptor_bytes[4] = {0};
    uint8_t descriptor_type = 0;
    uint32_t actual = 0;
    CHECK_C_API(usbhost_transport_get_additional_descriptor_at(
                    session, &location, descriptor_bytes, 2,
                    &descriptor_type, &actual) == USBHOST_INVALID_ARGUMENT);
    CHECK_C_API(actual == 3);
    CHECK_C_API(usbhost_transport_get_additional_descriptor_at(
                    session, &location, descriptor_bytes, sizeof(descriptor_bytes),
                    &descriptor_type, &actual) == USBHOST_OK);
    CHECK_C_API(descriptor_type == 0x24 && actual == 3);
    CHECK_C_API(descriptor_bytes[0] == 3 && descriptor_bytes[2] == 0xaa);

    CHECK_C_API(usbhost_transport_claim_interface(session, 3) == USBHOST_OK);
    CHECK_C_API(usbhost_transport_select_configuration(session, 1) == USBHOST_BUSY);
    CHECK_C_API(usbhost_transport_select_alternate_setting(session, 3, 1) == USBHOST_OK);
    CHECK_C_API(usbhost_transport_release_interface(session, 3) == USBHOST_OK);
    CHECK_C_API(usbhost_transport_select_configuration(session, 1) == USBHOST_OK);

    usbhost_transport_device_descriptor too_small = {0};
    too_small.struct_size = sizeof(uint32_t);
    CHECK_C_API(usbhost_transport_get_device_descriptor(session, &too_small)
                == USBHOST_INVALID_ARGUMENT);
    CHECK_C_API(usbhost_transport_get_configuration_at(session, 7, &configuration)
                == USBHOST_INVALID_ARGUMENT);

    CHECK_C_API(usbhost_transport_close(session) == USBHOST_OK);
    CHECK_C_API(usbhost_transport_close(session) == USBHOST_OK);
    CHECK_C_API(usbhost_test_transport_backend_close_count() == 1);
    CHECK_C_API(usbhost_transport_get_configuration_count(session, &count)
                == USBHOST_INVALID_STATE);
    CHECK_C_API(usbhost_transport_close(session ^ (UINT64_C(1) << 32))
                == USBHOST_INVALID_STATE);
    CHECK_C_API(usbhost_last_status() == USBHOST_INVALID_STATE);
    CHECK_C_API(strlen(usbhost_last_error()) != 0);
}

int runTransportCApiSessionTest(void) {
    failures = 0;
    failedOpenContract();
    descriptorAndStateContract();
    return failures;
}
