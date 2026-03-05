/**
 * @file mpu6050.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief MPU6050 accelerometer/gyroscope driver (I2C)
 * @version 1.0
 * @date 2026-02-25
 *
 * 6-axis motion sensor for tamper detection via physical shock/vibration.
 * Reads acceleration and angular velocity, with configurable shock threshold.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "core/vals.hpp"
#include "platform/platform.hpp"

#include <array>
#include <cstdint>

namespace gridshield::hardware::sensors {

// ============================================================================
// MPU6050 CONSTANTS
// ============================================================================

// I2C addresses
static constexpr uint8_t MPU6050_ADDR_LOW = 0x68;  // AD0 = LOW
static constexpr uint8_t MPU6050_ADDR_HIGH = 0x69; // AD0 = HIGH

// Register addresses
static constexpr uint8_t MPU6050_REG_ACCEL_XOUT_H = 0x3B;
static constexpr uint8_t MPU6050_REG_GYRO_XOUT_H = 0x43;
static constexpr uint8_t MPU6050_REG_PWR_MGMT_1 = 0x6B;
static constexpr uint8_t MPU6050_REG_WHO_AM_I = 0x75;
static constexpr uint8_t MPU6050_REG_ACCEL_CONFIG = 0x1C;
static constexpr uint8_t MPU6050_REG_GYRO_CONFIG = 0x1B;

// WHO_AM_I expected value
static constexpr uint8_t MPU6050_WHO_AM_I_VALUE = 0x68;

// Power management
static constexpr uint8_t MPU6050_WAKEUP_VALUE = 0x00;
static constexpr uint8_t MPU6050_SLEEP_VALUE = 0x40;

// Data sizes
static constexpr size_t MPU6050_ACCEL_DATA_SIZE = 6; // 3 axes * 2 bytes
static constexpr size_t MPU6050_GYRO_DATA_SIZE = 6;  // 3 axes * 2 bytes

// Sensitivity scale factors (LSB/unit at ±2g / ±250°/s defaults)
static constexpr int16_t MPU6050_ACCEL_SENSITIVITY = 16384; // LSB/g at ±2g
static constexpr int16_t MPU6050_GYRO_SENSITIVITY = 131;    // LSB/(°/s) at ±250°/s

// Default shock detection threshold (in mg, milli-g)
static constexpr uint16_t MPU6050_DEFAULT_SHOCK_THRESHOLD_MG = 1500;

// ============================================================================
// MPU6050 DATA STRUCTS
// ============================================================================
struct AccelData
{
    int16_t x_raw{};
    int16_t y_raw{};
    int16_t z_raw{};

    /// @brief Get acceleration in milli-g for each axis
    GS_NODISCARD int32_t x_mg() const noexcept
    {
        static constexpr int32_t MG_PER_G = 1000;
        return (static_cast<int32_t>(x_raw) * MG_PER_G) / MPU6050_ACCEL_SENSITIVITY;
    }

    GS_NODISCARD int32_t y_mg() const noexcept
    {
        static constexpr int32_t MG_PER_G = 1000;
        return (static_cast<int32_t>(y_raw) * MG_PER_G) / MPU6050_ACCEL_SENSITIVITY;
    }

    GS_NODISCARD int32_t z_mg() const noexcept
    {
        static constexpr int32_t MG_PER_G = 1000;
        return (static_cast<int32_t>(z_raw) * MG_PER_G) / MPU6050_ACCEL_SENSITIVITY;
    }
};

struct GyroData
{
    int16_t x_raw{};
    int16_t y_raw{};
    int16_t z_raw{};
};

// ============================================================================
// MPU6050 CONFIGURATION
// ============================================================================
struct MPU6050Config
{
    uint8_t i2c_addr{MPU6050_ADDR_LOW};
    uint8_t sda_pin{};
    uint8_t scl_pin{};
    uint16_t shock_threshold_mg{MPU6050_DEFAULT_SHOCK_THRESHOLD_MG};

    GS_CONSTEXPR MPU6050Config() noexcept = default;
};

// ============================================================================
// MPU6050 DRIVER
// ============================================================================
class MPU6050Driver
{
public:
    MPU6050Driver() noexcept = default;

    /**
     * @brief Initialize the MPU6050 — wake up from sleep and verify WHO_AM_I.
     */
    core::Result<void> init(platform::IPlatformI2C& i2c, const MPU6050Config& config) noexcept
    {
        i2c_ = &i2c;
        config_ = config;

        // Initialize I2C bus
        auto init_result = i2c_->init(config_.sda_pin, config_.scl_pin);
        if (init_result.is_error()) {
            return init_result;
        }

        // Verify WHO_AM_I register
        uint8_t who_am_i = 0;
        auto read_result =
            i2c_->read_reg(config_.i2c_addr, MPU6050_REG_WHO_AM_I, &who_am_i, sizeof(who_am_i));
        if (read_result.is_error()) {
            return read_result;
        }

        if (GS_UNLIKELY(who_am_i != MPU6050_WHO_AM_I_VALUE)) {
            return GS_MAKE_ERROR(core::ErrorCode::SensorNotFound);
        }

        // Wake up from sleep (clear sleep bit)
        auto wake_result = i2c_->write_reg(config_.i2c_addr,
                                           MPU6050_REG_PWR_MGMT_1,
                                           &MPU6050_WAKEUP_VALUE,
                                           sizeof(MPU6050_WAKEUP_VALUE));
        if (wake_result.is_error()) {
            return wake_result;
        }

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read accelerometer data (3-axis raw + milli-g conversion).
     */
    core::Result<AccelData> read_accel() noexcept
    {
        // NOLINTNEXTLINE(readability-simplify-boolean-expr)
        if (GS_UNLIKELY(!initialized_ || i2c_ == nullptr)) {
            return core::Result<AccelData>{GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

        std::array<uint8_t, MPU6050_ACCEL_DATA_SIZE> buf{};
        auto result =
            i2c_->read_reg(config_.i2c_addr, MPU6050_REG_ACCEL_XOUT_H, buf.data(), buf.size());
        if (result.is_error()) {
            return core::Result<AccelData>{result.error()};
        }

        AccelData data{};
        data.x_raw = static_cast<int16_t>((buf[0] << core::BITS_PER_BYTE) | buf[1]);
        data.y_raw = static_cast<int16_t>((buf[2] << core::BITS_PER_BYTE) | buf[3]);
        data.z_raw = static_cast<int16_t>((buf[4] << core::BITS_PER_BYTE) | buf[5]); // NOLINT(readability-magic-numbers)

        return core::Result<AccelData>{data};
    }

    /**
     * @brief Detect if a shock/vibration exceeds the configured threshold.
     * @return true if shock detected on any axis.
     */
    core::Result<bool> detect_shock() noexcept
    {
        auto accel_result = read_accel();
        if (accel_result.is_error()) {
            return core::Result<bool>{accel_result.error()};
        }

        const auto& accel = accel_result.value();
        int32_t abs_x = accel.x_mg() >= 0 ? accel.x_mg() : -accel.x_mg();
        int32_t abs_y = accel.y_mg() >= 0 ? accel.y_mg() : -accel.y_mg();
        int32_t abs_z = accel.z_mg() >= 0 ? accel.z_mg() : -accel.z_mg();

        auto threshold = static_cast<int32_t>(config_.shock_threshold_mg);
        bool shock = (abs_x > threshold) || (abs_y > threshold) || (abs_z > threshold);

        return core::Result<bool>{shock};
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    platform::IPlatformI2C* i2c_{};
    MPU6050Config config_{};
    bool initialized_{false};
};

} // namespace gridshield::hardware::sensors
