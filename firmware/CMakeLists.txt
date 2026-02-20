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
include_directories(include)

if(EXISTS "${CMAKE_SOURCE_DIR}/libs/micro-ecc")
    include_directories(libs/micro-ecc)
    message(STATUS "Library: micro-ecc found")
else()
    message(WARNING "Library: micro-ecc NOT found in libs/micro-ecc")
endif()

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

# Add micro-ecc if available
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

    # Crypto Library Configuration (Embedded)
    set(CRYPTO_LIB_DIR "${CMAKE_SOURCE_DIR}/.pio/libdeps/native/Crypto")
    
    if(EXISTS "${CRYPTO_LIB_DIR}")
        message(STATUS "Crypto Library: ${CRYPTO_LIB_DIR}")
        include_directories("${CRYPTO_LIB_DIR}")
        
        # Also include src/ subdirectory if it exists (Arduino library format)
        if(EXISTS "${CRYPTO_LIB_DIR}/src")
            include_directories("${CRYPTO_LIB_DIR}/src")
        endif()
        
        # Add Crypto source files
        file(GLOB CRYPTO_SOURCES "${CRYPTO_LIB_DIR}/*.cpp" "${CRYPTO_LIB_DIR}/src/*.cpp")
        list(APPEND COMMON_SOURCES ${CRYPTO_SOURCES})
        
        add_definitions(-DGS_PLATFORM_NATIVE=1)
    else()
        message(FATAL_ERROR "Crypto library not found at ${CRYPTO_LIB_DIR}. Run 'platformio lib install' first.")
    endif()
    
    # Main executable (production)
    add_executable(${PROJECT_NAME} 
        ${COMMON_SOURCES}
        src/native/main.cpp
    )
    
    target_include_directories(${PROJECT_NAME} PRIVATE
        include/common
        include/platform
        include/native
    )

    # Link standard libraries (no OpenSSL)
    if(WIN32)
        target_link_libraries(${PROJECT_NAME} Ws2_32)
    endif()
    
    # Compiler warnings
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8 /permissive-)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow 
            -Wconversion -Wnon-virtual-dtor
        )
    endif()
    
    # Sanitizers (Debug only)
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        if(NOT MSVC)
            message(STATUS "Enabling sanitizers")
            target_compile_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
                -fno-omit-frame-pointer
            )
            target_link_options(${PROJECT_NAME} PRIVATE
                -fsanitize=address,undefined
            )
        endif()
    endif()
    
    # Build examples if requested
    if(BUILD_EXAMPLES)
        message(STATUS "Building examples")
        
        add_executable(demo_native 
            examples/native/demo_native.cpp
            ${COMMON_SOURCES}
        )
        
        target_include_directories(demo_native PRIVATE
            include/common
            include/platform
            include/native
            "${CRYPTO_LIB_DIR}"
            "${CRYPTO_LIB_DIR}/src"
        )
        
        target_compile_definitions(demo_native PRIVATE GS_PLATFORM_NATIVE=1)
        
        if(WIN32)
            target_link_libraries(demo_native Ws2_32)
        endif()
        
        set_target_properties(demo_native PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${PLATFORM}"
        )
    endif()

    # Build tests if requested
    if(BUILD_TESTS)
        message(STATUS "Configuring Test Suite")
        
        set(UNITY_LIB_DIR "${CMAKE_SOURCE_DIR}/.pio/libdeps/native/Unity/src")
        if(EXISTS "${UNITY_LIB_DIR}/unity.c")
            include_directories("${UNITY_LIB_DIR}")
            
            add_executable(gridshield_test
                test/native/test_security_hardening.cpp
                test/native/test_key_storage.cpp
                "${UNITY_LIB_DIR}/unity.c"
                ${COMMON_SOURCES}
            )
            
            target_compile_definitions(gridshield_test PRIVATE GS_PLATFORM_NATIVE=1)
            
            target_include_directories(gridshield_test PRIVATE
                include/common
                include/platform
                include/native
                "${CRYPTO_LIB_DIR}"
                "${CRYPTO_LIB_DIR}/src"
                "${UNITY_LIB_DIR}"
            )
            
            if(WIN32)
                target_link_libraries(gridshield_test Ws2_32)
            endif()
            
            set_target_properties(gridshield_test PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${PLATFORM}"
            )
            
            add_test(NAME gridshield_test COMMAND gridshield_test)
            
        else()
            message(WARNING "Unity library not found at ${UNITY_LIB_DIR}")
        endif()
    endif()

    enable_testing()
    
elseif(PLATFORM STREQUAL "ARDUINO")
    message(STATUS "Configuring for Arduino platform")
    message(WARNING "Use arduino-cli for production builds:")
    message(STATUS "  arduino-cli compile --fqbn arduino:avr:mega src/arduino/main.ino")
    message(STATUS "  arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/main.ino")
    
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