/**
 * @file detector.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "../../include/analytics/detector.hpp"

namespace gridshield::analytics {

AnomalyDetector::AnomalyDetector() noexcept 
    : initialized_(false) {}

core::Result<void> AnomalyDetector::initialize(const ConsumptionProfile& baseline_profile) noexcept {
    if (initialized_) {
        return MAKE_ERROR(core::ErrorCode::SystemAlreadyInitialized);
    }
    
    profile_ = baseline_profile;
    recent_readings_.clear();
    initialized_ = true;
    
    return core::Result<void>();
}

core::Result<void> AnomalyDetector::update_profile(const core::MeterReading& reading) noexcept {
    if (!initialized_) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    // Add to recent readings buffer
    if (recent_readings_.full()) {
        core::MeterReading temp;
        recent_readings_.pop(temp); // Remove oldest
    }
    recent_readings_.push(reading);
    
    // Update hourly averages (simplified - in production use proper time windowing)
    if (recent_readings_.size() >= 10) {
        uint64_t sum = 0;
        for (size_t i = 0; i < recent_readings_.size(); ++i) {
            sum += recent_readings_[i].energy_wh;
        }
        
        size_t hour_index = (reading.timestamp / 3600000) % PROFILE_HISTORY_SIZE;
        profile_.hourly_avg_wh[hour_index] = static_cast<uint32_t>(sum / recent_readings_.size());
        
        // Update daily average
        sum = 0;
        for (size_t i = 0; i < PROFILE_HISTORY_SIZE; ++i) {
            sum += profile_.hourly_avg_wh[i];
        }
        profile_.daily_avg_wh = static_cast<uint32_t>(sum / PROFILE_HISTORY_SIZE);
        
        // Increase confidence
        if (profile_.profile_confidence < 100) {
            profile_.profile_confidence++;
        }
    }
    
    return core::Result<void>();
}

core::Result<AnomalyReport> AnomalyDetector::analyze(const core::MeterReading& reading) noexcept {
    if (!initialized_) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    AnomalyReport report;
    report.timestamp = reading.timestamp;
    report.current_value = reading.energy_wh;
    report.expected_value = calculate_expected_value(reading.timestamp);
    
    // Check for zero consumption
    if (reading.energy_wh == 0 && report.expected_value > 100) {
        report.type = AnomalyType::ZeroConsumption;
        report.severity = AnomalySeverity::Critical;
        report.confidence = 95;
        report.deviation_percent = 100;
        return core::Result<AnomalyReport>(report);
    }
    
    // Calculate deviation
    if (report.expected_value > 0) {
        int32_t diff = static_cast<int32_t>(reading.energy_wh) - 
                      static_cast<int32_t>(report.expected_value);
        report.deviation_percent = static_cast<uint32_t>(
            (diff * 100) / static_cast<int32_t>(report.expected_value)
        );
        
        if (diff < 0) {
            report.deviation_percent = static_cast<uint32_t>(-diff * 100 / 
                                                             static_cast<int32_t>(report.expected_value));
        }
    } else {
        report.deviation_percent = 0;
    }
    
    // Detect anomalies based on deviation
    if (report.deviation_percent > profile_.variance_threshold) {
        if (reading.energy_wh < report.expected_value) {
            report.type = AnomalyType::UnexpectedDrop;
        } else {
            report.type = AnomalyType::UnexpectedSpike;
        }
        
        report.severity = calculate_severity(report.deviation_percent);
        report.confidence = static_cast<uint16_t>(
            profile_.profile_confidence > 50 ? 
            (profile_.profile_confidence - 50) * 2 : 0
        );
    } else {
        report.type = AnomalyType::None;
        report.severity = AnomalySeverity::None;
        report.confidence = 0;
    }
    
    return core::Result<AnomalyReport>(report);
}

const ConsumptionProfile& AnomalyDetector::get_profile() const noexcept {
    return profile_;
}

core::Result<void> AnomalyDetector::reset_profile() noexcept {
    if (!initialized_) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    profile_ = ConsumptionProfile();
    recent_readings_.clear();
    
    return core::Result<void>();
}

AnomalySeverity AnomalyDetector::calculate_severity(uint32_t deviation_percent) const noexcept {
    if (deviation_percent >= 80) {
        return AnomalySeverity::Critical;
    } else if (deviation_percent >= 60) {
        return AnomalySeverity::High;
    } else if (deviation_percent >= 40) {
        return AnomalySeverity::Medium;
    } else if (deviation_percent >= 20) {
        return AnomalySeverity::Low;
    }
    return AnomalySeverity::None;
}

uint32_t AnomalyDetector::calculate_expected_value(core::timestamp_t timestamp) const noexcept {
    // Use hourly profile
    size_t hour_index = (timestamp / 3600000) % PROFILE_HISTORY_SIZE;
    uint32_t hourly_expected = profile_.hourly_avg_wh[hour_index];
    
    if (hourly_expected > 0) {
        return hourly_expected;
    }
    
    // Fallback to daily average
    return profile_.daily_avg_wh;
}

} // namespace gridshield::analytics