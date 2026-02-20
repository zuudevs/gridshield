/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Production Arduino application
 * @version 1.0
 * @date 2026-02-11
 * 
 * Hardware:
 * - Arduino Mega 2560
 * - Tamper switch: Digital Pin 2
 * - Serial: 115200 baud
 * 
 * Libraries Required:
 * - Crypto (arduino-cli lib install Crypto)
 * - micro-ecc (manual install to libs/)
 * 
 * @copyright Copyright (c) 2026
 */

#if GS_PLATFORM_ARDUINO

#include "core/system.hpp"
#include "platform_arduino.hpp"

using namespace gridshield;

static platform::arduino::ArduinoTime arduino_time;
static platform::arduino::ArduinoGPIO arduino_gpio;
static platform::arduino::ArduinoInterrupt arduino_interrupt;
static platform::arduino::ArduinoCrypto arduino_crypto;
static platform::arduino::ArduinoSerial arduino_serial;

static platform::PlatformServices services;
static GridShieldSystem* system_ptr = nullptr;

static SystemConfig create_config() {
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = 5000;
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1200;
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;
    
    return config;
}

void setup() {
    services.time = &arduino_time;
    services.gpio = &arduino_gpio;
    services.interrupt = &arduino_interrupt;
    services.crypto = &arduino_crypto;
    services.storage = nullptr;
    services.comm = &arduino_serial;
    
    auto init_result = arduino_serial.init();
    if (init_result.is_error()) {
        pinMode(LED_BUILTIN, OUTPUT);
        while (true) {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(100);
            digitalWrite(LED_BUILTIN, LOW);
            delay(100);
        }
    }
    
    Serial.println(F("GridShield v1.0"));
    
    system_ptr = new GridShieldSystem();
    if (system_ptr == nullptr) {
        Serial.println(F("FATAL: OOM"));
        while (true);
    }
    
    SystemConfig config = create_config();
    auto result = system_ptr->initialize(config, services);
    if (result.is_error()) {
        Serial.println(F("FATAL: Init failed"));
        while (true);
    }
    
    result = system_ptr->start();
    if (result.is_error()) {
        Serial.println(F("FATAL: Start failed"));
        while (true);
    }
    
    Serial.println(F("Running"));
}

void loop() {
    if (system_ptr == nullptr) {
        return;
    }
    
    auto result = system_ptr->process_cycle();
    if (result.is_error()) {
        Serial.print(F("ERROR: "));
        Serial.println(static_cast<int>(result.error().code));
    }
    
    delay(100);
}

#endif // GS_PLATFORM_ARDUINO