/**
 * @file mock_platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Mock implementation for native testing (PC/Mac/Linux)
 * @version 0.4
 * @date 2026-02-07
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"
#include "utils/gs_macros.hpp"

#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
#include <chrono>
#include <cstdint>
#include <cstring>
#include <random>
#include <thread>

#else
// NEED ADOPTION: For Arduino compatibility
#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#endif

namespace gridshield::platform::mock {

// ============================================================================
// MOCK TIME
// ============================================================================
class MockTime : public IPlatformTime
{
public:
    MockTime() noexcept
    {
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
        start_time_ = std::chrono::steady_clock::now();
#endif
    }

    core::timestamp_t get_timestamp_ms() noexcept override
    {
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time_);
        return static_cast<core::timestamp_t>(duration.count());
#else
        return static_cast<core::timestamp_t>(millis());
#endif
    }

    void delay_ms(uint32_t milliseconds) noexcept override
    {
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
        std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
#else
        delay(milliseconds);
#endif
    }

private:
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
    std::chrono::time_point<std::chrono::steady_clock> start_time_;
#endif
};

// ============================================================================
// MOCK GPIO
// ============================================================================
class MockGPIO : public IPlatformGPIO
{
public:
    MockGPIO() noexcept
    {
        memset(pin_states_, 0, sizeof(pin_states_));
        memset(pin_modes_, 0, sizeof(pin_modes_));
    }

    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override
    {
        pin_modes_[pin] = mode;
        return core::Result<void>{};
    }

    core::Result<bool> read(uint8_t pin) noexcept override
    {
        return core::Result<bool>(pin_states_[pin]);
    }

    core::Result<void> write(uint8_t pin, bool value) noexcept override
    {
        pin_states_[pin] = value;
        return core::Result<void>{};
    }

    // Test helper
    void simulate_trigger(uint8_t pin, bool state)
    {
        pin_states_[pin] = state;
    }

private:
    bool pin_states_[256];
    PinMode pin_modes_[256];
};

// ============================================================================
// MOCK INTERRUPT
// ============================================================================
class MockInterrupt : public IPlatformInterrupt
{
public:
    MockInterrupt() noexcept
    {
        memset(callbacks_, 0, sizeof(callbacks_));
        memset(contexts_, 0, sizeof(contexts_));
        memset(enabled_, 0, sizeof(enabled_));
    }

    core::Result<void> attach(uint8_t pin,
                              TriggerMode /*mode*/,
                              InterruptCallback callback,
                              void* context) noexcept override
    {
        if (GS_UNLIKELY(callback == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        callbacks_[pin] = callback;
        contexts_[pin] = context;
        return core::Result<void>{};
    }

    core::Result<void> detach(uint8_t pin) noexcept override
    {
        callbacks_[pin] = nullptr;
        contexts_[pin] = nullptr;
        return core::Result<void>{};
    }

    core::Result<void> enable(uint8_t pin) noexcept override
    {
        enabled_[pin] = true;
        return core::Result<void>{};
    }

    core::Result<void> disable(uint8_t pin) noexcept override
    {
        enabled_[pin] = false;
        return core::Result<void>{};
    }

    // Test helper
    void simulate_interrupt(uint8_t pin)
    {
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
// MOCK CRYPTO
// ============================================================================
class MockCrypto : public IPlatformCrypto
{
public:
    MockCrypto() noexcept
    {
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
        std::random_device rd;
        rng_ = std::mt19937(rd());
#else
        randomSeed(analogRead(0));
#endif
    }

    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override
    {
        if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
        std::uniform_int_distribution<int> dist(0, 255);
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(dist(rng_));
        }
#else
        for (size_t i = 0; i < length; ++i) {
            buffer[i] = static_cast<uint8_t>(random(256));
        }
#endif

        return core::Result<void>{};
    }

    core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept override
    {
        if (GS_UNLIKELY(data == nullptr)) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        // Simple checksum (NOT cryptographic CRC32)
        uint32_t sum = 0;
        for (size_t i = 0; i < length; ++i) {
            sum = ((sum << 5) + sum) + data[i];
        }
        return core::Result<uint32_t>(sum);
    }

    core::Result<void>
    sha256(const uint8_t* data, size_t length, uint8_t* hash_out) noexcept override
    {
        if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        // Placeholder hash (NOT cryptographically secure)
        for (size_t i = 0; i < 32; ++i) {
            uint8_t val = (length > 0) ? data[i % length] : 0;
            hash_out[i] = static_cast<uint8_t>((val + i) & 0xFF);
        }

        return core::Result<void>{};
    }

private:
#if GS_PLATFORM_NATIVE || defined(GS_QEMU_BUILD)
    std::mt19937 rng_;
#endif
};

// ============================================================================
// MOCK COMMUNICATION
// ============================================================================
class MockComm : public IPlatformComm
{
public:
    MockComm() noexcept : initialized_(false), connected_(true)
    {
        tx_buffer_.clear();
        rx_buffer_.clear();
    }

    core::Result<void> init() noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> shutdown() noexcept override
    {
        initialized_ = false;
        connected_ = false;
        return core::Result<void>{};
    }

    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override
    {
        if (GS_UNLIKELY(!initialized_ || !connected_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NetworkDisconnected));
        }

        if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        // Store in TX buffer
        for (size_t i = 0; i < length && !tx_buffer_.full(); ++i) {
            tx_buffer_.push(data[i]);
        }

        return core::Result<size_t>(length);
    }

    core::Result<size_t>
    receive(uint8_t* buffer, size_t max_length, uint32_t /*timeout_ms*/) noexcept override
    {
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

    bool is_connected() noexcept override
    {
        return connected_;
    }

    // Test helpers
    const core::StaticBuffer<uint8_t, 2048>& get_tx_buffer() const
    {
        return tx_buffer_;
    }

    void inject_rx_data(const uint8_t* data, size_t len)
    {
        for (size_t i = 0; i < len && !rx_buffer_.full(); i++) {
            rx_buffer_.push(data[i]);
        }
    }

    void set_connected(bool state)
    {
        connected_ = state;
    }

    void clear_buffers()
    {
        tx_buffer_.clear();
        rx_buffer_.clear();
    }

private:
    bool initialized_;
    bool connected_;
    core::StaticBuffer<uint8_t, 2048> tx_buffer_;
    core::StaticBuffer<uint8_t, 2048> rx_buffer_;
};

// ============================================================================
// MOCK STORAGE
// ============================================================================

class MockStorage : public IPlatformStorage
{
public:
    static constexpr size_t STORAGE_SIZE = 4096;

    MockStorage() noexcept
    {
#if GS_PLATFORM_NATIVE || GS_PLATFORM_ESP32
        std::memset(storage_, 0, STORAGE_SIZE);
#else
        memset(storage_, 0, STORAGE_SIZE);
#endif
    }

    core::Result<size_t> read(uint32_t address, uint8_t* buffer, size_t length) noexcept override
    {
        if (GS_UNLIKELY(address + length > STORAGE_SIZE || buffer == nullptr)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
#if GS_PLATFORM_NATIVE || GS_PLATFORM_ESP32
        std::memcpy(buffer, &storage_[address], length);
#else
        memcpy(buffer, &storage_[address], length);
#endif
        return core::Result<size_t>(length);
    }

    core::Result<size_t>
    write(uint32_t address, const uint8_t* data, size_t length) noexcept override
    {
        if (GS_UNLIKELY(address + length > STORAGE_SIZE || data == nullptr)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
#if GS_PLATFORM_NATIVE || GS_PLATFORM_ESP32
        std::memcpy(&storage_[address], data, length);
#else
        memcpy(&storage_[address], data, length);
#endif
        return core::Result<size_t>(length);
    }

    core::Result<void> erase(uint32_t address, size_t length) noexcept override
    {
        if (GS_UNLIKELY(address + length > STORAGE_SIZE)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
#if GS_PLATFORM_NATIVE || GS_PLATFORM_ESP32
        std::memset(&storage_[address], 0, length);
#else
        memset(&storage_[address], 0, length);
#endif
        return core::Result<void>{};
    }

private:
    uint8_t storage_[STORAGE_SIZE];
};

// ============================================================================
// MOCK WIFI
// ============================================================================
class MockWiFi : public IPlatformWiFi
{
public:
    MockWiFi() noexcept = default;

    core::Result<void> init() noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> connect(const char* /*ssid*/, const char* /*password*/) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        connected_ = true;
        return core::Result<void>{};
    }

    core::Result<void> disconnect() noexcept override
    {
        connected_ = false;
        return core::Result<void>{};
    }

    bool is_connected() noexcept override
    {
        return connected_;
    }

    core::Result<void> get_ip(char* buf, size_t len) noexcept override
    {
        if (GS_UNLIKELY(buf == nullptr || len == 0)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        static constexpr const char* MOCK_IP = "192.168.1.100";
        std::strncpy(buf, MOCK_IP, len - 1);
        buf[len - 1] = '\0';
        return core::Result<void>{};
    }

    // Test helpers
    void set_connected(bool state)
    {
        connected_ = state;
    }

private:
    bool initialized_{false};
    bool connected_{false};
};

// ============================================================================
// MOCK ADC
// ============================================================================
class MockADC : public IPlatformADC
{
public:
    MockADC() noexcept
    {
        mock_values_.fill(0);
    }

    core::Result<void> init(uint8_t channel, Attenuation /*atten*/) noexcept override
    {
        if (GS_UNLIKELY(channel >= MAX_ADC_CHANNELS)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        initialized_channels_[channel] = true;
        return core::Result<void>{};
    }

    core::Result<uint32_t> read_raw(uint8_t channel) noexcept override
    {
        if (GS_UNLIKELY(channel >= MAX_ADC_CHANNELS || !initialized_channels_[channel])) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::ADCReadError));
        }
        return core::Result<uint32_t>(mock_values_[channel]);
    }

    core::Result<uint32_t> read_mv(uint8_t channel) noexcept override
    {
        if (GS_UNLIKELY(channel >= MAX_ADC_CHANNELS || !initialized_channels_[channel])) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::ADCReadError));
        }
        // Convert raw (0-4095) to mV (0-3300) — 12-bit ADC range
        static constexpr uint32_t ADC_MAX_RAW = 4095;
        static constexpr uint32_t ADC_MAX_MV = 3300;
        uint32_t mv = (mock_values_[channel] * ADC_MAX_MV) / ADC_MAX_RAW;
        return core::Result<uint32_t>(mv);
    }

    // Test helpers
    void set_raw_value(uint8_t channel, uint32_t value)
    {
        if (channel < MAX_ADC_CHANNELS) {
            mock_values_[channel] = value;
        }
    }

private:
    std::array<uint32_t, MAX_ADC_CHANNELS> mock_values_{};
    std::array<bool, MAX_ADC_CHANNELS> initialized_channels_{};
};

