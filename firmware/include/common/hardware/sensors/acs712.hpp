/**
 * @file acs712.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief ACS712 Hall-effect current sensor driver
 * @version 1.0
 * @date 2026-02-25
 *
 * ADC-based current sensor supporting 5A, 20A, and 30A variants.
 * Measures AC/DC current in milliamps using analog voltage reading.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace gridshield::hardware::sensors {

// ============================================================================
// ACS712 CONSTANTS
// ============================================================================

/**
 * @brief ACS712 sensitivity in mV/A for each variant.
 * These map to the physical chip model variant.
 */
enum class ACS712Variant : uint8_t
{
    ACS712_5A = 0,  // 185 mV/A
    ACS712_20A = 1, // 100 mV/A
    ACS712_30A = 2  // 66 mV/A
};

static constexpr uint32_t ACS712_VCC_MV = 5000;          // Supply voltage
static constexpr uint32_t ACS712_ZERO_CURRENT_MV = 2500; // Vcc/2 at zero current
static constexpr uint16_t ACS712_SAMPLES_PER_READ = 100; // Averaging sample count

/// Sensitivity table in mV/A (indexed by ACS712Variant)
static constexpr uint16_t ACS712_SENSITIVITY_MV_PER_A[] = {185, 100, 66};

// ============================================================================
// ACS712 CONFIGURATION
// ============================================================================
struct ACS712Config
{
    uint8_t adc_channel{};
    ACS712Variant variant{ACS712Variant::ACS712_20A};
    platform::ADCAttenuation attenuation{platform::ADCAttenuation::Db12};
    int32_t calibration_offset_mv{}; // User-adjustable offset

    GS_CONSTEXPR ACS712Config() noexcept = default;
};

// ============================================================================
// ACS712 DRIVER
// ============================================================================
class ACS712Driver
{
public:
    ACS712Driver() noexcept = default;

    /**
     * @brief Initialize the sensor with ADC.
     */
    core::Result<void> init(platform::IPlatformADC& adc, const ACS712Config& config) noexcept
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
     * @brief Read current in milliamps (signed: positive = forward, negative = reverse).
     */
    core::Result<int32_t> read_current_ma() noexcept
    {
        if (GS_UNLIKELY(!initialized_ || adc_ == nullptr)) {
            return core::Result<int32_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        auto mv_result = adc_->read_mv(config_.adc_channel);
        if (mv_result.is_error()) {
            return core::Result<int32_t>(mv_result.error());
        }

        uint32_t raw_mv = mv_result.value();

        // Calculate current: I = (Vout - Vref) / sensitivity
        int32_t delta_mv = static_cast<int32_t>(raw_mv) -
                           static_cast<int32_t>(ACS712_ZERO_CURRENT_MV) +
                           config_.calibration_offset_mv;

        uint16_t sensitivity = ACS712_SENSITIVITY_MV_PER_A[static_cast<uint8_t>(config_.variant)];

        // Convert to mA: (delta_mv * 1000) / sensitivity
        static constexpr int32_t MA_PER_A = 1000;
        int32_t current_ma = (delta_mv * MA_PER_A) / static_cast<int32_t>(sensitivity);

        return core::Result<int32_t>(current_ma);
    }

    /**
     * @brief Check if the sensor is initialized.
     */
    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    platform::IPlatformADC* adc_{};
    ACS712Config config_{};
    bool initialized_{false};
};

} // namespace gridshield::hardware::sensors
