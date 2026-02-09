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
option(BUILD_TESTS "Build unit tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
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

# Output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${PLATFORM}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${PLATFORM}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib/${PLATFORM}")

# ============================================================================
# COMMON SOURCES
# ============================================================================
set(COMMON_SOURCES
    src/common/security/crypto.cpp
    src/common/hardware/tamper.cpp
    src/common/network/packet.cpp
    src/common/analytics/detector.cpp
    src/common/core/system.cpp
)

set(COMMON_HEADERS
    include/gridshield/common/utils/gs_macros.hpp
    include/gridshield/common/core/error.hpp
    include/gridshield/common/core/types.hpp
    include/gridshield/common/core/system.hpp
    include/gridshield/common/security/crypto.hpp
    include/gridshield/common/hardware/tamper.hpp
    include/gridshield/common/network/packet.hpp
    include/gridshield/common/analytics/detector.hpp
    include/gridshield/platform/platform.hpp
)

# ============================================================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================================================
if(PLATFORM STREQUAL "NATIVE")
    # ────────────────────────────────────────────────────────────────────────
    # NATIVE (PC) BUILD
    # ────────────────────────────────────────────────────────────────────────
    message(STATUS "Configuring for Native platform")
    
    # Main executable
    add_executable(${PROJECT_NAME} 
        ${COMMON_SOURCES}
        src/native/main.cpp
    )
    
    # Include directories
    target_include_directories(${PROJECT_NAME} PRIVATE
        include/gridshield/common
        include/gridshield/platform
        include/gridshield/native
    )
    
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
    
    # Tests
    if(BUILD_TESTS)
        enable_testing()
        add_subdirectory(test/native)
    endif()
    
    # Examples
    if(BUILD_EXAMPLES)
        add_subdirectory(examples/native)
    endif()
    
elseif(PLATFORM STREQUAL "ARDUINO")
    # ────────────────────────────────────────────────────────────────────────
    # ARDUINO BUILD (via CMake)
    # ────────────────────────────────────────────────────────────────────────
    message(STATUS "Configuring for Arduino platform")
    message(WARNING "Arduino build via CMake is experimental. Use arduino-cli for production.")
    
    # This is a minimal setup. For production, use arduino-cli:
    # arduino-cli compile --fqbn arduino:avr:mega gridshield.ino
    
    message(STATUS "For Arduino builds, use:")
    message(STATUS "  arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino")
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
    
    install(DIRECTORY include/gridshield/
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