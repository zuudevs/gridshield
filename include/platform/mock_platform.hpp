/**
 * @file mock_platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Mock implementation for platform interfaces
 * @version 0.2
 * @date 2026-02-07
 * * @copyright Copyright (c) 2026
 * */

#pragma once

#include "platform/platform.hpp"

#if defined(__AVR__) || defined(ARDUINO_ARCH_AVR)
    #define PLATFORM_AVR
    #include <Arduino.h>
	#include <stdint.h>

    using TimePoint = unsigned long;
    using Duration  = unsigned long;

#else
    #define PLATFORM_NATIVE
    #include <chrono>
    #include <random>
    #include <thread>
	#include <cstdint>

    using SysClock = std::chrono::steady_clock;
    using TimePoint = std::chrono::time_point<SysClock>;
    using Duration = std::chrono::milliseconds;
#endif 

// Helper function untuk waktu sekarang
inline TimePoint getCurrentTime() {
    #if defined(PLATFORM_NATIVE)
        return SysClock::now();
    #else
        return millis(); 
    #endif
}

namespace gridshield::platform::mock {

// -----------------------------------------------------------------------------
// MOCK TIME
// -----------------------------------------------------------------------------
class MockTime : public IPlatformTime {
public:
    MockTime() noexcept {
        #if defined(PLATFORM_NATIVE)
        start_time_ = SysClock::now();
        #endif
    }
    
    core::timestamp_t get_timestamp_ms() noexcept override {
        #if defined(PLATFORM_NATIVE)
            auto now = SysClock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
            return static_cast<core::timestamp_t>(duration.count());
        #else
            return static_cast<core::timestamp_t>(millis());
        #endif
    }
    
    void delay_ms(uint32_t ms) noexcept override {
        #if defined(PLATFORM_NATIVE)
            // JANGAN pakai while(true) loop kosong! Itu bikin CPU 100%.
            // Pakai sleep_for dari std::thread.
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        #else
            delay(ms);
        #endif
    }
    
private:
    // Member ini HANYA boleh ada di Native. 
    // Kalau di AVR, tipe data ini tidak dikenal.
    #if defined(PLATFORM_NATIVE)
    SysClock::time_point start_time_;
    #endif
};

// -----------------------------------------------------------------------------
// MOCK GPIO
// -----------------------------------------------------------------------------
class MockGPIO : public IPlatformGPIO {
public:
    MockGPIO() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            pin_states_[i] = false;
            pin_modes_[i] = PinMode::Input;
        }
    }
    
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        // Mencegah buffer overflow
        if (pin >= 256) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        pin_modes_[pin] = mode;
        return core::Result<void>();
    }
    
    core::Result<bool> read(uint8_t pin) noexcept override {
        if (pin >= 256) {
            return core::Result<bool>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
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
    
    // Helper untuk test environment
    void simulate_trigger(uint8_t pin, bool state) {
        if (pin < 256) {
            pin_states_[pin] = state;
        }
    }
    
private:
    bool pin_states_[256];
    PinMode pin_modes_[256];
};

// -----------------------------------------------------------------------------
// MOCK INTERRUPT
// -----------------------------------------------------------------------------
class MockInterrupt : public IPlatformInterrupt {
public:
    MockInterrupt() noexcept {
        for (size_t i = 0; i < 256; ++i) {
            callbacks_[i] = nullptr;
            contexts_[i] = nullptr;
            enabled_[i] = false;
        }
    }
    
    core::Result<void> attach(uint8_t pin, TriggerMode /*mode*/, 
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

// -----------------------------------------------------------------------------
// MOCK CRYPTO
// -----------------------------------------------------------------------------
class MockCrypto : public IPlatformCrypto {
public:
    MockCrypto() noexcept {
        #if defined(PLATFORM_NATIVE)
            // Seed random engine
            std::random_device rd;
            rng_ = std::mt19937(rd());
        #else
            // Arduino random seed (baca analog pin yg floating biasanya)
            randomSeed(analogRead(0));
        #endif
    }
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override {
        if (buffer == nullptr || length == 0) {
            return MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        
        #if defined(PLATFORM_NATIVE)
            std::uniform_int_distribution<int> dist(0, 255);
            for (size_t i = 0; i < length; ++i) {
                buffer[i] = static_cast<uint8_t>(dist(rng_));
            }
        #else
            for (size_t i = 0; i < length; ++i) {
                buffer[i] = static_cast<uint8_t>(random(256));
            }
        #endif
        
        return core::Result<void>();
    }
    
    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override {
        if (data == nullptr) {
            return core::Result<uint32_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // Simple checksum (Mock implementation)
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
        
        // Placeholder hash
        for (size_t i = 0; i < 32; ++i) {
            // Safe modulo arithmetic
            uint8_t val = (length > 0) ? data[i % length] : 0;
            hash_out[i] = static_cast<uint8_t>((val + i) & 0xFF);
        }
        
        return core::Result<void>();
    }
    
private:
    // INI YANG BIKIN ERROR SEBELUMNYA
    // std::mt19937 tidak ada di Arduino, jadi harus dibungkus #ifdef
    #if defined(PLATFORM_NATIVE)
    std::mt19937 rng_;
    #endif
};

// -----------------------------------------------------------------------------
// MOCK COMM
// -----------------------------------------------------------------------------
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
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (data == nullptr || length == 0) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        
        // Simpan ke TX buffer (seolah-olah dikirim keluar)
        for (size_t i = 0; i < length && !tx_buffer_.full(); ++i) {
            tx_buffer_.push(data[i]);
        }
        
        return core::Result<size_t>(length);
    }
    
    core::Result<size_t> receive(uint8_t* buffer, size_t max_length, 
                                uint32_t timeout_ms) noexcept override {
        if (!initialized_ || !connected_) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }
        
        if (buffer == nullptr || max_length == 0) {
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        // Simulasi timeout sederhana kalau buffer kosong
        if (rx_buffer_.empty()) {
            #if defined(PLATFORM_NATIVE)
                std::this_thread::sleep_for(std::chrono::milliseconds(1)); 
            #endif
            // Kalau real implementation mungkin nunggu sampai timeout, 
            // tapi untuk mock kita langsung return timeout error saja kalau kosong.
            return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::NetworkTimeout));
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
    
    // Getter untuk memeriksa apa yang sudah "dikirim" oleh aplikasi (untuk Unit Test)
    const core::StaticBuffer<uint8_t, 2048>& get_tx_buffer() const { return tx_buffer_; }
    
    // Helper untuk menyuntikkan data "diterima" (untuk Unit Test)
    void inject_rx_data(const uint8_t* data, size_t len) {
        for(size_t i=0; i<len && !rx_buffer_.full(); i++) {
            rx_buffer_.push(data[i]);
        }
    }
    
    void clear_buffers() {
        tx_buffer_.clear();
        rx_buffer_.clear();
    }

private:
    bool initialized_;
    bool connected_;
    core::StaticBuffer<uint8_t, 2048> tx_buffer_;
    core::StaticBuffer<uint8_t, 2048> rx_buffer_;
};

} // namespace gridshield::platform::mock