cmake_minimum_required(VERSION 3.22.1)
project(usbhost LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USBHOST_BUILD_TESTS "Build host-native tests" OFF)

get_filename_component(USBHOST_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
set(USBHOST_STLINK_DIR "${USBHOST_REPO_ROOT}/third_party/stlink")
set(USBHOST_LIBUSB_DIR "${USBHOST_REPO_ROOT}/third_party/libusb")

add_library(usbhost_core STATIC
    android/libusb_runtime.cpp
    core/session.cpp
    core/registry.cpp
    core/c_api.cpp
    transport/descriptors.cpp
    transport/error.cpp
    transport/registry.cpp
    stlink/stlink_usb_layout.c)
target_include_directories(usbhost_core PUBLIC
    "${CMAKE_CURRENT_LIST_DIR}"
    "${CMAKE_CURRENT_LIST_DIR}/include")
set_target_properties(usbhost_core PROPERTIES POSITION_INDEPENDENT_CODE ON)

if(ANDROID)
    if(NOT EXISTS "${USBHOST_LIBUSB_DIR}/libusb/core.c" OR
       NOT EXISTS "${USBHOST_STLINK_DIR}/src/stlink-lib/usb.c")
        message(FATAL_ERROR "Missing third-party sources. Run git submodule update --init --recursive")
    endif()

    target_include_directories(usbhost_core PRIVATE "${USBHOST_LIBUSB_DIR}/libusb")

    add_library(usb-1.0 SHARED
        "${USBHOST_LIBUSB_DIR}/libusb/core.c"
        "${USBHOST_LIBUSB_DIR}/libusb/descriptor.c"
        "${USBHOST_LIBUSB_DIR}/libusb/hotplug.c"
        "${USBHOST_LIBUSB_DIR}/libusb/io.c"
        "${USBHOST_LIBUSB_DIR}/libusb/sync.c"
        "${USBHOST_LIBUSB_DIR}/libusb/strerror.c"
        "${USBHOST_LIBUSB_DIR}/libusb/os/linux_usbfs.c"
        "${USBHOST_LIBUSB_DIR}/libusb/os/events_posix.c"
        "${USBHOST_LIBUSB_DIR}/libusb/os/threads_posix.c"
        "${USBHOST_LIBUSB_DIR}/libusb/os/linux_netlink.c")
    target_include_directories(usb-1.0 PRIVATE
        "${USBHOST_LIBUSB_DIR}/android"
        "${USBHOST_LIBUSB_DIR}/libusb"
        "${USBHOST_LIBUSB_DIR}/libusb/os")
    target_compile_options(usb-1.0 PRIVATE -fvisibility=hidden -pthread)
    target_link_libraries(usb-1.0 PRIVATE log)

    set(PROJECT_VERSION 1.9.0-dev)
    set(PROJECT_VERSION_MAJOR 1)
    set(PROJECT_VERSION_MINOR 9)
    set(PROJECT_VERSION_PATCH 0)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
    configure_file("${USBHOST_STLINK_DIR}/inc/version.h.in"
                   "${CMAKE_CURRENT_BINARY_DIR}/generated/version.h")

    set(STLINK_SOURCES
        calculate.c common_flash.c common_legacy.c flash_loader.c helper.c lib_md5.c logging.c
        map_file.c md5.c option_bytes.c read_write.c remote.c sg_legacy.c)
    list(TRANSFORM STLINK_SOURCES PREPEND "${USBHOST_STLINK_DIR}/src/stlink-lib/")

    add_library(stlink_android STATIC
        ${STLINK_SOURCES}
        stlink/stlink_chipid_android.c
        stlink/stlink_usb_android.c)
    target_include_directories(stlink_android PRIVATE
        "${USBHOST_LIBUSB_DIR}/libusb"
        "${USBHOST_STLINK_DIR}/inc"
        "${USBHOST_STLINK_DIR}/src"
        "${USBHOST_STLINK_DIR}/src/stlink-lib"
        "${CMAKE_CURRENT_BINARY_DIR}/generated"
        "${CMAKE_CURRENT_LIST_DIR}")
    target_compile_definitions(stlink_android PRIVATE
        STLINK_HAVE_UNISTD_H STLINK_HAVE_SYS_TIME_H STLINK_HAVE_SYS_MMAN_H)
    target_link_libraries(stlink_android PRIVATE usb-1.0 log)
    set_target_properties(stlink_android PROPERTIES POSITION_INDEPENDENT_CODE ON)

    add_library(usbhost SHARED
        stlink/stlink_backend.cpp
        jni/jni_bridge.cpp)
    target_include_directories(usbhost PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/include"
        "${USBHOST_LIBUSB_DIR}/libusb"
        "${USBHOST_STLINK_DIR}/inc"
        "${USBHOST_STLINK_DIR}/src/stlink-lib")
    target_link_libraries(usbhost PRIVATE usbhost_core stlink_android usb-1.0 log)
    target_compile_options(usbhost PRIVATE -fvisibility=hidden)
    target_link_options(usbhost PRIVATE
        "-Wl,--version-script=${CMAKE_CURRENT_LIST_DIR}/exports.map")
else()
    target_sources(usbhost_core PRIVATE core/backend_factory_stub.cpp)
endif()

if(USBHOST_BUILD_TESTS)
    find_package(Threads REQUIRED)
    enable_testing()
    add_executable(usbhost_tests
        tests/session_test.cpp
        tests/libusb_runtime_contract_test.cpp
        tests/scripted_usb_backend_test.cpp
        tests/status_contract_test.cpp
        tests/stlink_usb_contract_test.cpp
        tests/transport_header_contract_test.c
        tests/transport_header_cxx_contract_test.cpp
        tests/transport_descriptor_test.cpp
        tests/transport_registry_test.cpp
        tests/transport_error_test.cpp
        tests/transport_types_test.cpp
        tests/c_api_contract_test.c
        tests/cxx_api_contract_test.cpp)
    target_link_libraries(usbhost_tests PRIVATE usbhost_core Threads::Threads)
    add_test(NAME usbhost_tests COMMAND usbhost_tests)
endif()
