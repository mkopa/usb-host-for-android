#include "jni/transport_jni_contract.hpp"

#include <cstdint>
#include <stdexcept>

int runTransportJniTransferTest() {
    usbhost::jni::TransferSlice slice{};
    if (!usbhost::jni::checkedTransferSlice(8, 2, 4, slice)) return 1;
    if (slice.offset != 2 || slice.length != 4) return 1;
    if (usbhost::jni::checkedTransferSlice(8, -1, 1, slice)) return 1;
    if (usbhost::jni::checkedTransferSlice(8, 7, 2, slice)) return 1;
    if (usbhost::jni::checkedTransferSlice(8, 0, -1, slice)) return 1;

    usbhost::jni::CompletedInputCopy copy{};
    const usbhost::jni::TransferSlice requested{2, 4};
    if (!usbhost::jni::completedInputCopy(requested, 2, copy)) return 1;
    if (copy.destinationOffset != 2 || copy.length != 2) return 1;
    if (usbhost::jni::completedInputCopy(requested, 5, copy)) return 1;

    const auto success = usbhost::jni::executeTransferNoexcept([] {
        return usbhost::jni::NativeTransferResult{USBHOST_TIMEOUT, 3};
    });
    if (success.status != USBHOST_TIMEOUT || success.actualLength != 3) return 1;
    const auto failure = usbhost::jni::executeTransferNoexcept([]()
            -> usbhost::jni::NativeTransferResult {
        throw std::runtime_error("fixture");
    });
    return failure.status == USBHOST_INTERNAL_ERROR && failure.actualLength == 0 ? 0 : 1;
}
