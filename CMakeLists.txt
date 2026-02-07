cmake_minimum_required(VERSION 3.20)

project(
    GRIDSHIELD
    VERSION 1.0.0
    DESCRIPTION "Multi-layer Security Model for AMI Systems"
    LANGUAGES CXX C
)

if(NOT DEFINED PLATFORM)
    set(PLATFORM "NATIVE")
endif()

message(STATUS "═══════════════════════════════════")
message(STATUS " Target Platform: ${PLATFORM}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${PLATFORM})

add_executable(${PROJECT_NAME} 
    src/main.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    "${CMAKE_SOURCE_DIR}/include"
)

if(PLATFORM STREQUAL "AVR")
    set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".elf")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex -R .eeprom 
                $<TARGET_FILE:${PROJECT_NAME}> 
                ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${PROJECT_NAME}.hex
        COMMENT "Generating HEX file for Arduino..."
    )

    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_SIZE} -C --mcu=atmega328p $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Checking Memory Usage..."
    )

elseif(PLATFORM STREQUAL "NATIVE")
    if(MSVC)
        target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            target_compile_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
            target_link_options(${PROJECT_NAME} PRIVATE -fsanitize=address)
        endif()
    endif()

endif()

message(STATUS " Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS " Output Dir: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "═══════════════════════════════════")