# GridShield Test Application — ESP-IDF + Unity
cmake_minimum_required(VERSION 3.20)

# ESP-IDF build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

project(
    gridshield_tests
    VERSION 1.0.0
    DESCRIPTION "GridShield Unit Tests"
    LANGUAGES CXX C
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
