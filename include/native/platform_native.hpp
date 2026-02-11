/**
 * @file platform_native.hpp
 * @author zuudevs (zuudevs@gmail.com) 
 * @brief Native (PC) platform implementation for testing (C++17)
 * @version 0.4
 * @date 2026-02-09
 * 
 * Required: OpenSSL 1.1.1+ or 3.0+
 * 
 * Installation:
 * - Windows: https://slproweb.com/products/Win32OpenSSL.html
 * - Linux: sudo apt install libssl-dev
 * - macOS: brew install openssl
 * 
 * CMake: link against OpenSSL::SSL and OpenSSL::Crypto
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"

#if GS_PLATFORM_NATIVE

#include <chrono>
#include <random>
#include <thread>
#include <cstdint>
#include <cstring>
#include <iostream>

#include <openssl/sha.h>
#include <openssl/rand.h>
#define USE_OPENSSL 1

namespace gridshield {
namespace platform {
namespace native {

// ============================================================================
// NATIVE TIME
// ============================================================================
class NativeTime final : public IPlatformTime {
public:
    NativeTime() noexcept : start_time_(std::chrono::steady_clock::now()) {}
    ~NativeTime() noexcept override = default;
    
    core::timestamp_t get_timestamp_ms() noexcept override {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - start_time_
        );
        return static_cast<core::timestamp_t>(duration.count());
    }
    
    void delay_ms(uint32_t ms) noexcept override {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
    
private:
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
};

// ============================================================================
// NATIVE GPIO
// ============================================================================
class NativeGPIO final : public IPlatformGPIO {
public:
    NativeGPIO() noexcept {
        std::memset(pin_states_, 0, sizeof(pin_states_));
        std::memset(pin_modes_, 0, sizeof(pin_modes_));
    }
    
    ~NativeGPIO() noexcept override = default;
    
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        pin_modes_[pin] = mode;
        return core::Result<void>();
    }
    
    core::Result<bool> read(uint8_t pin) noexcept override {
        return core::Result<bool>(pin_states_[pin]);
    }
    
    core::Result<void> write(uint8_t pin, bool value) noexcept override {
        if (GS_UNLIKELY(pin_modes_[pin] != PinMode::Output)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidState);
        }
        pin_states_[pin] = value;
        return core::Result<void>();
    }
    
    // Test helper
    void simulate_trigger(uint8_t pin, bool state) noexcept {
        pin_states_[pin] = state;
    }
    
private:
    bool pin_states_[256];
    PinMode pin_modes_[256];
};

// ============================================================================
// NATIVE INTERRUPT
// ============================================================================
class NativeInterrupt final : public IPlatformInterrupt {
public:
    NativeInterrupt() noexcept {
        std::memset(callbacks_, 0, sizeof(callbacks_));
        std::memset(contexts_, 0, sizeof(contexts_));
        std::memset(enabled_, 0, sizeof(enabled_));
    }
    
    ~NativeInterrupt() noexcept override = default;
    
    core::Result<void> attach(uint8_t pin, TriggerMode /*mode*/, 
                             InterruptCallback callback, 
                             void* context) noexcept override {
        if (GS_UNLIKELY(callback == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        callbacks_[pin] = callback;
        contexts_[pin] = context;
        return core::Result<void>();
    }
    
    core::Result<void> detach(uint8_t pin) noexcept override {
        callbacks_[pin] = nullptr;
        contexts_[pin] = nullptr;
        return core::Result<void>();
    }
    
    core::Result<void> enable(uint8_t pin) noexcept override {
        enabled_[pin] = true;
        return core::Result<void>();
    }
    
    core::Result<void> disable(uint8_t pin) noexcept override {
        enabled_[pin] = false;
        return core::Result<void>();
    }
    
    // Test helper
    void simulate_interrupt(uint8_t pin) noexcept {
        if (enabled_[pin] && callbacks_[pin] != nullptr) {
            callbacks_[pin](contexts_[pin]);
        }
    }
    
private:
    InterruptCallback callbacks_[256];
    void* contexts_[256];
    bool enabled_[256];
};

// ============================================================================
// NATIVE CRYPTO (OpenSSL)
// ============================================================================
class NativeCrypto final : public IPlatformCrypto {
public:
    NativeCrypto() noexcept {
#ifndef USE_OPENSSL
        // Fallback to C++ RNG when OpenSSL not available
        std::random_device rd;
        rng_ = std::mt19937(rd());
#endif
    }
    
    ~NativeCrypto() noexcept override = default;
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
#ifdef USE_OPENSSL
        // OpenSSL cryptographically secure RNG
        if (RAND_bytes(buffer, static_cast<int>(length)) != 1) {
            return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
        }
        
#else
        // Fallback: C++ pseudo-random (NOT cryptographically secure)
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(rng_));
        }
