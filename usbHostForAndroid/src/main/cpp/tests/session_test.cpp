#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>

#include "core/session.hpp"
#include "core/read_plan.hpp"

namespace {

int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::cerr << __FILE__ << ':' << __LINE__ << " CHECK failed: " #condition << '\n'; \
        ++failures; \
    } \
} while (0)

class FakeBackend final : public usbhost::Backend {
public:
    usbhost_programmer_info programmerInfo() const override {
        auto value = usbhost::emptyProgrammerInfo();
        value.stlink_version = 3;
        value.jtag_version = 12;
        return value;
    }

    usbhost::Result connectTarget(usbhost_target_info &target) override {
        ++connectCalls;
        if (connectResult != USBHOST_OK) {
            return usbhost::Result::error(connectResult, "connect failed");
        }
        target = usbhost::emptyTargetInfo();
        target.chip_id = chipId;
        target.flash_base = 0x08000000u;
        target.flash_size = 512u * 1024u;
        target.flash_page_size = 2048u;
        target.sram_base = 0x20000000u;
        target.sram_size = 144u * 1024u;
        target.target_voltage_mv = 3300;
        return usbhost::Result::ok();
    }

    usbhost::Result readMemory(uint32_t address, uint8_t *destination, uint32_t length) override {
        ++readCalls;
        const int activeNow = ++activeReads;
        int observed = maximumActiveReads.load();
        while (activeNow > observed && !maximumActiveReads.compare_exchange_weak(observed, activeNow)) {}
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
        for (uint32_t i = 0; i < length; ++i) {
            destination[i] = static_cast<uint8_t>((address + i) & 0xffu);
        }
        --activeReads;
        return usbhost::Result::ok();
    }

    void close() noexcept override { ++(*closeCalls); }

    usbhost_status connectResult{USBHOST_OK};
    uint32_t chipId{0x467u};
    int connectCalls{0};
    std::atomic<int> readCalls{0};
    std::atomic<int> activeReads{0};
    std::atomic<int> maximumActiveReads{0};
    std::shared_ptr<int> closeCalls{std::make_shared<int>(0)};
};

void lifecycleTest() {
    auto backend = std::make_unique<FakeBackend>();
    FakeBackend *raw = backend.get();
    std::shared_ptr<int> closeCalls = raw->closeCalls;
    usbhost::Session session(std::move(backend));
    CHECK(session.programmerInfo().stlink_version == 3u);
    CHECK(session.state() == usbhost::SessionState::ProgrammerReady);
    CHECK(session.close().status == USBHOST_OK);
    CHECK(session.close().status == USBHOST_OK);
    CHECK(*closeCalls == 1);
}

void targetAndReadTest() {
    auto backend = std::make_unique<FakeBackend>();
    FakeBackend *raw = backend.get();
    usbhost::Session session(std::move(backend));
    usbhost_target_info target = usbhost::emptyTargetInfo();
    CHECK(session.connectTarget(target).status == USBHOST_OK);
    CHECK(target.chip_id == 0x467u);
    CHECK(session.connectTarget(target).status == USBHOST_OK);
    CHECK(raw->connectCalls == 1);

    std::vector<uint8_t> bytes(1025);
    CHECK(session.readMemory(0x08000001u, bytes.data(), bytes.size()).status == USBHOST_OK);
    CHECK(bytes.front() == 1u);
    CHECK(bytes.back() == 1u);
    CHECK(raw->readCalls == 1);
}

void missingAndUnsupportedTargetTest() {
    {
        auto backend = std::make_unique<FakeBackend>();
        backend->connectResult = USBHOST_TARGET_NOT_FOUND;
        usbhost::Session session(std::move(backend));
        usbhost_target_info target = usbhost::emptyTargetInfo();
        CHECK(session.connectTarget(target).status == USBHOST_TARGET_NOT_FOUND);
        CHECK(session.state() == usbhost::SessionState::ProgrammerReady);
    }
    {
        auto backend = std::make_unique<FakeBackend>();
        backend->chipId = 0x410u;
        usbhost::Session session(std::move(backend));
        usbhost_target_info target = usbhost::emptyTargetInfo();
        CHECK(session.connectTarget(target).status == USBHOST_UNSUPPORTED_TARGET);
        CHECK(session.state() == usbhost::SessionState::ProgrammerReady);
    }
}

