#!/usr/bin/env bash
set -euo pipefail

java -version
sdkmanager --version
test -x "${ANDROID_HOME}/ndk/28.2.13676358/toolchains/llvm/prebuilt/linux-x86_64/bin/clang"
test -x "${ANDROID_HOME}/cmake/3.22.1/bin/cmake"
test -d "${ANDROID_HOME}/platforms/android-37"