#endif
        
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
        
#ifdef USE_OPENSSL
        // OpenSSL SHA-256 (PRODUCTION)
        SHA256_CTX ctx;
        if (SHA256_Init(&ctx) != 1) {
            return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
        }
        
        if (SHA256_Update(&ctx, data, length) != 1) {
            return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
        }
        
        if (SHA256_Final(hash_out, &ctx) != 1) {
            return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
        }
        
#else
        // Fallback: Simple hash (NOT cryptographically secure)
        for (size_t i = 0; i < 32; ++i) {
            uint8_t val = (length > 0) ? data[i % length] : 0;
            hash_out[i] = static_cast<uint8_t>((val + i * 7) & 0xFF);
        }
#endif
        
        return core::Result<void>();
    }
    
private:
#ifndef USE_OPENSSL
    std::mt19937 rng_;
#endif
};

// ============================================================================
// NATIVE COMMUNICATION
// ============================================================================
class NativeComm final : public IPlatformComm {
public:
    NativeComm() noexcept : initialized_(false), connected_(true) {}
    
    ~NativeComm() noexcept override {
        if (initialized_) {
            shutdown();
        }
    }
    
    core::Result<void> init() noexcept override {
        if (!initialized_) {
            tx_buffer_.clear();
            rx_buffer_.clear();
            initialized_ = true;
        }
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        initialized_ = false;
        connected_ = false;
        return core::Result<void>();
    }
    
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        if (GS_UNLIKELY(!initialized_ || !connected_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        size_t sent = 0;
        for (size_t i = 0; i < length && !tx_buffer_.full(); ++i) {
            if (tx_buffer_.push(data[i])) {
                ++sent;
            }
        }
        
        return core::Result<size_t>(sent);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length, 
                                uint32_t /*timeout_ms*/) noexcept override {
        if (GS_UNLIKELY(!initialized_ || !connected_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (GS_UNLIKELY(buffer == nullptr || max_length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        if (rx_buffer_.empty()) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkTimeout));
        }
        
        size_t received = 0;
        while (received < max_length && !rx_buffer_.empty()) {
            uint8_t byte;
            if (rx_buffer_.pop(byte)) {
                buffer[received++] = byte;
            }
        }
        
        return core::Result<size_t>(received);
    }
    
    bool is_connected() noexcept override {
        return connected_;
    }
    
    // Test helpers
    GS_NODISCARD const core::StaticBuffer<uint8_t, 2048>& get_tx_buffer() const noexcept { 
        return tx_buffer_; 
    }
    
    void inject_rx_data(const uint8_t* data, size_t len) noexcept {
        for (size_t i = 0; i < len && !rx_buffer_.full(); ++i) {
            (void)rx_buffer_.push(data[i]);
        }
    }
    
    void clear_buffers() noexcept {
        tx_buffer_.clear();
        rx_buffer_.clear();
    }
    
private:
    bool initialized_;
    bool connected_;
    core::StaticBuffer<uint8_t, 2048> tx_buffer_;
    core::StaticBuffer<uint8_t, 2048> rx_buffer_;
};

} // namespace native
} // namespace platform
} // namespace gridshield

#endif // GS_PLATFORM_NATIVE