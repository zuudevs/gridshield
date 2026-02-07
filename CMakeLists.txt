cmake_minimum_required(VERSION 3.23)

project(
    GridShield
    VERSION 1.1.0
    DESCRIPTION "Multi-layer Security for AMI Systems"
    LANGUAGES CXX C
)

# ============================================================================
# OPTIONS & CONFIGURATION
# ============================================================================

option(ENABLE_SANITIZERS "Enable AddressSanitizer and UBSan (Native only)" OFF)

# Default platform if not set by preset
if(NOT DEFINED PLATFORM)
    set(PLATFORM "NATIVE" CACHE STRING "Build platform: NATIVE, AVR, ESP32")
endif()

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

# Output directory based on platform
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY 
    "${CMAKE_SOURCE_DIR}/bin/${PLATFORM}"
)

# ============================================================================
# SOURCE FILES (Platform-agnostic)
# ============================================================================

set(CORE_SOURCES
    src/core/system.cpp
    src/hardware/tamper.cpp
    src/security/crypto.cpp
    src/network/packet.cpp
    src/analytics/detector.cpp
    src/platform/platform.cpp
)

set(HEADERS
    include/utils/gs_macros.hpp
    include/core/error.hpp
    include/core/types.hpp
    include/core/system.hpp
    include/platform/platform.hpp
    include/hardware/tamper.hpp
    include/security/crypto.hpp
    include/network/packet.hpp
    include/analytics/detector.hpp
)

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================================================

if(PLATFORM STREQUAL "AVR")
    # ────────────────────────────────────────────────────────────────────────
    # AVR (Arduino)
    # ────────────────────────────────────────────────────────────────────────
    
    message(STATUS "Configuring for AVR (${AVR_MCU})...")
    
    # Main executable
    add_executable(${PROJECT_NAME} ${CORE_SOURCES} src/main.cpp)
    
    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ${ARDUINO_CORE_PATH}
        ${ARDUINO_VARIANT_PATH}
    )
    
    # Arduino core library
    add_library(arduino_core STATIC
        ${ARDUINO_CORE_PATH}/wiring.c
        ${ARDUINO_CORE_PATH}/wiring_digital.c
        ${ARDUINO_CORE_PATH}/wiring_analog.c
        ${ARDUINO_CORE_PATH}/hooks.c
        ${ARDUINO_CORE_PATH}/WInterrupts.c
        ${ARDUINO_CORE_PATH}/CDC.cpp
        ${ARDUINO_CORE_PATH}/HardwareSerial.cpp
        ${ARDUINO_CORE_PATH}/Print.cpp
        ${ARDUINO_CORE_PATH}/Stream.cpp
        ${ARDUINO_CORE_PATH}/WMath.cpp
        ${ARDUINO_CORE_PATH}/WString.cpp
        ${ARDUINO_CORE_PATH}/abi.cpp
        ${ARDUINO_CORE_PATH}/main.cpp
    )
    
    target_include_directories(arduino_core PUBLIC
        ${ARDUINO_CORE_PATH}
        ${ARDUINO_VARIANT_PATH}
    )
    
    target_link_libraries(${PROJECT_NAME} PRIVATE arduino_core)
    
    # Generate HEX file
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom
                $<TARGET_FILE:${PROJECT_NAME}>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex
        COMMENT "Generating HEX file..."
        VERBATIM
    )
    
    # Memory usage report
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${AVR_SIZE} --format=avr --mcu=${AVR_MCU} 
                $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Memory usage:"
        VERBATIM
    )
    
    # Upload target (usage: cmake --build build-avr --target upload)
    if(DEFINED AVR_UPLOAD_PORT)
        avr_add_upload_target(${PROJECT_NAME} ${AVR_UPLOAD_PORT})
    else()
        message(STATUS "Upload target disabled. Set AVR_UPLOAD_PORT to enable.")
    endif()

elseif(PLATFORM STREQUAL "ESP32")
    # ────────────────────────────────────────────────────────────────────────
    # ESP32 (Future support)
    # ────────────────────────────────────────────────────────────────────────
    
    message(FATAL_ERROR "ESP32 support coming soon. Use PlatformIO for now.")

elseif(PLATFORM STREQUAL "NATIVE")
    # ────────────────────────────────────────────────────────────────────────
    # NATIVE (PC - Development/Testing)
    # ────────────────────────────────────────────────────────────────────────
    
    message(STATUS "Configuring for Native (${CMAKE_SYSTEM_NAME})...")
    
    # Add demo main for native
    add_executable(${PROJECT_NAME} ${CORE_SOURCES} src/main.cpp)
    
    target_include_directories(${PROJECT_NAME} PRIVATE include)
    
    # Compiler warnings
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE 
            /W4 /utf-8 /permissive-
        )
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow -Wconversion
        )
    endif()
    
    # Sanitizers (Debug only)
    if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
        message(STATUS "Enabling sanitizers...")
        target_compile_options(${PROJECT_NAME} PRIVATE
            -fsanitize=address,undefined
            -fno-omit-frame-pointer
        )
        target_link_options(${PROJECT_NAME} PRIVATE
            -fsanitize=address,undefined
        )
    endif()
    
else()
    message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}. Use NATIVE, AVR, or ESP32.")
endif()

# ============================================================================
# INSTALL RULES
# ============================================================================

if(PLATFORM STREQUAL "AVR")
    install(FILES 
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex
        DESTINATION firmware
    )
endif()

# ============================================================================
# SUMMARY
# ============================================================================

message(STATUS "────────────────────────────────────────────")
message(STATUS " Output: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
if(PLATFORM STREQUAL "AVR")
    message(STATUS " HEX: ${PROJECT_NAME}.hex")
    message(STATUS " Upload: cmake --build . --target upload")
endif()
message(STATUS "════════════════════════════════════════════")