cmake_minimum_required(VERSION 3.22.1)
project(usbhost_native_tests LANGUAGES C CXX)

set(USBHOST_BUILD_TESTS ON CACHE BOOL "Build host-native tests" FORCE)
enable_testing()
add_subdirectory(../usbHostForAndroid/src/main/cpp usbhost)
