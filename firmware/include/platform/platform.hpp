/**
 * @file platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Hardware Abstraction Layer (HAL) interfaces
 * @version 0.4
 * @date 2026-02-25
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"

#include <array>

namespace gridshield::platform {

// ============================================================================
// CONSTANTS
// ============================================================================
static constexpr size_t MAX_SSID_LENGTH = 32;
static constexpr size_t MAX_PASSWORD_LENGTH = 64;
static constexpr size_t MAX_IP_STRING_LENGTH = 16;
static constexpr size_t MAX_ADC_CHANNELS = 8;
static constexpr size_t MAX_UART_PORTS = 3;

// ============================================================================
// TIME INTERFACE
// ============================================================================
class IPlatformTime
{
public:
    virtual ~IPlatformTime() noexcept = default;

    virtual core::timestamp_t get_timestamp_ms() noexcept = 0;
    virtual void delay_ms(uint32_t milliseconds) noexcept = 0;
};

// ============================================================================
// GPIO INTERFACE
// ============================================================================
class IPlatformGPIO
{
public:
    virtual ~IPlatformGPIO() noexcept = default;

    enum class PinMode : uint8_t
    {
        Input = 0,
        Output = 1,
        InputPullup = 2,
        InputPulldown = 3
    };

    virtual core::Result<void> configure(uint8_t pin, PinMode mode) noexcept = 0;
    virtual core::Result<bool> read(uint8_t pin) noexcept = 0;
    virtual core::Result<void> write(uint8_t pin, bool value) noexcept = 0;
};

using PinMode = IPlatformGPIO::PinMode;

// ============================================================================
// INTERRUPT INTERFACE
// ============================================================================
class IPlatformInterrupt
{
public:
    virtual ~IPlatformInterrupt() noexcept = default;

    enum class TriggerMode : uint8_t
    {
        Rising = 0,
        Falling = 1,
        Change = 2,
        Low = 3,
        High = 4
    };

    using InterruptCallback = void (*)(void* context) noexcept;

    virtual core::Result<void>
    attach(uint8_t pin, TriggerMode mode, InterruptCallback callback, void* context) noexcept = 0;
    virtual core::Result<void> detach(uint8_t pin) noexcept = 0;
    virtual core::Result<void> enable(uint8_t pin) noexcept = 0;
    virtual core::Result<void> disable(uint8_t pin) noexcept = 0;
};

using TriggerMode = IPlatformInterrupt::TriggerMode;
using InterruptCallback = IPlatformInterrupt::InterruptCallback;

// ============================================================================
// CRYPTO INTERFACE
// ============================================================================
class IPlatformCrypto
{
public:
    virtual ~IPlatformCrypto() noexcept = default;

    virtual core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept = 0;
    virtual core::Result<uint32_t> crc32(const uint8_t* data, size_t length) noexcept = 0;
    virtual core::Result<void>
    sha256(const uint8_t* data, size_t length, uint8_t* hash_out) noexcept = 0;
};

// ============================================================================
// STORAGE INTERFACE
// ============================================================================
class IPlatformStorage
{
public:
    virtual ~IPlatformStorage() noexcept = default;

    virtual core::Result<size_t>
    read(uint32_t address, uint8_t* buffer, size_t length) noexcept = 0;
    virtual core::Result<size_t>
    write(uint32_t address, const uint8_t* data, size_t length) noexcept = 0;
    virtual core::Result<void> erase(uint32_t address, size_t length) noexcept = 0;
};

// ============================================================================
// COMMUNICATION INTERFACE
// ============================================================================
class IPlatformComm
{
public:
    virtual ~IPlatformComm() noexcept = default;

    virtual core::Result<void> init() noexcept = 0;
    virtual core::Result<void> shutdown() noexcept = 0;
    virtual core::Result<size_t> send(const uint8_t* data, size_t length) noexcept = 0;
    virtual core::Result<size_t>
    receive(uint8_t* buffer, size_t max_length, uint32_t timeout_ms) noexcept = 0;
    virtual bool is_connected() noexcept = 0;
};

// ============================================================================
// WIFI INTERFACE
// ============================================================================
class IPlatformWiFi
{
public:
    virtual ~IPlatformWiFi() noexcept = default;

    virtual core::Result<void> init() noexcept = 0;
    virtual core::Result<void> connect(const char* ssid, const char* password) noexcept = 0;
    virtual core::Result<void> disconnect() noexcept = 0;
    virtual bool is_connected() noexcept = 0;
    virtual core::Result<void> get_ip(char* buf, size_t len) noexcept = 0;
};

// ============================================================================
// ADC INTERFACE
// ============================================================================
class IPlatformADC
{
public:
    virtual ~IPlatformADC() noexcept = default;

    enum class Attenuation : uint8_t
    {
        Db0 = 0,   // 0 dB   — ~100–950 mV
        Db2_5 = 1, // 2.5 dB — ~100–1250 mV
        Db6 = 2,   // 6 dB   — ~150–1750 mV
        Db12 = 3   // 12 dB  — ~150–2450 mV
    };

    virtual core::Result<void> init(uint8_t channel, Attenuation atten) noexcept = 0;
    virtual core::Result<uint32_t> read_raw(uint8_t channel) noexcept = 0;
    virtual core::Result<uint32_t> read_mv(uint8_t channel) noexcept = 0;
};

using ADCAttenuation = IPlatformADC::Attenuation;

// ============================================================================
// I2C INTERFACE
// ============================================================================
class IPlatformI2C
{
public:
    virtual ~IPlatformI2C() noexcept = default;

    static constexpr uint32_t DEFAULT_FREQ_HZ = 100000;

    virtual core::Result<void>
    init(uint8_t sda_pin, uint8_t scl_pin, uint32_t freq_hz = DEFAULT_FREQ_HZ) noexcept = 0;

    virtual core::Result<void>
    read_reg(uint8_t dev_addr, uint8_t reg_addr, uint8_t* buf, size_t len) noexcept = 0;

    virtual core::Result<void>
    write_reg(uint8_t dev_addr, uint8_t reg_addr, const uint8_t* buf, size_t len) noexcept = 0;

    virtual core::Result<void> shutdown() noexcept = 0;
};

// ============================================================================
// ONE-WIRE INTERFACE
// ============================================================================
class IPlatformOneWire
{
public:
    virtual ~IPlatformOneWire() noexcept = default;

    virtual core::Result<void> init(uint8_t pin) noexcept = 0;
    virtual core::Result<void> reset() noexcept = 0;
    virtual core::Result<void> write_byte(uint8_t data) noexcept = 0;
    virtual core::Result<uint8_t> read_byte() noexcept = 0;
};

// ============================================================================
// UART INTERFACE
// ============================================================================
class IPlatformUART
{
public:
    virtual ~IPlatformUART() noexcept = default;

    static constexpr uint32_t DEFAULT_BAUD_RATE = 9600;

    virtual core::Result<void> init(uint8_t port, uint32_t baud_rate,
                                     uint8_t tx_pin, uint8_t rx_pin) noexcept = 0;

    virtual core::Result<size_t>
    write(uint8_t port, const uint8_t* data, size_t length) noexcept = 0;

    virtual core::Result<size_t>
    read(uint8_t port, uint8_t* buffer, size_t max_length, uint32_t timeout_ms) noexcept = 0;

    virtual core::Result<void> shutdown(uint8_t port) noexcept = 0;
};

// ============================================================================
// PLATFORM SERVICES AGGREGATOR
// ============================================================================
struct PlatformServices
{
    // Core (required)
    IPlatformTime* time{};
    IPlatformGPIO* gpio{};
    IPlatformInterrupt* interrupt{};
    IPlatformCrypto* crypto{};
    IPlatformStorage* storage{};
    IPlatformComm* comm{};

    // Communication (optional)
    IPlatformWiFi* wifi{};

    // Peripheral buses (optional)
    IPlatformADC* adc{};
    IPlatformI2C* i2c{};
    IPlatformOneWire* one_wire{};
    IPlatformUART* uart{};

    GS_CONSTEXPR PlatformServices() noexcept = default;

    GS_NODISCARD GS_CONSTEXPR bool is_valid() const noexcept
    {
        return time != nullptr && gpio != nullptr && interrupt != nullptr && crypto != nullptr;
    }
};

} // namespace gridshield::platform