/**
 * @file detector.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Consumption anomaly detection with profile learning
 * @version 0.4
 * @date 2026-02-03
 *
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include <array>

namespace gridshield::analytics {

constexpr size_t PROFILE_HISTORY_SIZE = 24; // 24 hours
constexpr uint16_t DEFAULT_VARIANCE_THRESHOLD = 30;
constexpr size_t MAX_RECENT_READINGS = 100;
constexpr uint32_t MS_PER_HOUR = 3600000;
constexpr uint8_t CONFIDENCE_MAX = 100;
constexpr uint8_t CONFIDENCE_BASELINE = 50;
constexpr uint32_t DEVIATION_FULL = 100;
constexpr size_t MIN_LEARNING_READINGS = 10;

// Severity deviation thresholds (percent)
constexpr uint32_t SEVERITY_CRITICAL_THRESHOLD = 80;
constexpr uint32_t SEVERITY_HIGH_THRESHOLD = 60;
constexpr uint32_t SEVERITY_MEDIUM_THRESHOLD = 40;
constexpr uint32_t SEVERITY_LOW_THRESHOLD = 20;

// Confidence score for high-certainty detections
constexpr uint16_t CONFIDENCE_HIGH = 95;

// ============================================================================
// ANOMALY CLASSIFICATION
// ============================================================================
enum class AnomalyType : uint8_t
{
    None = 0,
    UnexpectedDrop = 1,
    UnexpectedSpike = 2,
    PatternDeviation = 3,
    ZeroConsumption = 4,
    ErraticBehavior = 5
};

enum class AnomalySeverity : uint8_t
{
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

// ============================================================================
// CONSUMPTION PROFILE
// ============================================================================
struct ConsumptionProfile
{
    std::array<uint32_t, PROFILE_HISTORY_SIZE> hourly_avg_wh{};
    uint32_t daily_avg_wh{};
    uint32_t weekly_avg_wh{};
    uint16_t variance_threshold{DEFAULT_VARIANCE_THRESHOLD};
    uint8_t profile_confidence{};
    uint8_t reserved{};

    GS_CONSTEXPR ConsumptionProfile() noexcept = default;
};

// ============================================================================
// ANOMALY REPORT
// ============================================================================
struct AnomalyReport
{
    core::timestamp_t timestamp{};
    AnomalyType type{};
    AnomalySeverity severity{};
    uint16_t confidence{};
    uint32_t current_value{};
    uint32_t expected_value{};
    uint32_t deviation_percent{};

    GS_CONSTEXPR AnomalyReport() noexcept = default;
};

// ============================================================================
// ANOMALY DETECTOR INTERFACE
// ============================================================================
class IAnomalyDetector
{
public:
    virtual ~IAnomalyDetector() noexcept = default;

    virtual core::Result<void> initialize(const ConsumptionProfile& baseline_profile) noexcept = 0;
    virtual core::Result<void> update_profile(const core::MeterReading& reading) noexcept = 0;

    virtual core::Result<AnomalyReport> analyze(const core::MeterReading& reading) noexcept = 0;

    GS_NODISCARD virtual const ConsumptionProfile& get_profile() const noexcept = 0;
    virtual core::Result<void> reset_profile() noexcept = 0;
};

// ============================================================================
// ANOMALY DETECTOR IMPLEMENTATION
// ============================================================================
class AnomalyDetector final : public IAnomalyDetector
{
public:
    AnomalyDetector() noexcept = default;
    ~AnomalyDetector() noexcept override = default;

    core::Result<void> initialize(const ConsumptionProfile& baseline_profile) noexcept override;
    core::Result<void> update_profile(const core::MeterReading& reading) noexcept override;

    core::Result<AnomalyReport> analyze(const core::MeterReading& reading) noexcept override;

    GS_NODISCARD const ConsumptionProfile& get_profile() const noexcept override;
    core::Result<void> reset_profile() noexcept override;

private:
    GS_NODISCARD static AnomalySeverity calculate_severity(uint32_t deviation_percent) noexcept;
    GS_NODISCARD uint32_t calculate_expected_value(core::timestamp_t timestamp) const noexcept;

    ConsumptionProfile profile_;
    core::StaticBuffer<core::MeterReading, MAX_RECENT_READINGS> recent_readings_;
    bool initialized_{false};
};

// ============================================================================
// CROSS-LAYER VALIDATION
// ============================================================================
struct CrossLayerValidation
{
    bool physical_tamper_detected{false};
    bool network_anomaly_detected{false};
    bool consumption_anomaly_detected{false};
    core::timestamp_t validation_timestamp{};

    GS_CONSTEXPR CrossLayerValidation() noexcept = default;

    GS_NODISCARD bool requires_investigation() const noexcept
    {
        return (physical_tamper_detected && consumption_anomaly_detected) ||
               (network_anomaly_detected && consumption_anomaly_detected);
    }

    GS_NODISCARD core::Priority get_priority() const noexcept
    {
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