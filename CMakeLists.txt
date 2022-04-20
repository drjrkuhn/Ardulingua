cmake_minimum_required(VERSION 3.8.0)
project(SlipInPlace VERSION 0.1.0)

set(CORELIB_NAME "libArdulingua")

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

add_subdirectory(src)
add_subdirectory(tests)

