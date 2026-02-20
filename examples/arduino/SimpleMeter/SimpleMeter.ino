/**
 * @file SimpleMeter.ino
 * @brief Basic example of GridShield Usage
 * 
 * Demonstrates:
 * 1. Initializing the GridShield system
 * 2. Simulating meter readings
 * 3. Sending secure packets
 * 
 * Hardware:
 * - Arduino Mega 2560
 * - LED on Pin 13 (Built-in)
 */

#include <GridShield.h>

// Platform implementations
#include <arduino/platform_arduino.hpp>

using namespace gridshield;

// Global instances
static platform::arduino::ArduinoTime arduino_time;
static platform::arduino::ArduinoGPIO arduino_gpio;
static platform::arduino::ArduinoInterrupt arduino_interrupt;
static platform::arduino::ArduinoCrypto arduino_crypto;
static platform::arduino::ArduinoStorage arduino_storage;
static platform::arduino::ArduinoSerial arduino_serial;

static platform::PlatformServices services;
static GridShieldSystem* system_ptr = nullptr;

void setup() {
    // 1. Setup Platform Services
    services.time = &arduino_time;
    services.gpio = &arduino_gpio;
    services.interrupt = &arduino_interrupt;
    services.crypto = &arduino_crypto;
    services.storage = &arduino_storage;
    services.comm = &arduino_serial;
    
    // Initialize Serial first for debug output
    arduino_serial.init();
    Serial.println(F("SimpleMeter Example Starting..."));

    // 2. Configure System
    SystemConfig config;
    config.meter_id = 0x1122334455667788;
    config.heartbeat_interval_ms = 10000; // 10 seconds
    config.reading_interval_ms = 5000;    // 5 seconds
    
    // Tamper on Pin 2
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    // 3. Initialize GridShield
    system_ptr = new GridShieldSystem();
    auto result = system_ptr->initialize(config, services);
    
    if (result.is_error()) {
        Serial.print(F("Init Failed: "));
        Serial.println((int)result.error().code);
        while(1);
    }
    
    // 4. Start System
    result = system_ptr->start();
    if (result.is_error()) {
        Serial.println(F("Start Failed"));
        while(1);
    }
    
    Serial.println(F("GridShield Initialized."));
}

void loop() {
    // 5. Process Security Cycle (Heartbeats, Tamper check, etc.)
    if (system_ptr) {
        system_ptr->process_cycle();
    }
    
    // 6. Simulate consumption reading every 5 seconds
    static unsigned long last_reading = 0;
    if (millis() - last_reading > 5000) {
        last_reading = millis();
        
        core::MeterReading reading;
        reading.timestamp = millis();
        reading.energy_wh = 12345; // Simulated value
        reading.voltage_mv = 220000;
        reading.current_ma = 5000;
        
        Serial.println(F("Sending Reading..."));
        system_ptr->send_meter_reading(reading);
    }
    
    delay(10);
}
