/**
 * @file tamper.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Physical tamper detection with interrupt-driven architecture
 * @version 0.3
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
    
    constexpr TamperConfig() noexcept 
        : sensor_pin(0), backup_power_pin(0), 
          debounce_ms(50), sensitivity(128) {}
};

// ============================================================================
// TAMPER DETECTOR INTERFACE
// ============================================================================
class ITamperDetector {
public:
    virtual ~ITamperDetector() = default;
    
    virtual core::Result<void> initialize(const TamperConfig& config, 
                                         platform::PlatformServices& platform) noexcept = 0;
    virtual core::Result<void> start() noexcept = 0;
    virtual core::Result<void> stop() noexcept = 0;
    
    virtual bool is_tampered() const noexcept = 0;
    virtual TamperType get_tamper_type() const noexcept = 0;
    virtual core::timestamp_t get_tamper_timestamp() const noexcept = 0;
    
    virtual core::Result<void> acknowledge_tamper() noexcept = 0;
    virtual core::Result<void> reset() noexcept = 0;
};

// ============================================================================
// TAMPER DETECTOR IMPLEMENTATION
// ============================================================================
class TamperDetector : public ITamperDetector {
public:
    TamperDetector() noexcept;
    ~TamperDetector() override = default;
    
    core::Result<void> initialize(const TamperConfig& config, 
                                 platform::PlatformServices& platform) noexcept override;
    core::Result<void> start() noexcept override;
    core::Result<void> stop() noexcept override;
    
    bool is_tampered() const noexcept override;
    TamperType get_tamper_type() const noexcept override;
    core::timestamp_t get_tamper_timestamp() const noexcept override;
    
    core::Result<void> acknowledge_tamper() noexcept override;
    core::Result<void> reset() noexcept override;
    
private:
    static void interrupt_handler(void* context) noexcept;
    void handle_tamper_event() noexcept;
    
    TamperConfig config_;
    platform::PlatformServices* platform_;
    
    // Volatile for ISR safety
    volatile bool is_tampered_;
    volatile TamperType tamper_type_;
    volatile core::timestamp_t tamper_timestamp_;
    volatile bool initialized_;
};

} // namespace hardware
} // namespace gridshield