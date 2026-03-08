# Test application main component
# Links to GridShield sources + Unity test framework

# Collect GridShield implementation sources (reuse from main project)
set(GS_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../main/src")
set(GS_LIB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../lib")

file(GLOB_RECURSE GS_SRC_FILES
    "${GS_SRC_DIR}/*.cpp"
    "${GS_SRC_DIR}/*.c"
)

# Test source files
file(GLOB TEST_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

# Combine all sources
set(ALL_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp"
    ${TEST_FILES}
    ${GS_SRC_FILES}
    "${GS_LIB_DIR}/micro-ecc/uECC.c"
)

idf_component_register(
    SRCS ${ALL_SOURCES}
    INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../include/platform"
        "${GS_LIB_DIR}/micro-ecc"
    REQUIRES
        unity
        driver
        esp_timer
        mbedtls
        nvs_flash
        esp_hw_support
)

# QEMU build flag + test flag
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    GS_QEMU_BUILD=1
    GS_TEST_BUILD=1
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
)
