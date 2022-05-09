cmake_minimum_required(VERSION 3.8.0)
project(${CORELIB_NAME} VERSION 0.1.0)

message("CORELIB_NAME = ${CORELIB_NAME}")

add_library(${CORELIB_NAME} INTERFACE 
    Ardulingua.h
    rdl/Common.h
    rdl/Delegate.h 
    rdl/JsonDelegate.h 
    rdl/JsonProtocol.h 
    rdl/JsonClient.h
    rdl/JsonServer.h
    rdl/JsonError.h
    rdl/Logger.h 
    rdl/ServerProperty.h
    rdl/SlipInPlace.h 
    rdl/std_type_traits.h
    rdl/std_utility.h
    rdl/sys_StringT.h
    rdl/sys_timing.h
    rdl/Polyfills/Print_Mock.h
    rdl/Polyfills/Print_MockHelp.h
    rdl/Polyfills/Print_MockIOS.h
    rdl/Polyfills/Printable_Mock.h
    rdl/Polyfills/Stream_Mock.h
    rdl/Polyfills/Stream_MockIOS.h
)
target_compile_features(${CORELIB_NAME} INTERFACE cxx_std_11)
target_include_directories(${CORELIB_NAME} INTERFACE .  ../../ArduinoJson/src  ../../ArduinoCore-host/api)
set_target_properties(${CORELIB_NAME} PROPERTIES LINKER_LANGUAGE CXX)