// ============================================================================
// MOCK I2C
// ============================================================================
class MockI2C : public IPlatformI2C
{
public:
    static constexpr size_t MOCK_REGISTER_COUNT = 256;

    MockI2C() noexcept
    {
        mock_registers_.fill(0);
    }

    core::Result<void>
    init(uint8_t /*sda_pin*/, uint8_t /*scl_pin*/, uint32_t /*freq_hz*/) noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void>
    read_reg(uint8_t /*dev_addr*/, uint8_t reg_addr, uint8_t* buf, size_t len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_ || buf == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::I2CError);
        }
        for (size_t i = 0; i < len && (reg_addr + i) < MOCK_REGISTER_COUNT; ++i) {
            buf[i] = mock_registers_[reg_addr + i];
        }
        return core::Result<void>{};
    }

    core::Result<void> write_reg(uint8_t /*dev_addr*/,
                                 uint8_t reg_addr,
                                 const uint8_t* buf,
                                 size_t len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_ || buf == nullptr)) {
            return GS_MAKE_ERROR(core::ErrorCode::I2CError);
        }
        for (size_t i = 0; i < len && (reg_addr + i) < MOCK_REGISTER_COUNT; ++i) {
            mock_registers_[reg_addr + i] = buf[i];
        }
        return core::Result<void>{};
    }

    core::Result<void> shutdown() noexcept override
    {
        initialized_ = false;
        return core::Result<void>{};
    }

    // Test helpers
    void set_register(uint8_t reg, uint8_t value)
    {
        mock_registers_[reg] = value;
    }
    uint8_t get_register(uint8_t reg) const
    {
        return mock_registers_[reg];
    }

