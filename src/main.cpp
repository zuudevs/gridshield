/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <Arduino.h>
#include "core/system.hpp"

// Include implementasi driver
#include "platform/arduino/gpio_arduino.hpp"
#include "platform/arduino/platform_arduino.hpp"

using namespace gridshield;

// --- Global Objects ---
// Karena Arduino memisahkan setup() dan loop(), kita butuh pointer global
// atau objek global agar state-nya terjaga.

// 1. Platform Drivers
platform::arduino::ArduinoGPIO platformGpio;
platform::arduino::ArduinoTime platformTime;
platform::arduino::ArduinoSerialComm platformComm;
platform::arduino::ArduinoSimpleCrypto platformCrypto;
platform::arduino::ArduinoInterruptStub platformInterrupt; // Masih stub
platform::arduino::ArduinoStorageStub platformStorage;     // Masih stub

// 2. Platform Services Container
platform::PlatformServices services;

// 3. Main System
GridShieldSystem* gridSystem = nullptr;

void setup() {
    // 1. Inisialisasi Serial untuk Debugging
    Serial.begin(115200);
    while (!Serial) { delay(10); } // Tunggu serial monitor (opsional)
    
    Serial.println("\n=== Booting GridShield Firmware v1.0 ===");

    // 2. Wiring Platform Services
    // Sambungkan driver yang kita buat ke struct services
    services.gpio = &platformGpio;
    services.time = &platformTime;
    services.comm = &platformComm;
    services.crypto = &platformCrypto;
    services.interrupt = &platformInterrupt;
    services.storage = &platformStorage;

    // 3. Konfigurasi Sistem
    SystemConfig config;
    // FIX: Gunakan Hexadesimal valid (0-9, A-F). 'S' dan 'P' tidak valid.
    config.meter_id = 0xEA320001; 
    config.heartbeat_interval_ms = 5000;
    config.reading_interval_ms = 10000;
    
    // Konfigurasi Tamper Pin (Sesuaikan dengan wiring di board kamu!)
    // Misal: Gunakan tombol BOOT di ESP32 (GPIO 0) atau pin lain
    config.tamper_config.sensor_pin = 0; 
    config.tamper_config.debounce_ms = 50;

    // 4. Inisialisasi GridShield System
    gridSystem = new GridShieldSystem();
    if (gridSystem == nullptr) {
        Serial.println("FATAL: Failed to allocate system memory!");
        while(1) delay(1000); // Halt
    }

    Serial.print("Initializing System... ");
    auto initResult = gridSystem->initialize(config, services);
    
    if (initResult.is_error()) {
        Serial.print("FAILED! Code: ");
        Serial.println((int)initResult.error().code());
        while(1) delay(1000); // Halt on error
    }
    Serial.println("OK!");

    // 5. Start System
    Serial.print("Starting Security Monitor... ");
    auto startResult = gridSystem->start();
    
    if (startResult.is_error()) {
        Serial.println("FAILED!");
    } else {
        Serial.println("OK! System Running.");
    }
}

void loop() {
    // Jalankan siklus proses utama GridShield
    if (gridSystem) {
        gridSystem->process_cycle();
    }
    
    // Beri sedikit delay agar tidak memakan CPU 100% dan trigger Watchdog
    // Di RTOS asli nanti bisa pakai vTaskDelay
    delay(10); 
}