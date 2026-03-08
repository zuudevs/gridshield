/**
 * @file incident_report.hpp
 * @author Rafi Indra Pramudhito Zuhayr
 * @brief Incident report generation from security event logs
 * @version 3.1.0
 * @date 2026-03-06
 *
 * Correlates events from multiple security layers to generate
 * structured incident reports for forensic analysis.
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "forensics/event_logger.hpp"
#include "utils/gs_macros.hpp"

#include <cstdint>

namespace gridshield::forensics {

// ============================================================================
// ATTACK CLASSIFICATION
// ============================================================================
enum class AttackType : uint8_t
{
    Unknown = 0,
    PhysicalTampering = 1, // Physical layer only
    NetworkIntrusion = 2,  // Network layer only
    ConsumptionFraud = 3,  // Analytics layer only
    HybridAttack = 4,      // Multiple layers compromised
    SystemCompromise = 5,  // System-level breach
};

static constexpr size_t INCIDENT_MAX_EVENTS = 16;

// ============================================================================
// INCIDENT REPORT
// ============================================================================
struct IncidentReport
{
    bool valid{false};
    AttackType attack_type{AttackType::Unknown};
    SecurityEventSeverity max_severity{SecurityEventSeverity::Info};

    // Affected layers
    bool physical_layer_affected{false};
    bool network_layer_affected{false};
    bool analytics_layer_affected{false};

    // Statistics
    uint16_t total_events{0};
    uint16_t critical_events{0};
    uint16_t high_events{0};
    core::timestamp_t first_event_time{0};
    core::timestamp_t last_event_time{0};

    // Confidence (0-100) that this is a coordinated attack
    uint8_t confidence{0};

    // Snapshot of relevant events
    SecurityEvent events[INCIDENT_MAX_EVENTS]{};
    size_t event_count{0};

    GS_CONSTEXPR IncidentReport() noexcept = default;
};

// ============================================================================
// INCIDENT REPORT GENERATOR
// ============================================================================
class IncidentReportGenerator
{
public:
    IncidentReportGenerator() noexcept = default;

    /**
     * @brief Generate an incident report from the event logger.
     *
     * Analyzes all logged events, correlates across layers, classifies
     * the attack type, and computes a confidence score.
     */
    core::Result<IncidentReport> generate_report(const EventLogger& logger) const noexcept
    {
        if (logger.event_count() == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        IncidentReport report{};
        report.valid = true;
        report.total_events = static_cast<uint16_t>(logger.event_count());

        // Analyze all events
        bool has_physical = false;
        bool has_network = false;
        bool has_analytics = false;
        SecurityEventSeverity max_sev = SecurityEventSeverity::Info;

        size_t events_to_copy = (logger.event_count() < INCIDENT_MAX_EVENTS) ? logger.event_count()
                                                                             : INCIDENT_MAX_EVENTS;

        // Copy the most recent events to the report
        size_t start_idx = (logger.event_count() > INCIDENT_MAX_EVENTS)
                               ? (logger.event_count() - INCIDENT_MAX_EVENTS)
                               : 0;

        for (size_t i = 0; i < events_to_copy; ++i) {
            auto result = logger.get_event(start_idx + i);
            if (result.is_ok()) {
                report.events[i] = result.value();
                report.event_count = i + 1;
            }
        }

        // Analyze all events (not just copied ones)
        for (size_t i = 0; i < logger.event_count(); ++i) {
            auto result = logger.get_event(i);
            if (result.is_error()) {
                continue;
            }

            const auto& evt = result.value();

            // Track first/last times
            if (report.first_event_time == 0 || evt.timestamp < report.first_event_time) {
                report.first_event_time = evt.timestamp;
            }
            if (evt.timestamp > report.last_event_time) {
                report.last_event_time = evt.timestamp;
            }

            // Track max severity
            if (static_cast<uint8_t>(evt.severity) > static_cast<uint8_t>(max_sev)) {
                max_sev = evt.severity;
            }

            // Count severity levels
            if (evt.severity == SecurityEventSeverity::Critical) {
                ++report.critical_events;
            } else if (evt.severity == SecurityEventSeverity::High) {
                ++report.high_events;
            }

            // Track affected layers
            switch (evt.source_layer) {
                case SourceLayer::Physical:
                    has_physical = true;
                    break;
                case SourceLayer::Network:
                    has_network = true;
                    break;
                case SourceLayer::Analytics:
                    has_analytics = true;
                    break;
                default:
                    break;
            }
        }

        report.max_severity = max_sev;
        report.physical_layer_affected = has_physical;
        report.network_layer_affected = has_network;
        report.analytics_layer_affected = has_analytics;

        // Classify attack type
        report.attack_type = classify_attack(has_physical, has_network, has_analytics);

        // Compute confidence score
        report.confidence = compute_confidence(report);

        return core::Result<IncidentReport>(report);
    }

private:
    /**
     * @brief Classify the attack based on which layers are affected.
     */
    static AttackType classify_attack(bool physical, bool network, bool analytics) noexcept
    {
        int layer_count = (physical ? 1 : 0) + (network ? 1 : 0) + (analytics ? 1 : 0);

        if (layer_count >= 2) {
            return AttackType::HybridAttack;
        }
        if (physical) {
            return AttackType::PhysicalTampering;
        }
        if (network) {
            return AttackType::NetworkIntrusion;
        }
        if (analytics) {
            return AttackType::ConsumptionFraud;
        }
        return AttackType::Unknown;
    }

    /**
     * @brief Compute confidence that this is a real, coordinated attack.
     *
     * Higher confidence when:
     * - Multiple layers affected (+30)
     * - Critical severity events present (+25)
     * - High event count (+15)
     * - Events in close temporal proximity (+15)
     * - Multiple event types (+15)
     */
    static uint8_t compute_confidence(const IncidentReport& report) noexcept
    {
        uint8_t confidence = 0;

        // Multiple layers affected → likely coordinated
        int layers = (report.physical_layer_affected ? 1 : 0) +
                     (report.network_layer_affected ? 1 : 0) +
                     (report.analytics_layer_affected ? 1 : 0);
        if (layers >= 2) {
            confidence += 30;
        } else if (layers == 1) {
            confidence += 10;
        }

        // Critical events present
        if (report.critical_events > 0) {
            confidence += 25;
        } else if (report.high_events > 0) {
            confidence += 15;
        }

        // High event count
        if (report.total_events >= 10) {
            confidence += 15;
        } else if (report.total_events >= 5) {
            confidence += 10;
        }

        // Temporal proximity (events within 60 seconds)
        if (report.first_event_time > 0 && report.last_event_time > 0) {
            uint64_t duration = report.last_event_time - report.first_event_time;
            static constexpr uint64_t SIXTY_SECONDS_MS = 60000;
            if (duration > 0 && duration <= SIXTY_SECONDS_MS) {
                confidence += 15;
            }
        }

        // Cap at 100
        return (confidence > 100) ? 100 : confidence;
    }
};

} // namespace gridshield::forensics
