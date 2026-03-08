/**
 * @file alert_dispatcher.hpp
 * @author Rafi Indra Pramudhito Zuhayr
 * @brief Configurable alert dispatch rules for security events
 * @version 3.3.0
 * @date 2026-03-08
 *
 * Provides a rule-based dispatcher that maps security events
 * to outbound alert actions (HTTP POST, MQTT publish, serial log).
 * Builds on EventLogger from v3.1.0 to enable flexible alerting
 * without hardcoded dispatch logic.
 *
 * Zero-heap, header-only, suitable for embedded deployment.
 */

#pragma once

#include "core/error.hpp"
#include "event_logger.hpp"
#include "utils/gs_macros.hpp"


#include <cstdint>

namespace gridshield::forensics {

// ============================================================================
// ALERT ACTION — what to do when a rule matches
// ============================================================================
enum class AlertAction : uint8_t
{
    None = 0,
    LogOnly = 1,     // Log to serial/console only
    HttpPost = 2,    // Send HTTP POST to configured endpoint
    MqttPublish = 3, // Publish to MQTT topic
    All = 4,         // All actions (log + HTTP + MQTT)
};

// ============================================================================
// ALERT RULE — maps event type + minimum severity to an action
// ============================================================================
struct AlertRule
{
    SecurityEventType event_type{SecurityEventType::None};
    SecurityEventSeverity min_severity{SecurityEventSeverity::Info};
    AlertAction action{AlertAction::None};
    bool active{false};

    GS_CONSTEXPR AlertRule() noexcept = default;

    GS_CONSTEXPR
    AlertRule(SecurityEventType type, SecurityEventSeverity severity, AlertAction act) noexcept
        : event_type(type), min_severity(severity), action(act), active(true)
    {}

    GS_CONSTEXPR bool is_valid() const noexcept
    {
        return active && event_type != SecurityEventType::None && action != AlertAction::None;
    }
};

// ============================================================================
// DISPATCH RESULT — returned when a rule matches an event
// ============================================================================
struct DispatchResult
{
    AlertAction action{AlertAction::None};
    SecurityEventType event_type{SecurityEventType::None};
    SecurityEventSeverity severity{SecurityEventSeverity::Info};
    bool matched{false};

    GS_CONSTEXPR DispatchResult() noexcept = default;
};

// ============================================================================
// ALERT DISPATCHER — rule-based security event dispatcher
// ============================================================================
static constexpr size_t MAX_ALERT_RULES = 16;

class AlertDispatcher
{
public:
    AlertDispatcher() noexcept = default;

    /**
     * @brief Add a dispatch rule.
     * @param type Event type to match
     * @param min_severity Minimum severity to trigger the rule
     * @param action Action to take when the rule matches
     * @return Success or error if rule buffer is full
     */
    core::Result<void> add_rule(SecurityEventType type,
                                SecurityEventSeverity min_severity,
                                AlertAction action) noexcept
    {
        if (count_ >= MAX_ALERT_RULES) {
            return GS_MAKE_ERROR(core::ErrorCode::BufferFull);
        }

        rules_[count_] = AlertRule(type, min_severity, action);
        ++count_;
        return core::Result<void>::ok();
    }

    /**
     * @brief Dispatch a security event against all rules.
     * Returns the first matching rule's action. If multiple
     * rules match, the highest-priority action (All > Mqtt > Http > Log)
     * is returned.
     */
    GS_NODISCARD DispatchResult dispatch(const SecurityEvent& event) const noexcept
    {
        DispatchResult best{};

        for (size_t i = 0; i < count_; ++i) {
            const auto& rule = rules_[i];
            if (!rule.is_valid()) {
                continue;
            }

            // Match event type
            if (rule.event_type != event.event_type) {
                continue;
            }

            // Match minimum severity
            if (static_cast<uint8_t>(event.severity) < static_cast<uint8_t>(rule.min_severity)) {
                continue;
            }

            // Take the highest-priority action among matching rules
            if (static_cast<uint8_t>(rule.action) > static_cast<uint8_t>(best.action)) {
                best.action = rule.action;
                best.event_type = event.event_type;
                best.severity = event.severity;
                best.matched = true;
            }
        }

        return best;
    }

    /**
     * @brief Check if any rule matches the given event.
     */
    GS_NODISCARD bool matches(const SecurityEvent& event) const noexcept
    {
        return dispatch(event).matched;
    }

    /**
     * @brief Get the number of configured rules.
     */
    GS_NODISCARD size_t rule_count() const noexcept
    {
        return count_;
    }

    /**
     * @brief Get a rule by index.
     */
    GS_NODISCARD core::Result<AlertRule> get_rule(size_t index) const noexcept
    {
        if (index >= count_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        return core::Result<AlertRule>(rules_[index]);
    }

    /**
     * @brief Remove a rule by index.
     */
    core::Result<void> remove_rule(size_t index) noexcept
    {
        if (index >= count_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        // Shift remaining rules down
        for (size_t i = index; i + 1 < count_; ++i) {
            rules_[i] = rules_[i + 1];
        }
        rules_[count_ - 1] = AlertRule{};
        --count_;

        return core::Result<void>::ok();
    }

    /**
     * @brief Clear all rules.
     */
    void clear_rules() noexcept
    {
        count_ = 0;
        for (auto& rule : rules_) {
            rule = AlertRule{};
        }
    }

private:
    AlertRule rules_[MAX_ALERT_RULES]{};
    size_t count_{0};
};

} // namespace gridshield::forensics
