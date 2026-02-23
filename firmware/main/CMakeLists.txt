# ESP-IDF component registration for GridShield main application
# This file registers all source files and include directories for the main component.

# Collect all source files from src/ subdirectories
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# ESP-IDF entry point
set(ENTRY_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/app_main.cpp")

# Combine all sources (including micro-ecc C library)
set(ALL_SOURCES
    ${ENTRY_SOURCE}
    ${SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/micro-ecc/uECC.c"
)

# Register the component with ESP-IDF
idf_component_register(
    SRCS ${ALL_SOURCES}
    INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/../include"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/common"
        "${CMAKE_CURRENT_SOURCE_DIR}/../include/platform"
        "${CMAKE_CURRENT_SOURCE_DIR}/../lib/micro-ecc"
    REQUIRES
        driver
        esp_timer
)

# Mark this as a QEMU build (disables Arduino-dependent code)
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    GS_QEMU_BUILD=1
)

# Apply C++17 standard
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
)
