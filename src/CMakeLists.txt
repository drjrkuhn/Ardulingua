cmake_minimum_required(VERSION 3.8.0)
project(${CORELIB_NAME} VERSION 0.1.0)

message("CORELIB_NAME = ${CORELIB_NAME}")

add_library(${CORELIB_NAME} INTERFACE 
    rdl/Delegate.h 
    rdl/Jsondelegate.h 
    rdl/JsonDispatch.h 
    rdl/Logger.h 
    rdl/common_rdl.h 
    rdl/ServerProperty.h
    rdl/SlipInPlace.h 
    rdl/StringT.h
    rdl/Polyfills/std_utility.h
    rdl/Polyfills/std_type_traits.h
    rdl/Polyfills/sys_timing.h
)
target_compile_features(${CORELIB_NAME} INTERFACE cxx_std_11)
target_include_directories(${CORELIB_NAME} INTERFACE .  ../../ArduinoJson/src  ../../ArduinoCore-host/api)
set_target_properties(${CORELIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
