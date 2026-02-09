/**
 * @file platform_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Arduino platform implementation for AVR
 * @version 0.0.3
 * @date 2026-02-09
 * 
 * Third-party dependencies (optional):
 * - Crypto library by Rhys Weatherley (for SHA256)
 * - EEPROM (built-in for storage)
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"

#if GS_PLATFORM_ARDUINO

#include <Arduino.h>

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
// ARDUINO INTERRUPT (Stub)
// ============================================================================
class ArduinoInterrupt final : public IPlatformInterrupt {
public:
    ArduinoInterrupt() noexcept = default;
    ~ArduinoInterrupt() noexcept override = default;
    
    core::Result<void> attach(uint8_t /*pin*/, TriggerMode /*mode*/,
                             InterruptCallback /*callback*/, 
                             void* /*context*/) noexcept override {
        // Note: Arduino interrupts must be attached in sketch via attachInterrupt()
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
// ARDUINO CRYPTO (Simple)
// ============================================================================
class ArduinoCrypto final : public IPlatformCrypto {
public:
    ArduinoCrypto() noexcept = default;
    ~ArduinoCrypto() noexcept override = default;
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(random(256));
        }
        
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(data == nullptr)) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // Simple hash
        uint32_t sum = 0xFFFFFFFF;
        for (size_t i = 0; i < length; ++i) {
            sum = ((sum << 5) + sum) ^ data[i];
        }
        return core::Result<uint32_t>(~sum);
    }
    
    core::Result<void> sha256(const uint8_t* data, size_t length,
                             uint8_t* hash_out) noexcept override {
        if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // TODO: Replace with Crypto library
        // #include <SHA256.h>
        // SHA256 sha256;
        // sha256.update(data, length);
        // sha256.finalize(hash_out, 32);
        
        // Placeholder
        for (size_t i = 0; i < 32; ++i) {
            hash_out[i] = (length > 0) ? 
                static_cast<uint8_t>((data[i % length] + i * 7) & 0xFF) : 
                static_cast<uint8_t>(i);
        }
        
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
            while (!Serial) { 
                delay(10); 
            }
            initialized_ = true;
        }
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        if (initialized_) {
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