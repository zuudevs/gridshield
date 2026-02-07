/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief GridShield Arduino firmware entry point
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#if PLATFORM_AVR

#include <Arduino.h>
#include "core/system.hpp"
#include "platform/arduino/gpio_arduino.hpp"
#include "platform/arduino/platform_arduino.hpp"

using namespace gridshield;

// ============================================================================
// GLOBAL STATE (Arduino requires globals for setup/loop pattern)
// ============================================================================

// Platform drivers
platform::arduino::ArduinoGPIO g_gpio;
platform::arduino::ArduinoTime g_time;
platform::arduino::ArduinoSerialComm g_comm;
platform::arduino::ArduinoSimpleCrypto g_crypto;
platform::arduino::ArduinoInterruptStub g_interrupt;
platform::arduino::ArduinoStorageStub g_storage;

// Platform services container
platform::PlatformServices g_services;

// Main system
GridShieldSystem* g_system = nullptr;

// ============================================================================
// SETUP (called once on boot)
// ============================================================================

void gridshield_setup() {
    // Initialize serial
    Serial.begin(115200);
    while (!Serial) { delay(10); }
    
    Serial.println(F("\n=== GridShield v1.0 ==="));
    Serial.println(F("Booting..."));
    
    // Wire platform services
    g_services.gpio = &g_gpio;
    g_services.time = &g_time;
    g_services.comm = &g_comm;
    g_services.crypto = &g_crypto;
    g_services.interrupt = &g_interrupt;
    g_services.storage = &g_storage;
    
    // System configuration
    SystemConfig config;
    config.meter_id = 0xEA320001;
    config.heartbeat_interval_ms = 30000;  // 30s
    config.reading_interval_ms = 10000;    // 10s
    
    // Tamper pin: GPIO0 (BOOT button on most ESP boards)
    config.tamper_config.sensor_pin = 0;
    config.tamper_config.debounce_ms = 50;
    
    // Allocate system
    g_system = new GridShieldSystem();
    if (g_system == nullptr) {
        Serial.println(F("FATAL: Memory allocation failed!"));
        while (1) { delay(1000); }
    }
    
    // Initialize system
    Serial.print(F("Initializing... "));
    auto init_result = g_system->initialize(config, g_services);
    
    if (init_result.is_error()) {
        Serial.print(F("FAILED! Code: "));
        Serial.println(static_cast<int>(init_result.error().code));
        while (1) { delay(1000); }
    }
    Serial.println(F("OK"));
    
    // Start system
    Serial.print(F("Starting... "));
    auto start_result = g_system->start();
    
    if (start_result.is_error()) {
        Serial.println(F("FAILED!"));
        while (1) { delay(1000); }
    }
    Serial.println(F("OK"));
    Serial.println(F("System running."));
}

// ============================================================================
// LOOP (called repeatedly)
// ============================================================================

void gridshield_loop() {
    if (g_system != nullptr) {
        auto result = g_system->process_cycle();
        
        // Log errors (non-blocking)
        if (result.is_error()) {
            Serial.print(F("Cycle error: "));
            Serial.println(static_cast<int>(result.error().code));
        }
    }
    
    delay(100); // Prevent watchdog timeout
}

#endif // PLATFORM_AVR