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
#include <EEPROM.h>

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
// ARDUINO INTERRUPT (with static trampoline for Mega 2560)
// ============================================================================

// Arduino Mega 2560 interrupt-capable pins: 2, 3, 18, 19, 20, 21
// attachInterrupt() requires void(*)() — no context pointer support.
// Solution: static trampoline functions that dispatch to stored callbacks.

namespace detail {

// Max 6 hardware interrupts on Mega 2560 (INT0..INT5)
static constexpr uint8_t MAX_INTERRUPTS = 6;

struct InterruptSlot {
    IPlatformInterrupt::InterruptCallback callback;
    void* context;
    uint8_t pin;
    bool active;
};

// Static storage for ISR dispatching
inline InterruptSlot interrupt_slots[MAX_INTERRUPTS] = {};

// Map interrupt index to pin number (Mega 2560 layout)
inline uint8_t interrupt_index_to_pin(uint8_t idx) {
    constexpr uint8_t pins[] = {2, 3, 21, 20, 19, 18};
    return (idx < MAX_INTERRUPTS) ? pins[idx] : 0xFF;
}

// Find slot index for a given pin, returns MAX_INTERRUPTS if not found
inline uint8_t find_slot_for_pin(uint8_t pin) {
    for (uint8_t i = 0; i < MAX_INTERRUPTS; ++i) {
        if (interrupt_slots[i].pin == pin && interrupt_slots[i].active) {
            return i;
        }
    }
    return MAX_INTERRUPTS;
}

// Find first free slot
inline uint8_t find_free_slot() {
    for (uint8_t i = 0; i < MAX_INTERRUPTS; ++i) {
        if (!interrupt_slots[i].active) return i;
    }
    return MAX_INTERRUPTS;
}

// Generic trampoline dispatcher
inline void trampoline_dispatch(uint8_t slot_idx) {
    if (slot_idx < MAX_INTERRUPTS && interrupt_slots[slot_idx].active) {
        auto& slot = interrupt_slots[slot_idx];
        if (slot.callback != nullptr) {
            slot.callback(slot.context);
        }
    }
}

// Static trampoline ISRs (one per interrupt slot)
inline void trampoline_0() { trampoline_dispatch(0); }
inline void trampoline_1() { trampoline_dispatch(1); }
inline void trampoline_2() { trampoline_dispatch(2); }
inline void trampoline_3() { trampoline_dispatch(3); }
inline void trampoline_4() { trampoline_dispatch(4); }
inline void trampoline_5() { trampoline_dispatch(5); }

using TrampolineFn = void (*)();

inline TrampolineFn get_trampoline(uint8_t slot_idx) {
    constexpr TrampolineFn trampolines[] = {
        trampoline_0, trampoline_1, trampoline_2,
        trampoline_3, trampoline_4, trampoline_5
    };
    return (slot_idx < MAX_INTERRUPTS) ? trampolines[slot_idx] : nullptr;
}

} // namespace detail

class ArduinoInterrupt final : public IPlatformInterrupt {
public:
    ArduinoInterrupt() noexcept = default;
    ~ArduinoInterrupt() noexcept override = default;
    
