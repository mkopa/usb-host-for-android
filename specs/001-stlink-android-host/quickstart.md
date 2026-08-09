# Quickstart: Validate ST-Link Android Host

## Prerequisites

- Windows 11 PowerShell
- JDK 17 or newer accepted by Gradle
- Android SDK platform 35, NDK `28.2.13676358`, and CMake `3.22.1`
- Git submodules initialized
- For hardware steps: USB-host Android device, ST-Link V3 in debug mode, STM32G0B0RET6 target,
  SWDIO/SWCLK/GND/VTref wiring, and valid target power

## Build and automated validation

```powershell
git submodule update --init --recursive
./gradlew.bat clean test assembleDebug
cmake -S native-tests -B build/native-tests
cmake --build build/native-tests --config Debug
ctest --test-dir build/native-tests -C Debug --output-on-failure
```

Expected: Gradle builds the library and sample, JVM tests pass, and native tests pass without USB.

## Hardware validation

1. Install the debug sample on the Android device.
2. Attach ST-Link V3 and grant the system USB permission prompt.
3. Confirm the sample lists the selected programmer and reports ST-Link V3 firmware information.
4. Connect the powered STM32G0B0RET6 and run target identification.
5. Confirm chip ID `0x467`, flash size 512 KiB, SRAM size 144 KiB, page size 2 KiB, and plausible
   target voltage.
6. Read 1 byte, 4 bytes, 1 KiB, and 64 KiB from flash and compare SHA-256 with an independent read.
7. Repeat target identification and each read size 20 times.
8. Detach ST-Link during a read and verify a terminal disconnection result and safe repeated close.
9. Copy `docs/hardware/evidence-template.md`, record the environment/results, and redact serials and
   memory contents from shared logs.

## Safety boundary

This feature contains no public erase, program, option-byte, reset, halt, run, or register-write
operation. If any validation step appears to modify target memory, stop and report it as a defect.
