/**
 * @file power_manager.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Power management and deep sleep controller
 * @version 1.0
 * @date 2026-02-25
 *
 * Manages ESP32 power states: active, light sleep, deep sleep.
 * Supports wake-on-tamper (GPIO), wake-on-timer, and adaptive
 * duty cycling based on power source and battery level.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <cstdint>

namespace gridshield::system {

// ============================================================================
// POWER CONSTANTS
// ============================================================================
static constexpr uint32_t POWER_MIN_SLEEP_MS = 100;
static constexpr uint32_t POWER_MAX_SLEEP_MS = 3600000;           // 1 hour max
static constexpr uint32_t POWER_DEFAULT_WAKE_INTERVAL_MS = 60000; // 1 min
static constexpr uint16_t POWER_BATTERY_FULL_MV = 4200;           // Li-ion full
static constexpr uint16_t POWER_BATTERY_LOW_MV = 3300;            // Low threshold
static constexpr uint16_t POWER_BATTERY_CRITICAL_MV = 3000;       // Critical
static constexpr uint8_t POWER_BATTERY_PERCENT_FULL = 100;
static constexpr uint8_t POWER_BATTERY_PERCENT_EMPTY = 0;

// ============================================================================
// POWER STATE
// ============================================================================
enum class PowerState : uint8_t
{
    Active = 0,
    LightSleep = 1,
    DeepSleep = 2,
    Hibernate = 3
};

// ============================================================================
// WAKE REASON
// ============================================================================
enum class WakeReason : uint8_t
{
    PowerOn = 0,
    Timer = 1,
    GPIO = 2, // Tamper pin
    UART = 3, // Serial wake
    Touchpad = 4,
    Unknown = 255
};

// ============================================================================
// POWER SOURCE
// ============================================================================
enum class PowerSource : uint8_t
{
    Mains = 0,   // AC mains powered
    Battery = 1, // Battery only
    Solar = 2,   // Solar + battery
    USB = 3      // USB debug power
};

// ============================================================================
// BATTERY STATUS
// ============================================================================
struct BatteryStatus
{
    uint16_t voltage_mv{};
    uint8_t percent{};
    bool charging{false};
    PowerSource source{PowerSource::Mains};

    GS_CONSTEXPR BatteryStatus() noexcept = default;
};

// ============================================================================
// POWER CONFIGURATION
// ============================================================================
struct PowerConfig
{
    uint32_t wake_interval_ms{POWER_DEFAULT_WAKE_INTERVAL_MS};
    uint8_t tamper_wake_pin{};
    bool enable_tamper_wake{true};
    bool enable_timer_wake{true};
    bool adaptive_duty_cycle{true};

    GS_CONSTEXPR PowerConfig() noexcept = default;
};

// ============================================================================
// POWER MANAGER
// ============================================================================
class PowerManager
{
public:
    PowerManager() noexcept = default;

    /**
     * @brief Initialize power management.
     */
    core::Result<void> init(platform::PlatformServices& platform,
                            const PowerConfig& config) noexcept
    {
        platform_ = &platform;
        config_ = config;
        state_ = PowerState::Active;
        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Request entering light sleep mode.
     *
     * CPU halts, RAM maintained. Wakes on timer or GPIO interrupt.
     * Returns after wakeup.
     */
    core::Result<WakeReason> enter_light_sleep(uint32_t duration_ms) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<WakeReason>{GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

        if (duration_ms < POWER_MIN_SLEEP_MS || duration_ms > POWER_MAX_SLEEP_MS) {
            return core::Result<WakeReason>{GS_MAKE_ERROR(core::ErrorCode::InvalidParameter)};
        }

        state_ = PowerState::LightSleep;
        last_sleep_duration_ms_ = duration_ms;

        // Real implementation: esp_light_sleep_start()
        // After wakeup, check wakeup cause

        state_ = PowerState::Active;
        return core::Result<WakeReason>{last_wake_reason_};
    }

    /**
     * @brief Request entering deep sleep mode.
     *
     * CPU and RAM powered off. Only RTC memory preserved.
     * Device reboots on wakeup — this function does not return.
     */
    core::Result<void> enter_deep_sleep(uint32_t duration_ms) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        if (duration_ms < POWER_MIN_SLEEP_MS || duration_ms > POWER_MAX_SLEEP_MS) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        state_ = PowerState::DeepSleep;
        last_sleep_duration_ms_ = duration_ms;

        // Real implementation:
        // 1. esp_sleep_enable_timer_wakeup(duration_ms * 1000)
        // 2. If tamper_wake: esp_sleep_enable_ext0_wakeup(pin, level)
        // 3. esp_deep_sleep_start() — never returns

        return core::Result<void>{};
    }

    /**
     * @brief Calculate optimal sleep duration based on battery and power source.
     *
     * Adaptive duty cycling: sleep longer on battery, shorter on mains.
     */
    GS_NODISCARD uint32_t calculate_adaptive_interval() const noexcept
    {
        if (!config_.adaptive_duty_cycle) {
            return config_.wake_interval_ms;
        }

        uint32_t interval = config_.wake_interval_ms;

        switch (battery_status_.source) {
            case PowerSource::Mains:
            case PowerSource::USB:
                // On mains: use configured interval directly
                break;

            case PowerSource::Battery: {
                // On battery: scale interval by remaining capacity
                if (battery_status_.voltage_mv <= POWER_BATTERY_CRITICAL_MV) {
                    // Critical: maximum sleep to preserve battery
                    static constexpr uint32_t CRITICAL_MULTIPLIER = 10;
                    interval *= CRITICAL_MULTIPLIER;
                } else if (battery_status_.voltage_mv <= POWER_BATTERY_LOW_MV) {
                    // Low: double the interval
                    static constexpr uint32_t LOW_MULTIPLIER = 2;
                    interval *= LOW_MULTIPLIER;
                }
                break;
            }

            case PowerSource::Solar:
                // Solar: slight increase when not charging
                if (!battery_status_.charging) {
                    static constexpr uint32_t SOLAR_NO_CHARGE_MULTIPLIER_NUM = 3;
                    static constexpr uint32_t SOLAR_NO_CHARGE_MULTIPLIER_DEN = 2;
                    interval = (interval * SOLAR_NO_CHARGE_MULTIPLIER_NUM) /
                               SOLAR_NO_CHARGE_MULTIPLIER_DEN;
                }
                break;
        }

        // Clamp to max
        if (interval > POWER_MAX_SLEEP_MS) {
            interval = POWER_MAX_SLEEP_MS;
        }

        return interval;
    }

    /**
     * @brief Read battery status from ADC (if available).
     */
    core::Result<BatteryStatus> read_battery_status() noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<BatteryStatus>{
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

        // In real implementation: read ADC pin for battery voltage
        // For now, return stored status
        return core::Result<BatteryStatus>{battery_status_};
    }

    // ---- Getters ----
    GS_NODISCARD PowerState get_state() const noexcept
    {
        return state_;
    }
    GS_NODISCARD WakeReason get_wake_reason() const noexcept
    {
        return last_wake_reason_;
    }
    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

    // ---- Test helpers ----
    void set_battery_status(const BatteryStatus& status) noexcept
    {
        battery_status_ = status;
    }

    void set_wake_reason(WakeReason reason) noexcept
    {
        last_wake_reason_ = reason;
    }

private:
    platform::PlatformServices* platform_{};
    PowerConfig config_{};
    BatteryStatus battery_status_{};
    PowerState state_{PowerState::Active};
    WakeReason last_wake_reason_{WakeReason::PowerOn};
    uint32_t last_sleep_duration_ms_{};
    bool initialized_{false};
};

} // namespace gridshield::system