    core::Result<void> attach(uint8_t pin, TriggerMode mode,
                             InterruptCallback callback, 
                             void* context) noexcept override {
        if (GS_UNLIKELY(callback == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        int interrupt_num = digitalPinToInterrupt(pin);
        if (interrupt_num == NOT_AN_INTERRUPT) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Convert TriggerMode to Arduino constant
        int arduino_mode;
        switch (mode) {
            case TriggerMode::Rising:  arduino_mode = RISING;  break;
            case TriggerMode::Falling: arduino_mode = FALLING; break;
            case TriggerMode::Change:  arduino_mode = CHANGE;  break;
            case TriggerMode::Low:     arduino_mode = LOW;     break;
            default: return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Find existing or allocate new slot
        uint8_t slot = detail::find_slot_for_pin(pin);
        if (slot == detail::MAX_INTERRUPTS) {
            slot = detail::find_free_slot();
            if (slot == detail::MAX_INTERRUPTS) {
                return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
            }
        }
        
        // Store callback info
        detail::interrupt_slots[slot].callback = callback;
        detail::interrupt_slots[slot].context = context;
        detail::interrupt_slots[slot].pin = pin;
        detail::interrupt_slots[slot].active = true;
        
        // Attach via Arduino API with static trampoline
        auto trampoline = detail::get_trampoline(slot);
        if (trampoline != nullptr) {
            attachInterrupt(interrupt_num, trampoline, arduino_mode);
        }
        
        return core::Result<void>();
    }
    
    core::Result<void> detach(uint8_t pin) noexcept override {
        int interrupt_num = digitalPinToInterrupt(pin);
        if (interrupt_num != NOT_AN_INTERRUPT) {
            detachInterrupt(interrupt_num);
        }
        
        uint8_t slot = detail::find_slot_for_pin(pin);
        if (slot < detail::MAX_INTERRUPTS) {
            detail::interrupt_slots[slot].callback = nullptr;
            detail::interrupt_slots[slot].context = nullptr;
            detail::interrupt_slots[slot].active = false;
        }
        
        return core::Result<void>();
    }
    
    core::Result<void> enable(uint8_t pin) noexcept override {
        // Re-enable by re-attaching the interrupt
        uint8_t slot = detail::find_slot_for_pin(pin);
        if (slot < detail::MAX_INTERRUPTS) {
            int interrupt_num = digitalPinToInterrupt(pin);
            if (interrupt_num != NOT_AN_INTERRUPT) {
                auto trampoline = detail::get_trampoline(slot);
                if (trampoline != nullptr) {
                    // Re-attach with FALLING (default for tamper)
                    attachInterrupt(interrupt_num, trampoline, FALLING);
                }
            }
        }
        return core::Result<void>();
    }
    
    core::Result<void> disable(uint8_t pin) noexcept override {
        int interrupt_num = digitalPinToInterrupt(pin);
        if (interrupt_num != NOT_AN_INTERRUPT) {
            detachInterrupt(interrupt_num);
        }
        return core::Result<void>();
    }
};

// ============================================================================
// ARDUINO CRYPTO (PRODUCTION)
// ============================================================================
class ArduinoCrypto final : public IPlatformCrypto {
public:
    ArduinoCrypto() noexcept : counter_(0) {
        // Initialize entropy pool from multiple analog sources + timing
        collect_entropy();
    }
    
    ~ArduinoCrypto() noexcept override = default;
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Generate random bytes using entropy pool + SHA-256 DRBG
        size_t offset = 0;
        while (offset < length) {
            // Mix entropy pool with counter and timing
            entropy_pool_[0] ^= static_cast<uint8_t>(counter_ >> 24);
            entropy_pool_[1] ^= static_cast<uint8_t>(counter_ >> 16);
            entropy_pool_[2] ^= static_cast<uint8_t>(counter_ >> 8);
            entropy_pool_[3] ^= static_cast<uint8_t>(counter_);
            entropy_pool_[4] ^= static_cast<uint8_t>(micros() & 0xFF);
            entropy_pool_[5] ^= static_cast<uint8_t>((micros() >> 8) & 0xFF);
            ++counter_;
            
            // Hash the pool to produce output block
            uint8_t hash_out[32];
            SHA256 sha;
            sha.reset();
            sha.update(entropy_pool_, sizeof(entropy_pool_));
            sha.finalize(hash_out, 32);
            
            // Copy hash output to buffer
            size_t to_copy = length - offset;
            if (to_copy > 32) to_copy = 32;
            for (size_t i = 0; i < to_copy; ++i) {
                buffer[offset + i] = hash_out[i];
            }
            offset += to_copy;
            
            // Feed hash back into pool (re-seed)
            for (size_t i = 0; i < 32; ++i) {
                entropy_pool_[i] ^= hash_out[i];
            }
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

private:
    uint8_t entropy_pool_[32];
    uint32_t counter_;

    void collect_entropy() noexcept {
        // Initial seeding using analog noise and timing jitter
        uint32_t seed_accum = 0;
        
        // Gather noise from unconnected analog pin (A0 is usually noisy enough if floating)
        // We accumulate multiple reads to get some entropy
        for (int i = 0; i < 64; ++i) {
            seed_accum = (seed_accum << 1) ^ analogRead(A0);
            delayMicroseconds(50); // Small delay to allow ADC noise to fluctuate
        }
        
        // Mix with boot time jitter
        seed_accum ^= micros();
        
        // Initialize pool with SHA-256 of the seed
        SHA256 sha;
        sha.reset();
        sha.update(&seed_accum, sizeof(seed_accum));
        sha.finalize(entropy_pool_, 32);
    }
};

// ============================================================================
// ARDUINO STORAGE (EEPROM)
// ============================================================================
class ArduinoStorage final : public IPlatformStorage {
public:
    ArduinoStorage() noexcept = default;
    ~ArduinoStorage() noexcept override = default;
    
    core::Result<size_t> read(uint32_t address, uint8_t* buffer, 
                             size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // EEPROM size check (Mega 2560 has 4KB)
        if (address + length > 4096) {
             return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }
        
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = EEPROM.read(address + i);
        }
        
        return core::Result<size_t>(length);
    }
    
    core::Result<size_t> write(uint32_t address, const uint8_t* data, 
                              size_t length) noexcept override {
         if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        if (address + length > 4096) {
             return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }
        
        for (size_t i = 0; i < length; ++i) {
            // update() only writes if value changed (preserves endurance)
            EEPROM.update(address + i, data[i]);
        }
        
        return core::Result<size_t>(length);
    }
    
    core::Result<void> erase(uint32_t address, size_t length) noexcept override {
         if (address + length > 4096) {
             return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
        }
        
        for (size_t i = 0; i < length; ++i) {
            EEPROM.update(address + i, 0xFF); // Erased state
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