private:
    bool initialized_{false};
    std::array<uint8_t, MOCK_REGISTER_COUNT> mock_registers_{};
};

// ============================================================================
// MOCK ONE-WIRE
// ============================================================================
class MockOneWire : public IPlatformOneWire
{
public:
    static constexpr size_t MOCK_BUFFER_SIZE = 16;

    MockOneWire() noexcept
    {
        rx_data_.fill(0);
    }

    core::Result<void> init(uint8_t /*pin*/) noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> reset() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::OneWireError);
        }
        rx_pos_ = 0;
        return core::Result<void>{};
    }

    core::Result<void> write_byte(uint8_t /*data*/) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::OneWireError);
        }
        return core::Result<void>{};
    }

    core::Result<uint8_t> read_byte() noexcept override
    {
        if (GS_UNLIKELY(!initialized_ || rx_pos_ >= MOCK_BUFFER_SIZE)) {
            return core::Result<uint8_t>(GS_MAKE_ERROR(core::ErrorCode::OneWireError));
        }
        return core::Result<uint8_t>(rx_data_[rx_pos_++]);
    }

    // Test helpers
    void inject_data(const uint8_t* data, size_t len)
    {
        for (size_t i = 0; i < len && i < MOCK_BUFFER_SIZE; ++i) {
            rx_data_[i] = data[i];
        }
        rx_pos_ = 0;
    }

