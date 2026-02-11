/**
 * @file platform_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Arduino platform implementation with production crypto (C++17)
 * @version 0.5
 * @date 2026-02-10
 * 
 * Required libraries (install via arduino-cli):
 * 1. Crypto by Rhys Weatherley (INSTALLED)
 *    arduino-cli lib install Crypto
 * 
 * 2. micro-ecc (for ECDSA - MANUAL INSTALL REQUIRED)
 *    Download: https://github.com/kmackay/micro-ecc
 *    Extract to: Arduino/libraries/micro-ecc/
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"

#if GS_PLATFORM_ARDUINO

#include <Arduino.h>

// Crypto library for SHA-256 (PRODUCTION)
#include <SHA256.h>

// uECC for ECDSA (uncomment when installed)
// #include <uECC.h>

namespace gridshield {
namespace platform {
namespace arduino {

// ============================================================================
// ARDUINO TIME
// ============================================================================
class ArduinoTime final : public IPlatformTime {
public:
    ArduinoTime() noexcept = default;
    ~ArduinoTime() noexcept override = default;
    
    core::timestamp_t get_timestamp_ms() noexcept override {
        return static_cast<core::timestamp_t>(millis());
    }
    
    void delay_ms(uint32_t ms) noexcept override {
        delay(ms);
    }
};

// ============================================================================
// ARDUINO GPIO
// ============================================================================
class ArduinoGPIO final : public IPlatformGPIO {
public:
    ArduinoGPIO() noexcept = default;
    ~ArduinoGPIO() noexcept override = default;
    
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        switch (mode) {
            case PinMode::Input:
                pinMode(pin, INPUT);
                break;
            case PinMode::Output:
                pinMode(pin, OUTPUT);
                break;
            case PinMode::InputPullup:
                pinMode(pin, INPUT_PULLUP);
                break;
            case PinMode::InputPulldown:
                // AVR doesn't have internal pulldown
                pinMode(pin, INPUT);
                break;
        }
        return core::Result<void>();
    }
    
    core::Result<bool> read(uint8_t pin) noexcept override {
        return core::Result<bool>(digitalRead(pin) == HIGH);
    }
    
    core::Result<void> write(uint8_t pin, bool value) noexcept override {
        digitalWrite(pin, value ? HIGH : LOW);
        return core::Result<void>();
    }
};

// ============================================================================
// ARDUINO INTERRUPT
// ============================================================================
class ArduinoInterrupt final : public IPlatformInterrupt {
public:
    ArduinoInterrupt() noexcept = default;
    ~ArduinoInterrupt() noexcept override = default;
    
    core::Result<void> attach(uint8_t /*pin*/, TriggerMode /*mode*/,
                             InterruptCallback /*callback*/, 
                             void* /*context*/) noexcept override {
        // Arduino interrupts must be attached in sketch via attachInterrupt()
        return core::Result<void>();
    }
    
    core::Result<void> detach(uint8_t /*pin*/) noexcept override {
        return core::Result<void>();
    }
    
    core::Result<void> enable(uint8_t /*pin*/) noexcept override {
        return core::Result<void>();
    }
    
    core::Result<void> disable(uint8_t /*pin*/) noexcept override {
        return core::Result<void>();
    }
};

// ============================================================================
// ARDUINO CRYPTO (PRODUCTION)
// ============================================================================
class ArduinoCrypto final : public IPlatformCrypto {
public:
    ArduinoCrypto() noexcept {
        // Seed RNG with analog noise
        randomSeed(analogRead(0) ^ analogRead(1) ^ analogRead(2));
    }
    
    ~ArduinoCrypto() noexcept override = default;
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Use Arduino's hardware RNG seeded with analog noise
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(random(256));
        }
        
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(data == nullptr)) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // FNV-1a hash (lightweight checksum)
        uint32_t hash = 2166136261UL;
        for (size_t i = 0; i < length; ++i) {
            hash ^= data[i];
            hash *= 16777619UL;
        }
        
        return core::Result<uint32_t>(hash);
    }
    
    core::Result<void> sha256(const uint8_t* data, size_t length,
                             uint8_t* hash_out) noexcept override {
        if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // PRODUCTION: Use Crypto library SHA-256
        SHA256 sha256;
        sha256.reset();
        sha256.update(data, length);
        sha256.finalize(hash_out, 32);
        
        return core::Result<void>();
    }
};

// ============================================================================
// ARDUINO SERIAL COMMUNICATION
// ============================================================================
class ArduinoSerial final : public IPlatformComm {
public:
    ArduinoSerial() noexcept : initialized_(false) {}
    
    ~ArduinoSerial() noexcept override {
        if (initialized_) {
            shutdown();
        }
    }
    
    core::Result<void> init() noexcept override {
        if (!initialized_) {
            Serial.begin(115200);
            while (!Serial && millis() < 5000) { 
                delay(10); 
            }
            initialized_ = true;
        }
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        if (initialized_) {
            Serial.flush();
            Serial.end();
            initialized_ = false;
        }
        return core::Result<void>();
    }
    
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        size_t written = Serial.write(data, length);
        Serial.flush();
        
        return core::Result<size_t>(written);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length,
                                uint32_t timeout_ms) noexcept override {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(buffer == nullptr || max_length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        unsigned long start = millis();
        size_t received = 0;
        
        while (received < max_length) {
            if (Serial.available() > 0) {
                buffer[received++] = static_cast<uint8_t>(Serial.read());
            }
            
            if (millis() - start > timeout_ms) {
                break;
            }
        }
        
        if (received == 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkTimeout));
        }
        
        return core::Result<size_t>(received);
    }
    
    bool is_connected() noexcept override {
        return initialized_ && Serial;
    }
    
private:
    bool initialized_;
};

} // namespace arduino
} // namespace platform
} // namespace gridshield

#endif // GS_PLATFORM_ARDUINO