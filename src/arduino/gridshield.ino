/**
 * @file gridshield.ino
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Arduino entry point for GridShield
 * @version 0.0.4
 * @date 2026-02-09
 * 
 * Hardware Requirements:
 * - Arduino Mega 2560 (recommended) or Uno
 * - Tamper switch on digital pin 2
 * - Serial @ 115200 baud
 * 
 * Optional Libraries:
 * - Crypto by Rhys Weatherley (for real SHA256)
 * 
 * @copyright Copyright (c) 2026
 */

#include "core/system.hpp"
#include "platform_arduino.hpp"

using namespace gridshield;

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================
static platform::arduino::ArduinoTime arduino_time;
static platform::arduino::ArduinoGPIO arduino_gpio;
static platform::arduino::ArduinoInterrupt arduino_interrupt;
static platform::arduino::ArduinoCrypto arduino_crypto;
static platform::arduino::ArduinoSerial arduino_serial;

static platform::PlatformServices services;
static GridShieldSystem* system_ptr = nullptr;

// ============================================================================
// CONFIGURATION
// ============================================================================
static SystemConfig create_config() {
    SystemConfig config;
    
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;  // 1 minute
    config.reading_interval_ms = 5000;     // 5 seconds
    
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    // Initialize baseline profile
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1000 + (i * 50);
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;
    config.baseline_profile.profile_confidence = 80;
    
    return config;
}

// ============================================================================
// ARDUINO SETUP
// ============================================================================
void setup() {
    // Setup platform services
    services.time = &arduino_time;
    services.gpio = &arduino_gpio;
    services.interrupt = &arduino_interrupt;
    services.crypto = &arduino_crypto;
    services.storage = nullptr;
    services.comm = &arduino_serial;
    
    // Initialize serial
    auto init_result = arduino_serial.init();
    if (init_result.is_error()) {
        // Blink LED to indicate error
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    }
    
    Serial.println(F("=== GridShield v1.0 ==="));
    Serial.println(F("Booting..."));
    
    // Create system
    system_ptr = new GridShieldSystem();
    if (system_ptr == nullptr) {
        Serial.println(F("ERROR: Out of memory"));
        while (true);
    }
    
    // Initialize system
    SystemConfig config = create_config();
    auto result = system_ptr->initialize(config, services);
    if (result.is_error()) {
        Serial.print(F("ERROR: Init failed, code "));
        Serial.println(static_cast<int>(result.error().code));
        while (true);
    }
    Serial.println(F("Initializing... OK"));
    
    // Start system
    result = system_ptr->start();
    if (result.is_error()) {
        Serial.print(F("ERROR: Start failed, code "));
        Serial.println(static_cast<int>(result.error().code));
        while (true);
    }
    Serial.println(F("Starting... OK"));
    Serial.println(F("System running."));
}

// ============================================================================
// ARDUINO LOOP
// ============================================================================
void loop() {
    if (system_ptr == nullptr) {
        return;
    }
    
    // Process system cycle
    auto result = system_ptr->process_cycle();
    if (result.is_error()) {
        Serial.print(F("ERROR: Cycle failed, code "));
        Serial.println(static_cast<int>(result.error().code));
    }
    
    // Small delay to prevent CPU hogging
    delay(100);
}
