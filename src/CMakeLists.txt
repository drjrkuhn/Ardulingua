cmake_minimum_required(VERSION 3.8.0)
project(${CORELIB_NAME} VERSION 0.1.0)

message("CORELIB_NAME = ${CORELIB_NAME}")

add_library(${CORELIB_NAME} INTERFACE delegate.h jsondelegate.h jsondispatch.h Polyfills/utility.h)
target_compile_features(${CORELIB_NAME} INTERFACE cxx_std_11)
target_include_directories(${CORELIB_NAME} INTERFACE .  ../../ArduinoJson/src  ../../ArduinoCore-host/api)
set_target_properties(${CORELIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)

