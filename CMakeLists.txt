cmake_minimum_required(VERSION 3.21)

project(micoring VERSION 0.1.0 LANGUAGES C CXX)

include(CMakePackageConfigHelpers)
include(CTest)

set(MRING_CONCURRENCY_SINGLE_CONTEXT 1)
set(MRING_CONCURRENCY_SPSC_ATOMIC 2)

set(MRING_CONCURRENCY_MODE "single-context" CACHE STRING "micoring concurrency mode")
set_property(CACHE MRING_CONCURRENCY_MODE PROPERTY STRINGS "single-context" "spsc-atomic")

option(MRING_BUILD_TESTS "Build micoring tests." ON)
option(MRING_STRICT_WARNINGS "Enable strict warnings for micoring-owned targets." ON)

if(MRING_CONCURRENCY_MODE STREQUAL "single-context")
    set(MRING_CONFIG_CONCURRENCY_MODE ${MRING_CONCURRENCY_SINGLE_CONTEXT})
elseif(MRING_CONCURRENCY_MODE STREQUAL "spsc-atomic")
    set(MRING_CONFIG_CONCURRENCY_MODE ${MRING_CONCURRENCY_SPSC_ATOMIC})
else()
    message(FATAL_ERROR "MRING_CONCURRENCY_MODE must be 'single-context' or 'spsc-atomic'")
endif()

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/mring_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/include/mring_config.h
    @ONLY)

add_library(micoring src/mring.c)
add_library(micoring::micoring ALIAS micoring)

target_compile_features(micoring PUBLIC c_std_99)
target_include_directories(micoring
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated/include>
        $<INSTALL_INTERFACE:include>)

if(MRING_STRICT_WARNINGS)
    if(MSVC)
        target_compile_options(micoring PRIVATE /W4 /WX)
    else()
        target_compile_options(micoring PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Werror)
    endif()
endif()

install(TARGETS micoring EXPORT micoringTargets)
install(FILES include/mring.h ${CMAKE_CURRENT_BINARY_DIR}/generated/include/mring_config.h DESTINATION include)
install(EXPORT micoringTargets FILE micoringTargets.cmake NAMESPACE micoring:: DESTINATION lib/cmake/micoring)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/micoringConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/micoringConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/micoringConfig.cmake
    INSTALL_DESTINATION lib/cmake/micoring)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/micoringConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/micoringConfigVersion.cmake
    DESTINATION lib/cmake/micoring)

if(MRING_BUILD_TESTS)
    add_subdirectory(tests)
endif()
