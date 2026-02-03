/**
 * @file platform_arduino.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <Arduino.h>
#include "platform/platform.hpp"
#include <vector>

namespace gridshield::platform::arduino {

class ArduinoTime : public IPlatformTime {
public:
    core::timestamp_t get_timestamp_ms() noexcept override {
        return static_cast<core::timestamp_t>(millis());
    }
    
    void delay_ms(uint32_t ms) noexcept override {
        delay(ms);
    }
};

class ArduinoSerialComm : public IPlatformComm {
public:
    core::Result<void> init() noexcept override {
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        return core::Result<void>();
    }
    
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        size_t written = Serial.write(data, length);
        return core::Result<size_t>(written);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length, uint32_t timeout_ms) noexcept override {
        unsigned long start = millis();
        size_t received = 0;
        
        while (millis() - start < timeout_ms && received < max_length) {
            if (Serial.available()) {
                buffer[received++] = Serial.read();
            } else {
                delay(1); 
            }
        }
        
        if (received == 0) return MAKE_ERROR(core::ErrorCode::NetworkTimeout);
        return core::Result<size_t>(received);
    }
    
    bool is_connected() noexcept override {
        return true;
    }
};

class ArduinoSimpleCrypto : public IPlatformCrypto {
public:
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        for(size_t i=0; i<length; i++) {
            buffer[i] = (uint8_t)random(0, 256);
        }
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        uint32_t sum = 0;
        for(size_t i=0; i<length; i++) sum += data[i];
        return core::Result<uint32_t>(sum);
    }
    
    core::Result<void> sha256(const uint8_t* data, size_t length, uint8_t* hash_out) noexcept override {
        memset(hash_out, 0, 32);
        for(size_t i=0; i<length; i++) {
            hash_out[i % 32] ^= data[i];
        }
        return core::Result<void>();
    }
};

class ArduinoInterruptStub : public IPlatformInterrupt {
public:
    core::Result<void> attach(uint8_t pin, TriggerMode mode, InterruptCallback callback, void* context) noexcept override {
        return core::Result<void>();
    }
    core::Result<void> detach(uint8_t pin) noexcept override { return core::Result<void>(); }
    core::Result<void> enable(uint8_t pin) noexcept override { return core::Result<void>(); }
    core::Result<void> disable(uint8_t pin) noexcept override { return core::Result<void>(); }
};

class ArduinoStorageStub : public IPlatformStorage {
public:
    core::Result<size_t> read(uint32_t address, uint8_t* buffer, size_t length) noexcept override { return core::Result<size_t>((size_t)0); }
    core::Result<size_t> write(uint32_t address, const uint8_t* data, size_t length) noexcept override { return core::Result<size_t>(length); }
    core::Result<void> erase(uint32_t address, size_t length) noexcept override { return core::Result<void>(); }
};

} // namespace gridshield::platform::arduino