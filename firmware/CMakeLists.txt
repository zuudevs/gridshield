# GridShield Firmware — ESP-IDF + QEMU
cmake_minimum_required(VERSION 3.20)

# ESP-IDF build system (MUST be before project())
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Project definition
project(
    GRIDSHEILD
    VERSION 1.0.1
    DESCRIPTION "Multi-Layer Security Framework for Advanced Metering Infrastructure (AMI)"
    LANGUAGES CXX C
)

# C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)