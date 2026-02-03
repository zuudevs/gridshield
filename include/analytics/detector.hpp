/**
 * @file detector.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"

namespace gridshield::analytics {

constexpr size_t PROFILE_HISTORY_SIZE = 24; // 24 hours of hourly averages

enum class AnomalyType : uint8_t {
    None = 0,
    UnexpectedDrop = 1,
    UnexpectedSpike = 2,
    PatternDeviation = 3,
    ZeroConsumption = 4,
    ErraticBehavior = 5
};

enum class AnomalySeverity : uint8_t {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

struct ConsumptionProfile {
    uint32_t hourly_avg_wh[PROFILE_HISTORY_SIZE];
    uint32_t daily_avg_wh;
    uint32_t weekly_avg_wh;
    uint16_t variance_threshold;
    uint8_t profile_confidence;
    uint8_t reserved;
    
    constexpr ConsumptionProfile() noexcept 
        : hourly_avg_wh{}, daily_avg_wh(0), weekly_avg_wh(0),
          variance_threshold(30), profile_confidence(0), reserved(0) {}
};

struct AnomalyReport {
    core::timestamp_t timestamp;
    AnomalyType type;
    AnomalySeverity severity;
    uint16_t confidence;
    uint32_t current_value;
    uint32_t expected_value;
    uint32_t deviation_percent;
    
    constexpr AnomalyReport() noexcept
        : timestamp(0), type(AnomalyType::None), severity(AnomalySeverity::None),
          confidence(0), current_value(0), expected_value(0), deviation_percent(0) {}
};

class IAnomalyDetector {
public:
    virtual ~IAnomalyDetector() = default;
    
    virtual core::Result<void> initialize(const ConsumptionProfile& baseline_profile) noexcept = 0;
    virtual core::Result<void> update_profile(const core::MeterReading& reading) noexcept = 0;
    
    virtual core::Result<AnomalyReport> analyze(const core::MeterReading& reading) noexcept = 0;
    
    virtual const ConsumptionProfile& get_profile() const noexcept = 0;
    virtual core::Result<void> reset_profile() noexcept = 0;
};

class AnomalyDetector : public IAnomalyDetector {
public:
    AnomalyDetector() noexcept;
    ~AnomalyDetector() override = default;
    
    core::Result<void> initialize(const ConsumptionProfile& baseline_profile) noexcept override;
    core::Result<void> update_profile(const core::MeterReading& reading) noexcept override;
    
    core::Result<AnomalyReport> analyze(const core::MeterReading& reading) noexcept override;
    
    const ConsumptionProfile& get_profile() const noexcept override;
    core::Result<void> reset_profile() noexcept override;
    
private:
    AnomalySeverity calculate_severity(uint32_t deviation_percent) const noexcept;
    uint32_t calculate_expected_value(core::timestamp_t timestamp) const noexcept;
    
    ConsumptionProfile profile_;
    core::StaticBuffer<core::MeterReading, 100> recent_readings_;
    bool initialized_;
};

struct CrossLayerValidation {
    bool physical_tamper_detected;
    bool network_anomaly_detected;
    bool consumption_anomaly_detected;
    core::timestamp_t validation_timestamp;
    
    constexpr CrossLayerValidation() noexcept
        : physical_tamper_detected(false),
          network_anomaly_detected(false),
          consumption_anomaly_detected(false),
          validation_timestamp(0) {}
    
    constexpr bool requires_investigation() const noexcept {
        return (physical_tamper_detected && consumption_anomaly_detected) ||
               (network_anomaly_detected && consumption_anomaly_detected);
    }
    
    constexpr core::Priority get_priority() const noexcept {
        if (physical_tamper_detected && consumption_anomaly_detected && network_anomaly_detected) {
            return core::Priority::Emergency;
        }
        if (physical_tamper_detected || 
            (consumption_anomaly_detected && network_anomaly_detected)) {
            return core::Priority::Critical;
        }
        if (consumption_anomaly_detected || network_anomaly_detected) {
            return core::Priority::High;
        }
        return core::Priority::Normal;
    }
};

} // namespace gridshield::analytics