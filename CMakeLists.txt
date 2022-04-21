cmake_minimum_required(VERSION 3.8.0)
project(SlipInPlace VERSION 0.1.0)

set(CORELIB_NAME "libArdulingua")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

find_package(catch2 2 CONFIG REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

add_subdirectory(src)
add_subdirectory(tests)

