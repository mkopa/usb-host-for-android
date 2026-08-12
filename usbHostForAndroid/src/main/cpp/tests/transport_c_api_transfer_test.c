#include <stdint.h>

#include "usbhost/transport.h"

void usbhost_test_transport_install_fixture(int open_status);
void usbhost_test_transport_set_completion(int status, uint32_t actual_length,
                                           int defer_until_cancel);
void usbhost_test_transport_schedule_cancel(usbhost_transport_session session);
int usbhost_test_transport_join_cancel(void);
int usbhost_test_transport_observed_transfer_type(void);
int usbhost_test_transport_observed_endpoint(void);

static int failures;
#define CHECK_C_TRANSFER(value) do { if (!(value)) ++failures; } while (0)

static void transferContract(void) {
    usbhost_transport_session session = USBHOST_TRANSPORT_INVALID_SESSION;
    uint8_t bytes[8] = {0};
    uint32_t actual = UINT32_MAX;

    usbhost_test_transport_install_fixture(USBHOST_OK);
    CHECK_C_TRANSFER(usbhost_transport_open_fd(51, &session) == USBHOST_OK);

    usbhost_test_transport_set_completion(USBHOST_TIMEOUT, 3, 0);
    CHECK_C_TRANSFER(usbhost_transport_control_transfer(
        session, 0x80, 6, 0x0100, 0, bytes, sizeof(bytes), 25, &actual)
        == USBHOST_TIMEOUT);
    CHECK_C_TRANSFER(actual == 3);
    CHECK_C_TRANSFER(usbhost_test_transport_observed_transfer_type()
                     == USBHOST_TRANSPORT_TRANSFER_CONTROL);

    CHECK_C_TRANSFER(usbhost_transport_claim_interface(session, 3) == USBHOST_OK);
    usbhost_test_transport_set_completion(USBHOST_OK, 4, 0);
    actual = UINT32_MAX;
    CHECK_C_TRANSFER(usbhost_transport_bulk_transfer(
        session, 0x81, bytes, sizeof(bytes), 50, &actual) == USBHOST_OK);
    CHECK_C_TRANSFER(actual == 4);
    CHECK_C_TRANSFER(usbhost_test_transport_observed_transfer_type()
                     == USBHOST_TRANSPORT_TRANSFER_BULK);
    CHECK_C_TRANSFER(usbhost_test_transport_observed_endpoint() == 0x81);

    CHECK_C_TRANSFER(usbhost_transport_select_alternate_setting(session, 3, 1)
                     == USBHOST_OK);
    usbhost_test_transport_set_completion(USBHOST_OK, 2, 0);
    actual = UINT32_MAX;
    CHECK_C_TRANSFER(usbhost_transport_interrupt_transfer(
        session, 0x82, bytes, sizeof(bytes), 50, &actual) == USBHOST_OK);
    CHECK_C_TRANSFER(actual == 2);
    CHECK_C_TRANSFER(usbhost_test_transport_observed_transfer_type()
                     == USBHOST_TRANSPORT_TRANSFER_INTERRUPT);
    CHECK_C_TRANSFER(usbhost_test_transport_observed_endpoint() == 0x82);

    actual = UINT32_MAX;
    CHECK_C_TRANSFER(usbhost_transport_bulk_transfer(
        session, 0x82, bytes, sizeof(bytes), 50, &actual) == USBHOST_INVALID_ARGUMENT);
    CHECK_C_TRANSFER(actual == 0);
    CHECK_C_TRANSFER(usbhost_transport_control_transfer(
        session, 0x80, 0, 0, 0, bytes, sizeof(bytes), 1, 0)
        == USBHOST_INVALID_ARGUMENT);
    CHECK_C_TRANSFER(usbhost_transport_bulk_transfer(
        session, 0x82, 0, 1, 50, &actual) == USBHOST_INVALID_ARGUMENT);
    CHECK_C_TRANSFER(usbhost_transport_cancel(session) == USBHOST_INVALID_STATE);

    usbhost_test_transport_set_completion(USBHOST_CANCELLED, 2, 1);
    usbhost_test_transport_schedule_cancel(session);
    actual = UINT32_MAX;
    CHECK_C_TRANSFER(usbhost_transport_control_transfer(
        session, 0x00, 9, 0, 0, bytes, sizeof(bytes), 1000, &actual)
        == USBHOST_CANCELLED);
    CHECK_C_TRANSFER(actual == 2);
    CHECK_C_TRANSFER(usbhost_test_transport_join_cancel() == USBHOST_OK);

    CHECK_C_TRANSFER(usbhost_transport_release_interface(session, 3) == USBHOST_OK);
    CHECK_C_TRANSFER(usbhost_transport_close(session) == USBHOST_OK);
}

int runTransportCApiTransferTest(void) {
    failures = 0;
    transferContract();
    return failures;
}
