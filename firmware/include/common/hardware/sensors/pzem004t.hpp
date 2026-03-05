/**
 * @file pzem004t.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief PZEM-004T energy meter driver (UART/Modbus RTU)
 * @version 1.0
 * @date 2026-02-25
 *
 * All-in-one energy meter providing voltage, current, power,
 * energy, frequency, and power factor via Modbus RTU over UART.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <array>
#include <cstdint>

namespace gridshield::hardware::sensors {

// ============================================================================
// PZEM-004T CONSTANTS
// ============================================================================
static constexpr uint8_t PZEM_DEFAULT_ADDR = 0xF8; // Default slave address
static constexpr uint32_t PZEM_BAUD_RATE = 9600;
static constexpr uint32_t PZEM_TIMEOUT_MS = 500;
static constexpr size_t PZEM_RESPONSE_SIZE = 25; // Modbus response frame length
static constexpr size_t PZEM_REQUEST_SIZE = 8;   // Modbus request frame length

// Modbus register addresses
static constexpr uint16_t PZEM_REG_VOLTAGE = 0x0000;
static constexpr uint16_t PZEM_REG_CURRENT = 0x0001;    // Low word
static constexpr uint16_t PZEM_REG_CURRENT_HI = 0x0002; // High word
static constexpr uint16_t PZEM_REG_POWER = 0x0003;      // Low word
static constexpr uint16_t PZEM_REG_POWER_HI = 0x0004;   // High word
static constexpr uint16_t PZEM_REG_ENERGY = 0x0005;     // Low word
static constexpr uint16_t PZEM_REG_ENERGY_HI = 0x0006;  // High word
static constexpr uint16_t PZEM_REG_FREQUENCY = 0x0007;
static constexpr uint16_t PZEM_REG_POWER_FACTOR = 0x0008;
static constexpr uint16_t PZEM_REG_ALARM_STATUS = 0x0009;
static constexpr uint16_t PZEM_INPUT_REGISTER_COUNT = 10;

// Scaling factors (fixed-point divisors)
static constexpr uint32_t PZEM_VOLTAGE_SCALE = 10;   // 0.1V resolution
static constexpr uint32_t PZEM_CURRENT_SCALE = 1000; // 0.001A resolution
static constexpr uint32_t PZEM_POWER_SCALE = 10;     // 0.1W resolution
static constexpr uint32_t PZEM_FREQ_SCALE = 10;      // 0.1Hz resolution
static constexpr uint32_t PZEM_PF_SCALE = 100;       // 0.01 resolution

// ============================================================================
// PZEM-004T READING
// ============================================================================
struct PzemReading
{
    uint32_t voltage_mv{};       // Voltage in millivolts
    uint32_t current_ma{};       // Current in milliamps
    uint32_t power_mw{};         // Active power in milliwatts
    uint32_t energy_wh{};        // Energy in watt-hours
    uint16_t frequency_hz10{};   // Frequency in 0.1 Hz
    uint16_t power_factor_100{}; // Power factor * 100
    bool alarm{};
};

// ============================================================================
// PZEM-004T CONFIGURATION
// ============================================================================
struct PzemConfig
{
    uint8_t uart_port{};
    uint8_t tx_pin{};
    uint8_t rx_pin{};
    uint8_t slave_addr{PZEM_DEFAULT_ADDR};

    GS_CONSTEXPR PzemConfig() noexcept = default;
};

// ============================================================================
// PZEM-004T DRIVER
// ============================================================================
class PZEM004TDriver
{
public:
    PZEM004TDriver() noexcept = default;

    /**
     * @brief Initialize UART for PZEM communication.
     */
    core::Result<void> init(platform::IPlatformUART& uart, const PzemConfig& config) noexcept
    {
        uart_ = &uart;
        config_ = config;

        auto result =
            uart_->init(config_.uart_port, PZEM_BAUD_RATE, config_.tx_pin, config_.rx_pin);
        if (result.is_error()) {
            return result;
        }

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read all measurements from the PZEM-004T.
     *
     * Sends a Modbus RTU "Read Input Registers" command and parses the response.
     */
    core::Result<PzemReading> read() noexcept
    {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        if (GS_UNLIKELY(!initialized_ || uart_ == nullptr)) {
            return core::Result<PzemReading>{GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

        // Build Modbus RTU request frame
        std::array<uint8_t, PZEM_REQUEST_SIZE> request{};
        static constexpr uint8_t MODBUS_FUNC_READ_INPUT = 0x04;
        static constexpr size_t IDX_SLAVE_ADDR = 0;
        static constexpr size_t IDX_FUNCTION = 1;
        static constexpr size_t IDX_START_ADDR_HI = 2;
        static constexpr size_t IDX_START_ADDR_LO = 3;
        static constexpr size_t IDX_REG_COUNT_HI = 4;
        static constexpr size_t IDX_REG_COUNT_LO = 5;
        static constexpr size_t IDX_CRC_LO = 6;
        static constexpr size_t IDX_CRC_HI = 7;
        static constexpr size_t CRC_DATA_LENGTH = 6; // Bytes before CRC

        request[IDX_SLAVE_ADDR] = config_.slave_addr;
        request[IDX_FUNCTION] = MODBUS_FUNC_READ_INPUT;
        request[IDX_START_ADDR_HI] = 0x00;
        request[IDX_START_ADDR_LO] = 0x00;
        request[IDX_REG_COUNT_HI] = 0x00;
        request[IDX_REG_COUNT_LO] = static_cast<uint8_t>(PZEM_INPUT_REGISTER_COUNT);
        auto crc = calculate_crc16(request.data(), CRC_DATA_LENGTH);
        static constexpr uint8_t BYTE_MASK = 0xFF;
        static constexpr uint8_t BITS_PER_BYTE = 8;
        request[IDX_CRC_LO] = static_cast<uint8_t>(crc & BYTE_MASK);
        request[IDX_CRC_HI] = static_cast<uint8_t>((crc >> BITS_PER_BYTE) & BYTE_MASK);

        // Send request
        auto tx_result = uart_->write(config_.uart_port, request.data(), request.size());
        if (tx_result.is_error()) {
            return core::Result<PzemReading>{tx_result.error()};
        }

        // Read response
        std::array<uint8_t, PZEM_RESPONSE_SIZE> response{};
        auto rx_result =
            uart_->read(config_.uart_port, response.data(), response.size(), PZEM_TIMEOUT_MS);
        if (rx_result.is_error()) {
            return core::Result<PzemReading>{rx_result.error()};
        }

        return parse_response(response);
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    platform::IPlatformUART* uart_{};
    PzemConfig config_{};
    bool initialized_{false};

    /// Byte offsets within the Modbus response data payload (after header)
    static constexpr size_t RESP_HEADER_SIZE = 3; // [addr][func][byte_count]
    static constexpr size_t BYTES_PER_REGISTER = 2;

    // Register data byte offsets (relative to data start = RESP_HEADER_SIZE)
    static constexpr size_t REG0_VOLTAGE_OFFSET = 0;
    static constexpr size_t REG1_CURRENT_LO_OFFSET = 2;
    static constexpr size_t REG2_CURRENT_HI_OFFSET = 4;
    static constexpr size_t REG3_POWER_LO_OFFSET = 6;
    static constexpr size_t REG4_POWER_HI_OFFSET = 8;
    static constexpr size_t REG5_ENERGY_LO_OFFSET = 10;
    static constexpr size_t REG6_ENERGY_HI_OFFSET = 12;
    static constexpr size_t REG7_FREQUENCY_OFFSET = 14;
    static constexpr size_t REG8_PF_OFFSET = 16;
    static constexpr size_t REG9_ALARM_OFFSET = 18;

    // Bit shift constants
    static constexpr uint8_t SHIFT_8 = 8;
    static constexpr uint8_t SHIFT_16 = 16;
    static constexpr uint8_t SHIFT_24 = 24;

    /**
     * @brief Extract a 16-bit register value from response data.
     */
    static uint16_t extract_u16(const std::array<uint8_t, PZEM_RESPONSE_SIZE>& data,
                                size_t offset) noexcept
    {
        size_t idx = RESP_HEADER_SIZE + offset;
        return static_cast<uint16_t>((data[idx] << SHIFT_8) | data[idx + 1]);
    }

    /**
     * @brief Extract a 32-bit value from two consecutive registers (hi:lo order).
     */
    static uint32_t extract_u32(const std::array<uint8_t, PZEM_RESPONSE_SIZE>& data,
                                size_t lo_offset,
                                size_t hi_offset) noexcept
    {
        uint16_t lo_word = extract_u16(data, lo_offset);
        uint16_t hi_word = extract_u16(data, hi_offset);
        return (static_cast<uint32_t>(hi_word) << SHIFT_16) | static_cast<uint32_t>(lo_word);
    }

    /**
     * @brief Parse the 25-byte Modbus RTU response into a PzemReading.
     */
    static core::Result<PzemReading>
    parse_response(const std::array<uint8_t, PZEM_RESPONSE_SIZE>& data) noexcept
    {
        PzemReading reading{};

        // Voltage: register 0 in 0.1V -> convert to mV (* 100)
        static constexpr uint32_t VOLTAGE_TO_MV = PZEM_VOLTAGE_SCALE * PZEM_VOLTAGE_SCALE;
        reading.voltage_mv =
            static_cast<uint32_t>(extract_u16(data, REG0_VOLTAGE_OFFSET)) * VOLTAGE_TO_MV;

        // Current: registers 1-2 (32-bit) in 0.001A = mA directly
        reading.current_ma = extract_u32(data, REG1_CURRENT_LO_OFFSET, REG2_CURRENT_HI_OFFSET);

        // Power: registers 3-4 (32-bit) in 0.1W -> convert to mW (* 100)
        static constexpr uint32_t POWER_TO_MW = PZEM_POWER_SCALE * PZEM_POWER_SCALE;
        reading.power_mw =
            extract_u32(data, REG3_POWER_LO_OFFSET, REG4_POWER_HI_OFFSET) * POWER_TO_MW;

        // Energy: registers 5-6 (32-bit) in 1 Wh
        reading.energy_wh = extract_u32(data, REG5_ENERGY_LO_OFFSET, REG6_ENERGY_HI_OFFSET);

        // Frequency: register 7 in 0.1 Hz
        reading.frequency_hz10 = extract_u16(data, REG7_FREQUENCY_OFFSET);

        // Power factor: register 8 in 0.01
        reading.power_factor_100 = extract_u16(data, REG8_PF_OFFSET);

        // Alarm status: register 9
        reading.alarm = extract_u16(data, REG9_ALARM_OFFSET) != 0;

        return core::Result<PzemReading>{reading};
    }

    /**
     * @brief Calculate Modbus CRC-16.
     */
    static uint16_t calculate_crc16(const uint8_t* data, size_t length) noexcept
    {
        static constexpr uint16_t CRC_INIT = 0xFFFF;
        static constexpr uint16_t CRC_POLYNOMIAL = 0xA001;
        static constexpr uint16_t CRC_LSB_MASK = 0x0001;

        uint16_t crc = CRC_INIT;
        for (size_t i = 0; i < length; ++i) {
            crc ^= static_cast<uint16_t>(data[i]);
            for (uint8_t bit = 0; bit < SHIFT_8; ++bit) {
                if ((crc & CRC_LSB_MASK) != 0) {
                    crc = (crc >> 1) ^ CRC_POLYNOMIAL;
                } else {
                    crc >>= 1;
                }
            }
        }
        return crc;
    }
};

} // namespace gridshield::hardware::sensors
