/**
 * @file platform_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Complete Arduino platform implementation for AVR
 * @version 0.2
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 *
 * PRODUCTION NOTE:
 * - Replace mock crypto with Crypto library by Rhys Weatherley
 * - Implement EEPROM storage using EEPROM.h
 * - Use hardware RNG if available (ESP32 has esp_random())
 */

#pragma once

#include "platform/platform.hpp"

#if PLATFORM_AVR
// NEED ADOPTION: Arduino core headers
#include <Arduino.h>

namespace gridshield {
namespace platform {
namespace arduino {

// ============================================================================
// ARDUINO TIME
// ============================================================================
class ArduinoTime : public IPlatformTime {
public:
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
class ArduinoGPIO : public IPlatformGPIO {
public:
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
                // AVR doesn't have internal pulldown - use external resistor
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
// ARDUINO INTERRUPT (Stub - requires manual ISR registration)
// ============================================================================
class ArduinoInterruptStub : public IPlatformInterrupt {
public:
    core::Result<void> attach(uint8_t /*pin*/, TriggerMode /*mode*/,
                             InterruptCallback /*callback*/, 
                             void* /*context*/) noexcept override {
        // NOTE: Arduino interrupts must be attached in sketch via attachInterrupt()
        // This is a stub to satisfy interface
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
// ARDUINO CRYPTO (Simple implementation - NOT cryptographically secure!)
// ============================================================================
class ArduinoSimpleCrypto : public IPlatformCrypto {
public:
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(random(256));
        }
        
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(data == nullptr)) {
            return core::Result<uint32_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // Simple checksum (not true CRC32)
        uint32_t sum = 0;
        for (size_t i = 0; i < length; ++i) {
            sum = ((sum << 5) + sum) + data[i];
        }
        return core::Result<uint32_t>(sum);
    }
    
    core::Result<void> sha256(const uint8_t* data, size_t length,
                             uint8_t* hash_out) noexcept override {
        if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // NEED ADOPTION: Replace with Crypto library
        // #include <SHA256.h>
        // SHA256 sha256;
        // sha256.update(data, length);
        // sha256.finalize(hash_out, 32);
        
        // Placeholder: Simple hash (INSECURE)
        for (size_t i = 0; i < 32; ++i) {
            hash_out[i] = (length > 0) ? data[i % length] ^ static_cast<uint8_t>(i) : 
                                         static_cast<uint8_t>(i);
        }
        
        return core::Result<void>();
    }
};

// ============================================================================
// ARDUINO STORAGE (Stub - requires EEPROM library)
// ============================================================================
class ArduinoStorageStub : public IPlatformStorage {
public:
    core::Result<size_t> read(uint32_t /*address*/, uint8_t* /*buffer*/,
                             size_t /*length*/) noexcept override {
        // NEED ADOPTION: Implement with EEPROM library
        // #include <EEPROM.h>
        // for (size_t i = 0; i < length; ++i) {
        //     buffer[i] = EEPROM.read(address + i);
        // }
        return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NotImplemented));
    }
    
    core::Result<size_t> write(uint32_t /*address*/, const uint8_t* /*data*/,
                              size_t /*length*/) noexcept override {
        return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NotImplemented));
    }
    
    core::Result<void> erase(uint32_t /*address*/, size_t /*length*/) noexcept override {
        return MAKE_ERROR(core::ErrorCode::NotImplemented);
    }
};

// ============================================================================
// ARDUINO SERIAL COMMUNICATION
// ============================================================================
class ArduinoSerialComm : public IPlatformComm {
public:
    ArduinoSerialComm() : initialized_(false) {}
    
    core::Result<void> init() noexcept override {
        if (!initialized_) {
            Serial.begin(115200);
            while (!Serial) { delay(10); }
            initialized_ = true;
        }
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        Serial.end();
        initialized_ = false;
        return core::Result<void>();
    }
    
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        size_t written = Serial.write(data, length);
        Serial.flush();
        
        return core::Result<size_t>(written);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length,
                                uint32_t timeout_ms) noexcept override {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(buffer == nullptr || max_length == 0)) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        unsigned long start = millis();
        size_t received = 0;
        
        while (received < max_length) {
            if (Serial.available() > 0) {
                buffer[received++] = Serial.read();
            }
            
            if (millis() - start > timeout_ms) {
                break;
            }
        }
        
        if (received == 0) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkTimeout));
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

#endif // PLATFORM_AVR