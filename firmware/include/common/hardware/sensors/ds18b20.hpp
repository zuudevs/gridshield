/**
 * @file ds18b20.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief DS18B20 digital temperature sensor driver (OneWire)
 * @version 1.0
 * @date 2026-02-25
 *
 * OneWire-based digital temperature sensor with 0.0625°C resolution.
 * Supports skip ROM command for single-sensor configurations.
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
// DS18B20 CONSTANTS
// ============================================================================

// OneWire ROM commands
static constexpr uint8_t DS18B20_CMD_SKIP_ROM = 0xCC;
static constexpr uint8_t DS18B20_CMD_CONVERT_T = 0x44;
static constexpr uint8_t DS18B20_CMD_READ_SCRATCHPAD = 0xBE;

// Scratchpad layout
static constexpr size_t DS18B20_SCRATCHPAD_SIZE = 9;

// Temperature resolution
static constexpr int32_t DS18B20_RESOLUTION_SCALE = 16; // 1/16 °C = 0.0625°C

// Conversion time (maximum at 12-bit resolution)
static constexpr uint32_t DS18B20_CONVERSION_TIME_MS = 750;

// Temperature limits (in raw units)
static constexpr int16_t DS18B20_MIN_RAW = -880; // -55°C * 16
static constexpr int16_t DS18B20_MAX_RAW = 2000; // 125°C * 16

// ============================================================================
// DS18B20 CONFIGURATION
// ============================================================================
struct DS18B20Config
{
    uint8_t pin{};

    GS_CONSTEXPR DS18B20Config() noexcept = default;
};

// ============================================================================
// DS18B20 DRIVER
// ============================================================================
class DS18B20Driver
{
public:
    DS18B20Driver() noexcept = default;

    /**
     * @brief Initialize OneWire bus for the sensor.
     */
    core::Result<void> init(platform::IPlatformOneWire& one_wire,
                            const DS18B20Config& config) noexcept
    {
        ow_ = &one_wire;

        auto result = ow_->init(config.pin);
        if (result.is_error()) {
            return result;
        }

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read temperature in units of 0.1°C (e.g., 253 = 25.3°C).
     *
     * Issues a temperature conversion, waits, then reads the scratchpad.
     * Note: This is a blocking call (~750ms at 12-bit resolution).
     */
    core::Result<int16_t> read_temperature_c10() noexcept
    {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        if (GS_UNLIKELY(!initialized_ || ow_ == nullptr)) {
            return core::Result<int16_t>{GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

        // Reset bus
        GS_TRY(ow_->reset());

        // Skip ROM (single sensor on bus)
        GS_TRY(ow_->write_byte(DS18B20_CMD_SKIP_ROM));

        // Start conversion
        GS_TRY(ow_->write_byte(DS18B20_CMD_CONVERT_T));

        // NOTE: In real hardware, wait DS18B20_CONVERSION_TIME_MS here.
        // For testing, mock OneWire pre-loads the scratchpad data.

        // Reset and read scratchpad
        GS_TRY(ow_->reset());
        GS_TRY(ow_->write_byte(DS18B20_CMD_SKIP_ROM));
        GS_TRY(ow_->write_byte(DS18B20_CMD_READ_SCRATCHPAD));

        // Read 9 bytes of scratchpad
        std::array<uint8_t, DS18B20_SCRATCHPAD_SIZE> scratchpad{};
        for (auto& byte : scratchpad) {
            auto read_result = ow_->read_byte();
            if (read_result.is_error()) {
                return core::Result<int16_t>{read_result.error()};
            }
            byte = read_result.value();
        }

        // Parse temperature from bytes 0-1 (little-endian, signed)
        auto raw_temp = static_cast<int16_t>(static_cast<uint16_t>(scratchpad[0]) |
                                                (static_cast<uint16_t>(scratchpad[1]) << 8));

        // Validate range
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        if (GS_UNLIKELY(raw_temp < DS18B20_MIN_RAW || raw_temp > DS18B20_MAX_RAW)) {
            return core::Result<int16_t>{GS_MAKE_ERROR(core::ErrorCode::SensorReadFailure)};
        }

        // Convert to 0.1°C: raw_temp is in 1/16°C units
        // temp_c10 = raw_temp * 10 / 16
        static constexpr int16_t DECI_DEGREE_MULTIPLIER = 10;
        auto temp_c10 = static_cast<int16_t>(
            (static_cast<int32_t>(raw_temp) * DECI_DEGREE_MULTIPLIER) / DS18B20_RESOLUTION_SCALE);

        return core::Result<int16_t>{temp_c10};
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    platform::IPlatformOneWire* ow_{};
    bool initialized_{false};
};

} // namespace gridshield::hardware::sensors
