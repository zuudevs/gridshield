/**
 * @file sensor_manager.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Unified sensor data aggregation and management
 * @version 1.0
 * @date 2026-02-25
 *
 * Coordinates all sensor drivers, aggregates readings into
 * a unified SensorData struct, and converts to MeterReading
 * for the core system pipeline.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "hardware/sensors/acs712.hpp"
#include "hardware/sensors/ds18b20.hpp"
#include "hardware/sensors/mpu6050.hpp"
#include "hardware/sensors/pzem004t.hpp"
#include "hardware/sensors/zmpt101b.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace gridshield::hardware {

// ============================================================================
// SENSOR DATA — Aggregated readings from all sensors
// ============================================================================
struct SensorData
{
    uint32_t current_ma{};       // ACS712 current in mA
    uint32_t voltage_mv{};       // ZMPT101B voltage in mV
    uint32_t power_mw{};         // Calculated or PZEM power in mW
    uint32_t energy_wh{};        // PZEM energy in Wh
    int16_t temperature_c10{};   // DS18B20 temperature in 0.1°C
    int16_t accel_x_mg{};        // MPU6050 X-axis (milli-g)
    int16_t accel_y_mg{};        // MPU6050 Y-axis (milli-g)
    int16_t accel_z_mg{};        // MPU6050 Z-axis (milli-g)
    uint16_t frequency_hz10{};   // PZEM frequency in 0.1 Hz
    uint16_t power_factor_100{}; // PZEM PF * 100
    bool shock_detected{};       // MPU6050 threshold exceeded
    bool pzem_available{};       // Whether PZEM data is populated
};

// ============================================================================
// SENSOR MANAGER CONFIGURATION
// ============================================================================
struct SensorManagerConfig
{
    bool enable_acs712{false};
    bool enable_zmpt101b{false};
    bool enable_pzem004t{false};
    bool enable_ds18b20{false};
    bool enable_mpu6050{false};

    sensors::ACS712Config acs712_config{};
    sensors::ZMPT101BConfig zmpt101b_config{};
    sensors::PzemConfig pzem_config{};
    sensors::DS18B20Config ds18b20_config{};
    sensors::MPU6050Config mpu6050_config{};

    GS_CONSTEXPR SensorManagerConfig() noexcept = default;
};

// ============================================================================
// SENSOR MANAGER
// ============================================================================
class SensorManager
{
public:
    SensorManager() noexcept = default;

    /**
     * @brief Initialize all enabled sensors.
     */
    core::Result<void> initialize(platform::PlatformServices& platform,
                                  const SensorManagerConfig& config) noexcept
    {
        config_ = config;

        if (config_.enable_acs712 && platform.adc != nullptr) {
            auto result = acs712_.init(*platform.adc, config_.acs712_config);
            if (result.is_error()) {
                return result;
            }
        }

        if (config_.enable_zmpt101b && platform.adc != nullptr) {
            auto result = zmpt101b_.init(*platform.adc, config_.zmpt101b_config);
            if (result.is_error()) {
                return result;
            }
        }

        if (config_.enable_pzem004t && platform.uart != nullptr) {
            auto result = pzem_.init(*platform.uart, config_.pzem_config);
            if (result.is_error()) {
                return result;
            }
        }

        if (config_.enable_ds18b20 && platform.one_wire != nullptr) {
            auto result = ds18b20_.init(*platform.one_wire, config_.ds18b20_config);
            if (result.is_error()) {
                return result;
            }
        }

        if (config_.enable_mpu6050 && platform.i2c != nullptr) {
            auto result = mpu6050_.init(*platform.i2c, config_.mpu6050_config);
            if (result.is_error()) {
                return result;
            }
        }

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read all enabled sensors and aggregate data.
     */
    core::Result<SensorData> read_all() noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<SensorData>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        SensorData data{};

        // ACS712 — Current
        if (acs712_.is_initialized()) {
            auto result = acs712_.read_current_ma();
            if (result.is_ok()) {
                int32_t current_signed = result.value();
                data.current_ma =
                    static_cast<uint32_t>(current_signed >= 0 ? current_signed : -current_signed);
            }
        }

        // ZMPT101B — Voltage
        if (zmpt101b_.is_initialized()) {
            auto result = zmpt101b_.read_voltage_mv();
            if (result.is_ok()) {
                data.voltage_mv = result.value();
            }
        }

        // PZEM-004T — Full energy meter
        if (pzem_.is_initialized()) {
            auto result = pzem_.read();
            if (result.is_ok()) {
                auto& reading = result.value();
                data.voltage_mv = reading.voltage_mv; // Override ZMPT101B if PZEM available
                data.current_ma = reading.current_ma; // Override ACS712 if PZEM available
                data.power_mw = reading.power_mw;
                data.energy_wh = reading.energy_wh;
                data.frequency_hz10 = reading.frequency_hz10;
                data.power_factor_100 = reading.power_factor_100;
                data.pzem_available = true;
            }
        }

        // Calculate power from V*I if PZEM not available
        if (!data.pzem_available && data.voltage_mv > 0 && data.current_ma > 0) {
            static constexpr uint32_t MV_TO_V_DIVISOR = 1000;
            data.power_mw = (data.voltage_mv * data.current_ma) / MV_TO_V_DIVISOR;
        }

        // DS18B20 — Temperature
        if (ds18b20_.is_initialized()) {
            auto result = ds18b20_.read_temperature_c10();
            if (result.is_ok()) {
                data.temperature_c10 = result.value();
            }
        }

        // MPU6050 — Accelerometer + Shock detection
        if (mpu6050_.is_initialized()) {
            auto accel_result = mpu6050_.read_accel();
            if (accel_result.is_ok()) {
                auto& accel = accel_result.value();
                data.accel_x_mg = static_cast<int16_t>(accel.x_mg());
                data.accel_y_mg = static_cast<int16_t>(accel.y_mg());
                data.accel_z_mg = static_cast<int16_t>(accel.z_mg());
            }

            auto shock_result = mpu6050_.detect_shock();
            if (shock_result.is_ok()) {
                data.shock_detected = shock_result.value();
            }
        }

        return core::Result<SensorData>(data);
    }

    /**
     * @brief Convert SensorData to a MeterReading for the core pipeline.
     */
    static core::MeterReading to_meter_reading(const SensorData& data,
                                               core::timestamp_t timestamp) noexcept
    {
        core::MeterReading reading{};
        reading.voltage_mv = data.voltage_mv;
        reading.current_ma =
            static_cast<uint16_t>(data.current_ma <= UINT16_MAX ? data.current_ma : UINT16_MAX);
        reading.energy_wh = data.energy_wh;
        reading.timestamp = timestamp;
        return reading;
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    SensorManagerConfig config_{};
    sensors::ACS712Driver acs712_;
    sensors::ZMPT101BDriver zmpt101b_;
    sensors::PZEM004TDriver pzem_;
    sensors::DS18B20Driver ds18b20_;
    sensors::MPU6050Driver mpu6050_;
    bool initialized_{false};
};

} // namespace gridshield::hardware
