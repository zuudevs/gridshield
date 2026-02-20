/**
 * @file tamper.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Physical tamper detection with ISR support
 * @version 0.4
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "platform/platform.hpp"

namespace gridshield {
namespace hardware {

// ============================================================================
// TAMPER TYPES
// ============================================================================
enum class TamperType : uint8_t {
    None = 0,
    CasingOpened = 1,
    MagneticInterference = 2,
    TemperatureAnomaly = 3,
    VibrationDetected = 4,
    PowerCutAttempt = 5,
    PhysicalShock = 6
};

// ============================================================================
// TAMPER CONFIGURATION
// ============================================================================
struct TamperConfig {
    uint8_t sensor_pin;
    uint8_t backup_power_pin;
    uint16_t debounce_ms;
    uint8_t sensitivity;
    
    GS_CONSTEXPR TamperConfig() noexcept
        : sensor_pin(0), backup_power_pin(0), 
          debounce_ms(50), sensitivity(128) {}
};

// ============================================================================
// TAMPER DETECTOR INTERFACE
// ============================================================================
class ITamperDetector {
public:
    virtual ~ITamperDetector() noexcept = default;
    
    virtual core::Result<void> initialize(const TamperConfig& config, 
                                         platform::PlatformServices& platform) noexcept = 0;
    virtual core::Result<void> start() noexcept = 0;
    virtual core::Result<void> stop() noexcept = 0;
    
    // Call from main loop to process deferred debounce
    virtual core::Result<void> poll() noexcept = 0;
    
    GS_NODISCARD virtual bool is_tampered() const noexcept = 0;
    GS_NODISCARD virtual TamperType get_tamper_type() const noexcept = 0;
    GS_NODISCARD virtual core::timestamp_t get_tamper_timestamp() const noexcept = 0;
    
    virtual core::Result<void> acknowledge_tamper() noexcept = 0;
    virtual core::Result<void> reset() noexcept = 0;
};

// ============================================================================
// TAMPER DETECTOR IMPLEMENTATION
// ============================================================================
class TamperDetector final : public ITamperDetector {
public:
    TamperDetector() noexcept;
    ~TamperDetector() noexcept override = default;
    
    core::Result<void> initialize(const TamperConfig& config, 
                                 platform::PlatformServices& platform) noexcept override;
    core::Result<void> start() noexcept override;
    core::Result<void> stop() noexcept override;
    core::Result<void> poll() noexcept override;
    
    GS_NODISCARD bool is_tampered() const noexcept override;
    GS_NODISCARD TamperType get_tamper_type() const noexcept override;
    GS_NODISCARD core::timestamp_t get_tamper_timestamp() const noexcept override;
    
    core::Result<void> acknowledge_tamper() noexcept override;
    core::Result<void> reset() noexcept override;
    
private:
    static void interrupt_handler(void* context) noexcept;
    void confirm_tamper() noexcept;
    
    TamperConfig config_;
    platform::PlatformServices* platform_;
    
    // Volatile for ISR safety
    volatile bool is_tampered_;
    volatile bool pending_tamper_;           // ISR sets this, poll() processes it
    volatile TamperType tamper_type_;
    volatile core::timestamp_t tamper_timestamp_;
    volatile core::timestamp_t last_trigger_time_; // For debounce in poll()
    volatile bool initialized_;
};

} // namespace hardware
} // namespace gridshield