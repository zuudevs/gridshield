cmake_minimum_required(VERSION 3.20)

project(
    GridShield
    VERSION 1.1.0
    DESCRIPTION "Multi-layer Security for AMI Systems"
    LANGUAGES CXX C
)

# ============================================================================
# PLATFORM SELECTION
# ============================================================================

if(NOT DEFINED PLATFORM)
    set(PLATFORM "NATIVE" CACHE STRING "Build platform: NATIVE or AVR")
endif()

message(STATUS "═══════════════════════════════════")
message(STATUS " Platform: ${PLATFORM}")

# ============================================================================
# COMPILER SETTINGS
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM})

# ============================================================================
# SOURCE FILES
# ============================================================================

set(CORE_SOURCES
    src/core/system.cpp
    src/hardware/tamper.cpp
    src/security/crypto.cpp
    src/network/packet.cpp
    src/analytics/detector.cpp
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
# PLATFORM-SPECIFIC BUILD
# ============================================================================

if(PLATFORM STREQUAL "AVR")
    # ────────────────────────────────────────────────────────────────────────
    # AVR TOOLCHAIN (Arduino-compatible)
    # ────────────────────────────────────────────────────────────────────────
    
    set(CMAKE_SYSTEM_NAME Generic)
    set(CMAKE_SYSTEM_PROCESSOR avr)
    
    # AVR toolchain paths (adjust for your system)
    set(AVR_GCC_DIR "C:/Users/ASUS/AppData/Local/Arduino15/packages/arduino/tools/avr-gcc/7.3.0-atmel3.6.1-arduino7")
    
    set(CMAKE_C_COMPILER "${AVR_GCC_DIR}/bin/avr-gcc.exe")
    set(CMAKE_CXX_COMPILER "${AVR_GCC_DIR}/bin/avr-g++.exe")
    set(CMAKE_OBJCOPY "${AVR_GCC_DIR}/bin/avr-objcopy.exe")
    set(CMAKE_SIZE "${AVR_GCC_DIR}/bin/avr-size.exe")
    
    # MCU settings
    set(MCU "atmega328p")
    set(F_CPU "16000000UL")
    
    # AVR compiler flags
    add_compile_options(
        -mmcu=${MCU}
        -DF_CPU=${F_CPU}
        -DPLATFORM_AVR=1
        -Os
        -ffunction-sections
        -fdata-sections
        -flto
        -Wall
        -Wextra
    )
    
    # AVR linker flags
    add_link_options(
        -mmcu=${MCU}
        -Wl,--gc-sections
        -flto
    )
    
    # Arduino core sources (minimal - extend as needed)
    set(ARDUINO_CORE_DIR "${AVR_GCC_DIR}/../../../hardware/avr/1.8.7/cores/arduino")
    
    add_executable(${PROJECT_NAME}
        ${CORE_SOURCES}
        src/main.cpp
    )
    
    target_include_directories(${PROJECT_NAME} PRIVATE
        include
        ${ARDUINO_CORE_DIR}
    )
    
    # Generate HEX file for upload
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom
                $<TARGET_FILE:${PROJECT_NAME}>
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex
        COMMENT "Generating HEX file..."
    )
    
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} -C --mcu=${MCU} $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Memory usage:"
    )

elseif(PLATFORM STREQUAL "NATIVE")
    # ────────────────────────────────────────────────────────────────────────
    # NATIVE BUILD (x86/x64 for testing)
    # ────────────────────────────────────────────────────────────────────────
    
    add_executable(${PROJECT_NAME}
        ${CORE_SOURCES}
        # Native uses mock_platform instead of Arduino drivers
    )
    
    target_include_directories(${PROJECT_NAME} PRIVATE include)
    
    # Compiler warnings
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE
            -Wall -Wextra -Wpedantic -Wshadow
        )
    endif()
    
    # Sanitizers in debug mode
    if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND NOT MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
        target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address,undefined)
    endif()
    
else()
    message(FATAL_ERROR "Unknown PLATFORM: ${PLATFORM}. Use NATIVE or AVR.")
endif()

# ============================================================================
# OUTPUT
# ============================================================================

message(STATUS " Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS " Output Dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "═══════════════════════════════════")