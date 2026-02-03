cmake_minimum_required(VERSION 3.20)
project(
    gridshield
    VERSION 1.0.0
    DESCRIPTION "Multi-layer Security Model for Protection of AMI (Advanced Metering Infrastructure) Systems"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Compiler warnings and optimizations
if(MSVC)
    add_compile_options(
        /W4 
        /permissive-
        /utf-8
    )
else()
    add_compile_options(
        -Wall 
        -Wextra 
        -Wpedantic 
        -Wconversion 
        -Wshadow
        -Wno-unused-parameter
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        add_compile_options(/O2)
    else()
        add_compile_options(-O3)
    endif()
endif()

# Source files
set(GRIDSHIELD_SOURCES
    src/main.cpp
    src/core/system.cpp
    src/hardware/tamper.cpp
    src/security/crypto.cpp
    src/network/packet.cpp
    src/analytics/detector.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${GRIDSHIELD_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
)

# Installation rules
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# Print build information
message(STATUS "")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "  Project: ${PROJECT_NAME}")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "═══════════════════════════════════════════════════════════")
message(STATUS "")