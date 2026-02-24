# ============================================================================
# GridShield Code Coverage — Native GCC Build with gcov/lcov
# ============================================================================
#
# This builds the test_app sources natively (not for ESP32) to collect
# code coverage. It uses GCC's --coverage flag + lcov for HTML reports.
#
# Usage:  bash run_coverage.sh
#
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(
    gridshield_coverage
    VERSION 1.0.0
    DESCRIPTION "GridShield Coverage Build"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ============================================================================
# Paths
# ============================================================================
set(GS_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(GS_SRC_DIR "${GS_ROOT}/main/src")
set(GS_INCLUDE_DIR "${GS_ROOT}/include")
set(GS_LIB_DIR "${GS_ROOT}/lib")
set(GS_TEST_DIR "${GS_ROOT}/test_app/main")

# ============================================================================
# GridShield Production Sources
# ============================================================================
set(GS_SOURCES
    ${GS_SRC_DIR}/analytics/detector.cpp
    ${GS_SRC_DIR}/core/system.cpp
    ${GS_SRC_DIR}/hardware/tamper.cpp
    ${GS_SRC_DIR}/network/packet.cpp
    ${GS_SRC_DIR}/security/crypto.cpp
    ${GS_SRC_DIR}/security/hkdf.cpp
    ${GS_SRC_DIR}/platform/platform.cpp
)

# micro-ecc
set(UECC_SOURCES ${GS_LIB_DIR}/micro-ecc/uECC.c)

# ============================================================================
# Test Sources (reuse from test_app, excluding test_main.cpp)
# ============================================================================
file(GLOB TEST_FILES "${GS_TEST_DIR}/test_*.cpp")
list(FILTER TEST_FILES EXCLUDE REGEX "test_main\\.cpp$")

# ============================================================================
# Native test runner (replaces FreeRTOS-based test_main.cpp)
# ============================================================================
set(NATIVE_TEST_MAIN "${CMAKE_CURRENT_SOURCE_DIR}/coverage_main.cpp")

# ============================================================================
# Build
# ============================================================================
add_executable(gridshield_tests
    ${NATIVE_TEST_MAIN}
    ${TEST_FILES}
    ${GS_SOURCES}
    ${UECC_SOURCES}
)

target_include_directories(gridshield_tests PRIVATE
    ${GS_INCLUDE_DIR}
    ${GS_INCLUDE_DIR}/common
    ${GS_INCLUDE_DIR}/platform
    ${GS_LIB_DIR}/micro-ecc
    ${CMAKE_CURRENT_SOURCE_DIR}  # For unity.h shim
)

target_compile_definitions(gridshield_tests PRIVATE
    GS_PLATFORM_NATIVE=1
    GS_TEST_BUILD=1
)

# Coverage flags
target_compile_options(gridshield_tests PRIVATE
    --coverage
    -fprofile-arcs
    -ftest-coverage
    -g
    -O0
)

target_link_options(gridshield_tests PRIVATE
    --coverage
)
