# ESP-IDF component registration for GridShield main application
# This file registers all source files and include directories for the main component.
# Supports two build modes:
#   1. QEMU Simulation (GS_QEMU_BUILD) — uses mock platform, no WiFi
#   2. Demo Mode (real ESP32) — uses WiFi + HTTP to send data to backend

# Collect all source files from src/ subdirectories
file(GLOB_RECURSE SRC_FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
)

# Entry points: QEMU simulation + Demo firmware
set(ENTRY_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/app_main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/demo_main.cpp"
)

# Combine all sources (including micro-ecc C library)
set(ALL_SOURCES
    ${ENTRY_SOURCES}
    ${SRC_FILES}
    "${CMAKE_CURRENT_SOURCE_DIR}/../lib/micro-ecc/uECC.c"
)

# Register the component with ESP-IDF
# Both QEMU and Demo modes need all deps; unused ones are linked but harmless
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
        mbedtls
        nvs_flash
        esp_hw_support
        esp_wifi
        esp_http_client
        esp_event
        esp_netif
        json
        lwip
)

# Build mode selection:
# If GRIDSHIELD_DEMO_MODE is ON → Demo firmware (demo_main.cpp, no GS_QEMU_BUILD)
# If GRIDSHIELD_DEMO_MODE is OFF → QEMU simulation (app_main.cpp, GS_QEMU_BUILD=1)
if(NOT CONFIG_GRIDSHIELD_DEMO_MODE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE
        GS_QEMU_BUILD=1
    )
endif()

# Apply C++17 standard
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=gnu++17
    -fno-exceptions
    -fno-rtti
)
