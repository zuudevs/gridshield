cmake_minimum_required(VERSION 3.20)

project(
    GridShield
    VERSION 1.0.0
    DESCRIPTION "Multi-layer Security for AMI Systems"
    LANGUAGES CXX C
)

# ============================================================================
# BUILD OPTIONS
# ============================================================================
option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_EXAMPLES "Build examples" OFF)
option(ENABLE_SANITIZERS "Enable sanitizers (Native only)" OFF)

# Platform selection
if(NOT DEFINED PLATFORM)
    set(PLATFORM "NATIVE" CACHE STRING "Build platform: NATIVE or ARDUINO")
endif()

set_property(CACHE PLATFORM PROPERTY STRINGS "NATIVE" "ARDUINO")

message(STATUS "════════════════════════════════════════════")
message(STATUS " GridShield v${PROJECT_VERSION}")
message(STATUS " Platform: ${PLATFORM}")
message(STATUS " Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "════════════════════════════════════════════")

# ============================================================================
# C++ STANDARD
# ============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ============================================================================
# GLOBAL INCLUDE DIRECTORIES
# ============================================================================
# 1. Folder include utama proyek
include_directories(include)

# 2. Folder library eksternal (Project-Local)
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/micro-ecc")
    include_directories(libs/micro-ecc)
    message(STATUS "Library: micro-ecc found.")
else()
    message(WARNING "Library: micro-ecc NOT found in libs/micro-ecc. Crypto might fail.")
endif()

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${PLATFORM}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/${PLATFORM}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/libs/${PLATFORM}")

# ============================================================================
# COMMON SOURCES
# ============================================================================
set(COMMON_SOURCES
    src/common/security/crypto.cpp
    src/common/hardware/tamper.cpp
    src/common/network/packet.cpp
    src/common/analytics/detector.cpp
    src/common/core/system.cpp
    src/platform/platform.cpp
)

# Tambahkan source library eksternal jika ada
if(EXISTS "${CMAKE_SOURCE_DIR}/libs/micro-ecc/uECC.c")
    list(APPEND COMMON_SOURCES "libs/micro-ecc/uECC.c")
endif()

set(COMMON_HEADERS
    include/common/utils/gs_macros.hpp
    include/common/core/error.hpp
    include/common/core/types.hpp
    include/common/core/system.hpp
    include/common/security/crypto.hpp
    include/common/hardware/tamper.hpp
    include/common/network/packet.hpp
    include/common/analytics/detector.hpp
    include/platform/platform.hpp
)

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================================================
if(PLATFORM STREQUAL "NATIVE")
    message(STATUS "Configuring for Native platform")

    # --- OPENSSL CONFIGURATION (PRECISE MANUAL LINKING) ---
    # Path dasar instalasi
    set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")
    set(OPENSSL_INCLUDE_DIR "${OPENSSL_ROOT_DIR}/include")
    
    # Path spesifik library berdasarkan info dari Rara
    # "C:\Program Files\OpenSSL-Win64\lib\VC\x64\MD\libssl.lib"
    set(OPENSSL_LIB_DIR "${OPENSSL_ROOT_DIR}/lib/VC/x64/MD")

    # Kita set variabel library secara eksplisit
    set(OPENSSL_CRYPTO_LIB "${OPENSSL_LIB_DIR}/libcrypto.lib")
    set(OPENSSL_SSL_LIB "${OPENSSL_LIB_DIR}/libssl.lib")

    # Verifikasi keberadaan file
    if(EXISTS "${OPENSSL_CRYPTO_LIB}" AND EXISTS "${OPENSSL_SSL_LIB}")
        message(STATUS "OpenSSL Include: ${OPENSSL_INCLUDE_DIR}")
        message(STATUS "OpenSSL Crypto Lib: ${OPENSSL_CRYPTO_LIB}")
        message(STATUS "OpenSSL SSL Lib: ${OPENSSL_SSL_LIB}")

        include_directories(${OPENSSL_INCLUDE_DIR})
        # Gabungkan untuk linking
        set(OPENSSL_LIBRARIES "${OPENSSL_SSL_LIB}" "${OPENSSL_CRYPTO_LIB}")
        
        # Tambahkan definisi preprocessor agar header OpenSSL tidak bingung
        add_definitions(-DOPENSSL_API_COMPAT=10100) 
    else()
        message(FATAL_ERROR "OpenSSL libraries not found at ${OPENSSL_LIB_DIR}. Please check the path carefully!")
    endif()
    # -------------------------------------------
    
    # Main executable
    add_executable(${PROJECT_NAME} 
        ${COMMON_SOURCES}
        src/native/main.cpp
    )
    
    # Include directories (Target specific)
    target_include_directories(${PROJECT_NAME} PRIVATE
        include/common
        include/platform
        include/native
    )

    # Link OpenSSL libraries (Pakai variabel manual tadi)
    target_link_libraries(${PROJECT_NAME} ${OPENSSL_LIBRARIES} Ws2_32 Crypt32)
    # Catatan: Ws2_32 dan Crypt32 sering dibutuhkan OpenSSL di Windows
    
    # Compiler warnings
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE 
            /W4 /utf-8 /permissive-
        )
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow 
            -Wconversion -Wnon-virtual-dtor
        )
    endif()
    
    # Sanitizers (Debug only)
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(NOT MSVC)
            message(STATUS "Enabling sanitizers...")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
    
elseif(PLATFORM STREQUAL "ARDUINO")
    message(STATUS "Configuring for Arduino platform")
    message(WARNING "Use arduino-cli for production builds:")
    
    message(STATUS "  arduino-cli compile --libraries ./libs --fqbn arduino:avr:mega src/arduino/gridshield.ino")
    message(STATUS "  arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino")
    
else()
    message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}. Use NATIVE or ARDUINO.")
endif()

# ============================================================================
# INSTALL RULES
# ============================================================================
if(PLATFORM STREQUAL "NATIVE")
    install(TARGETS ${PROJECT_NAME}
        RUNTIME DESTINATION bin
    )
    
    install(DIRECTORY include/common/
        DESTINATION include/gridshield
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()

# ============================================================================
# SUMMARY
# ============================================================================
message(STATUS "────────────────────────────────────────────")
message(STATUS " Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS " Tests: ${BUILD_TESTS}")
message(STATUS " Examples: ${BUILD_EXAMPLES}")
message(STATUS "════════════════════════════════════════════")