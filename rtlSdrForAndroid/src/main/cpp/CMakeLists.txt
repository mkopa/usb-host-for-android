cmake_minimum_required(VERSION 3.22.1)
project(rtlsdr_audio LANGUAGES C CXX)

add_library(rtlsdr_audio SHARED
    rtlsdr_audio_jni.cpp
    third_party/rtl-sdr/src/tuner_r82xx.c
)

target_include_directories(rtlsdr_audio PRIVATE
    third_party/rtl-sdr/include
    ../../../../usbHostForAndroid/src/main/cpp/include
)

target_compile_features(rtlsdr_audio PRIVATE cxx_std_17)
target_compile_definitions(rtlsdr_audio PRIVATE _FILE_OFFSET_BITS=64)

target_link_libraries(rtlsdr_audio PRIVATE
    android
    dl
    log
)
