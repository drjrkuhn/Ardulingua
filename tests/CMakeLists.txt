cmake_minimum_required(VERSION 3.8.0)
project("test_${CORELIB_NAME}")

###
### Test SlipInPlace
###

set(SLIP_TEST_TARGET "${PROJECT_NAME}_slip")
set(SLIP_TEST_SRCS 
    slip/hrslip.h
    slip/main.cpp
    slip/test_encode.cpp
    slip/test_encode_null.cpp
    slip/test_encode_slip.cpp
    slip/test_decode.cpp
    slip/test_decode_null.cpp
    slip/test_decode_slip.cpp
    slip/test_sliputils.cpp
    )

add_executable(${SLIP_TEST_TARGET}  ${SLIP_TEST_SRCS})
target_compile_features(${SLIP_TEST_TARGET} PUBLIC cxx_std_11)
add_dependencies(${SLIP_TEST_TARGET}	${CORELIB_NAME})
target_link_libraries(${SLIP_TEST_TARGET} PRIVATE Catch2::Catch2 ${CORELIB_NAME})

set(DISPATCH_TEST_TARGET "${PROJECT_NAME}_dispatch")
set(DISPATCH_TEST_SRCS 
    dispatch/main.cpp
    dispatch/test_delegate.cpp
    )

add_executable(${DISPATCH_TEST_TARGET}  ${DISPATCH_TEST_SRCS})
target_compile_features(${DISPATCH_TEST_TARGET} PUBLIC cxx_std_11)
add_dependencies(${DISPATCH_TEST_TARGET}	${CORELIB_NAME})
target_link_libraries(${DISPATCH_TEST_TARGET} PRIVATE Catch2::Catch2 ${CORELIB_NAME})

catch_discover_tests(${SLIP_TEST_TARGET})
catch_discover_tests(${DISPATCH_TEST_TARGET})

set(ARDUINO_CORE_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/Common.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/IPAddress.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/Print.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/Printable.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/Stream.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api/WString.cpp
)

add_executable("devel1" devel1_main.cpp ${ARDUINO_CORE_SRCS})
target_compile_features("devel1" PUBLIC cxx_std_11)
target_include_directories("devel1" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api")
add_dependencies("devel1" ${CORELIB_NAME})
target_link_libraries("devel1" PRIVATE ${CORELIB_NAME})

add_executable("test_server" test_server.cpp ${ARDUINO_CORE_SRCS})
target_compile_features("test_server" PUBLIC cxx_std_11)
target_include_directories("test_server" PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../ArduinoCore-host/api")
add_dependencies("test_server" ${CORELIB_NAME})
target_link_libraries("test_server" PRIVATE ${CORELIB_NAME})


# add_executable("devel2" main_devel2.cpp hrslip.h)
# target_compile_features("devel2" PUBLIC cxx_std_11)
# add_dependencies("devel2" ${CORELIB_NAME})
# target_link_libraries("devel2" PRIVATE ${CORELIB_NAME})

# add_executable("samples" main_samples.cpp hrslip.h)
# target_compile_features("samples" PUBLIC cxx_std_11)
# add_dependencies("samples" ${CORELIB_NAME})
# target_link_libraries("samples" PRIVATE ${CORELIB_NAME})