void serializationTest() {
    auto backend = std::make_unique<FakeBackend>();
    FakeBackend *raw = backend.get();
    usbhost::Session session(std::move(backend));
    usbhost_target_info target = usbhost::emptyTargetInfo();
    CHECK(session.connectTarget(target).status == USBHOST_OK);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) {
        workers.emplace_back([&session]() {
            uint8_t bytes[4]{};
            CHECK(session.readMemory(0x08000000u, bytes, sizeof(bytes)).status == USBHOST_OK);
        });
    }
    for (std::thread &worker : workers) worker.join();
    CHECK(raw->readCalls == 4);
    CHECK(raw->maximumActiveReads == 1);
}

void readPlanningTest() {
    const std::vector<usbhost::ReadChunk> chunks = usbhost::planAlignedRead(0x08000001u, 5000u);
    CHECK(chunks.size() == 2u);
    CHECK(chunks[0].transferAddress == 0x08000000u);
    CHECK(chunks[0].transferLength == 4096u);
    CHECK(chunks[0].sourceOffset == 1u);
    CHECK(chunks[0].destinationOffset == 0u);
    CHECK(chunks[1].destinationOffset == 4095u);
    CHECK(chunks[0].copyLength + chunks[1].copyLength == 5000u);
    CHECK(usbhost::planAlignedRead(0x08000000u, 0u).empty());
}

void invalidRangeTest() {
    auto backend = std::make_unique<FakeBackend>();
    FakeBackend *raw = backend.get();
    usbhost::Session session(std::move(backend));
    usbhost_target_info target = usbhost::emptyTargetInfo();
    CHECK(session.connectTarget(target).status == USBHOST_OK);
    uint8_t byte = 0;
    CHECK(session.readMemory(0x08000000u, &byte, 0).status == USBHOST_INVALID_ARGUMENT);
    CHECK(session.readMemory(0xfffffff0u, &byte, 32).status == USBHOST_INVALID_ARGUMENT);
    CHECK(session.readMemory(0x08080000u, &byte, 1).status == USBHOST_INVALID_ARGUMENT);
    CHECK(session.readMemory(0x20000000u, &byte, USBHOST_MAX_READ_SIZE + 1u).status
          == USBHOST_INVALID_ARGUMENT);
    CHECK(raw->readCalls == 0);
}

void terminalFailureTest() {
    auto backend = std::make_unique<FakeBackend>();
    FakeBackend *raw = backend.get();
    raw->connectResult = USBHOST_DISCONNECTED;
    usbhost::Session session(std::move(backend));
    usbhost_target_info target = usbhost::emptyTargetInfo();
    CHECK(session.connectTarget(target).status == USBHOST_DISCONNECTED);
    CHECK(session.state() == usbhost::SessionState::Failed);
    uint8_t byte = 0;
    CHECK(session.readMemory(0x08000000u, &byte, 1).status == USBHOST_INVALID_STATE);
    CHECK(session.close().status == USBHOST_OK);
}

}  // namespace

int runStlinkUsbContractTest();

int main() {
    lifecycleTest();
    targetAndReadTest();
    missingAndUnsupportedTargetTest();
    serializationTest();
    readPlanningTest();
    invalidRangeTest();
    terminalFailureTest();
    CHECK(runStlinkUsbContractTest() == 0);
    CHECK(usbhost_close(0xdeadbeefULL) == USBHOST_OK);
    if (failures != 0) {
        std::cerr << failures << " native test(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All native tests passed\n";
    return EXIT_SUCCESS;
}