private:
    bool initialized_{false};
    std::array<uint8_t, MOCK_BUFFER_SIZE> rx_data_{};
    size_t rx_pos_{};
};

// ============================================================================
// MOCK UART
// ============================================================================
class MockUART : public IPlatformUART
{
public:
    static constexpr size_t UART_BUFFER_SIZE = 512;

    MockUART() noexcept
    {
        tx_buffer_.fill(0);
        rx_buffer_.fill(0);
    }

    core::Result<void> init(uint8_t port,
                            uint32_t /*baud_rate*/,
                            uint8_t /*tx_pin*/,
                            uint8_t /*rx_pin*/) noexcept override
    {
        if (GS_UNLIKELY(port >= MAX_UART_PORTS)) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        initialized_ports_[port] = true;
        return core::Result<void>{};
    }

    core::Result<size_t> write(uint8_t port, const uint8_t* data, size_t length) noexcept override
    {
        if (GS_UNLIKELY(port >= MAX_UART_PORTS || !initialized_ports_[port])) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::UARTError));
        }
        if (GS_UNLIKELY(data == nullptr || length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        size_t written = 0;
        while (written < length && tx_len_ < UART_BUFFER_SIZE) {
            tx_buffer_[tx_len_++] = data[written++];
        }
        return core::Result<size_t>(written);
    }

    core::Result<size_t> read(uint8_t port,
                              uint8_t* buffer,
                              size_t max_length,
                              uint32_t /*timeout_ms*/) noexcept override
    {
        if (GS_UNLIKELY(port >= MAX_UART_PORTS || !initialized_ports_[port])) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::UARTError));
        }
        if (GS_UNLIKELY(buffer == nullptr || max_length == 0)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        size_t read_count = 0;
        while (read_count < max_length && rx_pos_ < rx_len_) {
            buffer[read_count++] = rx_buffer_[rx_pos_++];
        }
        return core::Result<size_t>(read_count);
    }

    core::Result<void> shutdown(uint8_t port) noexcept override
    {
        if (port < MAX_UART_PORTS) {
            initialized_ports_[port] = false;
        }
        return core::Result<void>{};
    }

    // Test helpers
    void inject_rx_data(const uint8_t* data, size_t len)
    {
        rx_len_ = 0;
        rx_pos_ = 0;
        for (size_t i = 0; i < len && i < UART_BUFFER_SIZE; ++i) {
            rx_buffer_[i] = data[i];
            rx_len_++;
        }
    }

    const std::array<uint8_t, UART_BUFFER_SIZE>& get_tx_buffer() const
    {
        return tx_buffer_;
    }
    size_t get_tx_length() const
    {
        return tx_len_;
    }

    void clear_buffers()
    {
        tx_buffer_.fill(0);
        rx_buffer_.fill(0);
        tx_len_ = 0;
        rx_len_ = 0;
        rx_pos_ = 0;
    }

private:
    std::array<bool, MAX_UART_PORTS> initialized_ports_{};
    std::array<uint8_t, UART_BUFFER_SIZE> tx_buffer_{};
    std::array<uint8_t, UART_BUFFER_SIZE> rx_buffer_{};
    size_t tx_len_{};
    size_t rx_len_{};
    size_t rx_pos_{};
};

} // namespace gridshield::platform::mock