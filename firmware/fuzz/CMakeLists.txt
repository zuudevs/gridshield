# ============================================================================
# GridShield Fuzzing — Native Build with LibFuzzer + ASan + UBSan
# ============================================================================
#
# Prerequisites: clang (14+), cmake (3.20+)
#
# Build:
#   cmake -B build -S . \
#     -DCMAKE_C_COMPILER=clang \
#     -DCMAKE_CXX_COMPILER=clang++
#   cmake --build build
#
# Run:
#   ./build/fuzz_packet_parse -max_len=2048 -runs=1000000
#
# ============================================================================

cmake_minimum_required(VERSION 3.20)

project(
    gridshield_fuzz
    VERSION 1.0.0
    DESCRIPTION "GridShield Fuzz Tests"
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

# ============================================================================
# GridShield Sources (same as test_app — all production code)
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

# micro-ecc library
set(UECC_SOURCES ${GS_LIB_DIR}/micro-ecc/uECC.c)

# ============================================================================
# Fuzz Target: fuzz_packet_parse
# ============================================================================
add_executable(fuzz_packet_parse
    fuzz_packet_parse.cpp
    ${GS_SOURCES}
    ${UECC_SOURCES}
)

target_include_directories(fuzz_packet_parse PRIVATE
    ${GS_INCLUDE_DIR}
    ${GS_INCLUDE_DIR}/common
    ${GS_INCLUDE_DIR}/platform
    ${GS_LIB_DIR}/micro-ecc
)

# Native platform build flags
target_compile_definitions(fuzz_packet_parse PRIVATE
    GS_PLATFORM_NATIVE=1
)

# LibFuzzer + ASan + UBSan
target_compile_options(fuzz_packet_parse PRIVATE
    -fsanitize=fuzzer,address,undefined
    -fno-omit-frame-pointer
    -g
)

target_link_options(fuzz_packet_parse PRIVATE
    -fsanitize=fuzzer,address,undefined
)
