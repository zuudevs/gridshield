/**
 * @file mock_platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "platform/platform.hpp"
#include <chrono>
#include <random>

namespace gridshield::platform::mock {

class MockTime : public IPlatformTime {
public:
    MockTime() noexcept : start_time_(std::chrono::steady_clock::now()) {}
    
    core::timestamp_t get_timestamp_ms() noexcept override {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
        return static_cast<core::timestamp_t>(duration.count());
    }
    
    void delay_ms(uint32_t ms) noexcept override {
        auto start = std::chrono::steady_clock::now();
        while (true) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);
            if (elapsed.count() >= ms) break;
        }
    }
    
private:
    std::chrono::steady_clock::time_point start_time_;
};

class MockGPIO : public IPlatformGPIO {
public:
    MockGPIO() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            pin_states_[i] = false;
            pin_modes_[i] = PinMode::Input;
        }
    }
    
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        pin_modes_[pin] = mode;
        return core::Result<void>();
    }
    
    core::Result<bool> read(uint8_t pin) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        return core::Result<bool>(pin_states_[pin]);
    }
    
    core::Result<void> write(uint8_t pin, bool value) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        pin_states_[pin] = value;
        return core::Result<void>();
    }
    
    void simulate_trigger(uint8_t pin, bool state) {
        if (pin < 256) {
            pin_states_[pin] = state;
        }
    }
    
private:
    bool pin_states_[256];
    PinMode pin_modes_[256];
};

class MockInterrupt : public IPlatformInterrupt {
public:
    MockInterrupt() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            callbacks_[i] = nullptr;
            contexts_[i] = nullptr;
            enabled_[i] = false;
        }
    }
    
    core::Result<void> attach(uint8_t pin, TriggerMode mode, 
                             InterruptCallback callback, 
                             void* context) noexcept override {
        if (pin >= 256 || callback == nullptr) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        callbacks_[pin] = callback;
        contexts_[pin] = context;
        return core::Result<void>();
    }
    
    core::Result<void> detach(uint8_t pin) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        callbacks_[pin] = nullptr;
        contexts_[pin] = nullptr;
        return core::Result<void>();
    }
    
    core::Result<void> enable(uint8_t pin) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        enabled_[pin] = true;
        return core::Result<void>();
    }
    
    core::Result<void> disable(uint8_t pin) noexcept override {
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        enabled_[pin] = false;
        return core::Result<void>();
    }
    
    void simulate_interrupt(uint8_t pin) {
        if (pin < 256 && enabled_[pin] && callbacks_[pin] != nullptr) {
            callbacks_[pin](contexts_[pin]);
        }
    }
    
private:
    InterruptCallback callbacks_[256];
    void* contexts_[256];
    bool enabled_[256];
};

class MockCrypto : public IPlatformCrypto {
public:
    MockCrypto() noexcept : rng_(std::random_device{}()) {}
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (buffer == nullptr || length == 0) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(rng_));
        }
        
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        if (data == nullptr) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Simple checksum (not real CRC32)
        uint32_t sum = 0;
        for (size_t i = 0; i < length; ++i) {
            sum += data[i];
        }
        return core::Result<uint32_t>(sum);
    }
    
    core::Result<void> sha256(const uint8_t* data, size_t length, 
                             uint8_t* hash_out) noexcept override {
        if (data == nullptr || hash_out == nullptr) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Placeholder hash (not real SHA256)
        for (size_t i = 0; i < 32; ++i) {
            hash_out[i] = static_cast<uint8_t>((data[i % length] + i) & 0xFF);
        }
        
        return core::Result<void>();
    }
    
private:
    std::mt19937 rng_;
};

class MockComm : public IPlatformComm {
public:
    MockComm() noexcept : initialized_(false), connected_(true) {
        tx_buffer_.clear();
        rx_buffer_.clear();
    }
    
    core::Result<void> init() noexcept override {
        initialized_ = true;
        return core::Result<void>();
    }
    
    core::Result<void> shutdown() noexcept override {
        initialized_ = false;
        connected_ = false;
        return core::Result<void>();
    }
    
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        if (!initialized_ || !connected_) {
            return MAKE_ERROR(core::ErrorCode::NetworkDisconnected);
        }
        
        if (data == nullptr || length == 0) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Simulate sending by storing in TX buffer
        for (size_t i = 0; i < length && !tx_buffer_.full(); ++i) {
            tx_buffer_.push(data[i]);
        }
        
        return core::Result<size_t>(length);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length, 
                                uint32_t timeout_ms) noexcept override {
        if (!initialized_ || !connected_) {
            return MAKE_ERROR(core::ErrorCode::NetworkDisconnected);
        }
        
        if (buffer == nullptr || max_length == 0) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        // Simulate receiving from RX buffer
        size_t received = 0;
        while (received < max_length && !rx_buffer_.empty()) {
            uint8_t byte;
            if (rx_buffer_.pop(byte)) {
                buffer[received++] = byte;
            }
        }
        
        if (received == 0) {
            return MAKE_ERROR(core::ErrorCode::NetworkTimeout);
        }
        
        return core::Result<size_t>(received);
    }
    
    bool is_connected() noexcept override {
        return connected_;
    }
    
    const core::StaticBuffer<uint8_t, 2048>& get_tx_buffer() const { return tx_buffer_; }
    
private:
    bool initialized_;
    bool connected_;
    core::StaticBuffer<uint8_t, 2048> tx_buffer_;
    core::StaticBuffer<uint8_t, 2048> rx_buffer_;
};

} // namespace gridshield::platform::mock