/**
 * @file tamper.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Tamper detection implementation with ISR handling
 * @version 0.4
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "hardware/tamper.hpp"

namespace gridshield {
namespace hardware {

TamperDetector::TamperDetector() noexcept
    : platform_(nullptr),
      is_tampered_(false),
      tamper_type_(TamperType::None),
      tamper_timestamp_(0),
      initialized_(false) {}

core::Result<void> TamperDetector::initialize(
    const TamperConfig& config, 
    platform::PlatformServices& platform) noexcept {
    
    if (GS_UNLIKELY(initialized_)) {
        return GS_MAKE_ERROR(core::ErrorCode::SystemAlreadyInitialized);
    }
    
    if (GS_UNLIKELY(!platform.is_valid())) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    config_ = config;
    platform_ = &platform;
    
    // Configure sensor pin
    GS_TRY(platform_->gpio->configure(
        config_.sensor_pin, 
        platform::PinMode::InputPullup
    ));
    
    // Configure backup power monitoring
    if (config_.backup_power_pin > 0) {
        GS_TRY(platform_->gpio->configure(
            config_.backup_power_pin,
            platform::PinMode::Input
        ));
    }
    
    initialized_ = true;
    return core::Result<void>();
}

core::Result<void> TamperDetector::start() noexcept {
    if (GS_UNLIKELY(!initialized_)) {
        return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    // Attach interrupt
    GS_TRY(platform_->interrupt->attach(
        config_.sensor_pin,
        platform::TriggerMode::Falling,
        &TamperDetector::interrupt_handler,
        this
    ));
    
    return platform_->interrupt->enable(config_.sensor_pin);
}

core::Result<void> TamperDetector::stop() noexcept {
    if (GS_UNLIKELY(!initialized_)) {
        return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    GS_TRY(platform_->interrupt->disable(config_.sensor_pin));
    return platform_->interrupt->detach(config_.sensor_pin);
}

bool TamperDetector::is_tampered() const noexcept {
    return is_tampered_;
}

TamperType TamperDetector::get_tamper_type() const noexcept {
    return tamper_type_;
}

core::timestamp_t TamperDetector::get_tamper_timestamp() const noexcept {
    return tamper_timestamp_;
}

core::Result<void> TamperDetector::acknowledge_tamper() noexcept {
    // Tamper acknowledged but not cleared (requires manual reset)
    return core::Result<void>();
}

core::Result<void> TamperDetector::reset() noexcept {
    if (GS_UNLIKELY(!initialized_)) {
        return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    is_tampered_ = false;
    tamper_type_ = TamperType::None;
    tamper_timestamp_ = 0;
    
    return core::Result<void>();
}

void TamperDetector::interrupt_handler(void* context) noexcept {
    auto* detector = static_cast<TamperDetector*>(context);
    if (GS_LIKELY(detector != nullptr)) {
        detector->handle_tamper_event();
    }
}

void TamperDetector::handle_tamper_event() noexcept {
    if (GS_UNLIKELY(!initialized_ || platform_ == nullptr)) {
        return;
    }
    
    // Read current sensor state
    auto read_result = platform_->gpio->read(config_.sensor_pin);
    if (GS_UNLIKELY(read_result.is_error())) {
        return;
    }
    
    const bool sensor_triggered = !read_result.value(); // Active low
    
    if (sensor_triggered && !is_tampered_) {
        // Debounce check
        platform_->time->delay_ms(config_.debounce_ms);
        
        // Re-read after debounce
        read_result = platform_->gpio->read(config_.sensor_pin);
        if (GS_UNLIKELY(read_result.is_error() || read_result.value())) {
            return; // False trigger or read error
        }
        
        // Tamper confirmed
        is_tampered_ = true;
        tamper_type_ = TamperType::CasingOpened;
        tamper_timestamp_ = platform_->time->get_timestamp_ms();
        
        // Check backup power status
        if (config_.backup_power_pin > 0) {
            auto power_result = platform_->gpio->read(config_.backup_power_pin);
            if (power_result.is_ok() && !power_result.value()) {
                tamper_type_ = TamperType::PowerCutAttempt;
            }
        }
    }
}

} // namespace hardware
} // namespace gridshield