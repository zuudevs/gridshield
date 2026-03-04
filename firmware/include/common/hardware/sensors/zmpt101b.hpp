/**
 * @file zmpt101b.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief ZMPT101B AC voltage sensor driver
 * @version 1.0
 * @date 2026-02-25
 *
 * ADC-based AC voltage transformer sensor.
 * Measures AC voltage in millivolts using analog sampling.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace gridshield::hardware::sensors {

// ============================================================================
// ZMPT101B CONSTANTS
// ============================================================================
static constexpr uint32_t ZMPT101B_VCC_MV = 5000;
static constexpr uint32_t ZMPT101B_ZERO_POINT_MV = 2500;   // Vcc/2 at zero crossing
static constexpr uint16_t ZMPT101B_SAMPLES_PER_READ = 200; // Full AC cycle sampling
static constexpr uint32_t ZMPT101B_DEFAULT_RATIO = 500;    // Transformer turns ratio
static constexpr uint32_t ZMPT101B_RATIO_DIVISOR = 1000;   // For fixed-point ratio

// ============================================================================
// ZMPT101B CONFIGURATION
// ============================================================================
struct ZMPT101BConfig
{
    uint8_t adc_channel{};
    platform::ADCAttenuation attenuation{platform::ADCAttenuation::Db12};
    uint32_t voltage_ratio{ZMPT101B_DEFAULT_RATIO}; // x1000 fixed-point
    int32_t calibration_offset_mv{};

    GS_CONSTEXPR ZMPT101BConfig() noexcept = default;
};

// ============================================================================
// ZMPT101B DRIVER
// ============================================================================
class ZMPT101BDriver
{
public:
    ZMPT101BDriver() noexcept = default;

    /**
     * @brief Initialize the sensor with ADC.
     */
    core::Result<void> init(platform::IPlatformADC& adc, const ZMPT101BConfig& config) noexcept
    {
        adc_ = &adc;
        config_ = config;

        auto result = adc_->init(config_.adc_channel, config_.attenuation);
        if (result.is_error()) {
            return result;
        }

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read AC voltage in millivolts (RMS approximation from single sample).
     *
     * For production use, this should sample at least one full AC cycle
     * and compute true RMS. This simplified version reads a single sample.
     */
    core::Result<uint32_t> read_voltage_mv() noexcept
    {
        if (GS_UNLIKELY(!initialized_ || adc_ == nullptr)) {
            return core::Result<uint32_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        auto mv_result = adc_->read_mv(config_.adc_channel);
        if (mv_result.is_error()) {
            return core::Result<uint32_t>(mv_result.error());
        }

        uint32_t raw_mv = mv_result.value();

        // Compute voltage delta from zero crossing
        int32_t delta_mv = static_cast<int32_t>(raw_mv) -
                           static_cast<int32_t>(ZMPT101B_ZERO_POINT_MV) +
                           config_.calibration_offset_mv;

        // Apply transformer ratio (fixed-point: ratio/1000)
        uint32_t abs_delta = static_cast<uint32_t>(delta_mv >= 0 ? delta_mv : -delta_mv);
        uint32_t voltage_mv = (abs_delta * config_.voltage_ratio) / ZMPT101B_RATIO_DIVISOR;

        return core::Result<uint32_t>(voltage_mv);
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    platform::IPlatformADC* adc_{};
    ZMPT101BConfig config_{};
    bool initialized_{false};
};

} // namespace gridshield::hardware::sensors
