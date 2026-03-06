/**
 * @file event_logger.hpp
 * @author Rafi Indra Pramudhito Zuhayr
 * @brief Security event logging for forensic analysis
 * @version 3.1.0
 * @date 2026-03-06
 *
 * Provides a circular buffer of SecurityEvent records for
 * post-incident forensic analysis and attack timeline reconstruction.
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "utils/gs_macros.hpp"

#include <cstdint>
#include <cstring>

namespace gridshield::forensics {

// ============================================================================
// SECURITY EVENT TYPES
// ============================================================================
enum class SecurityEventType : uint8_t
{
    None = 0,

    // Physical layer events
    CasingOpened = 1,
    MagneticInterference = 2,
    PowerCutAttempt = 3,
    PhysicalShock = 4,

    // Network layer events
    SignatureVerifyFailed = 10,
    ReplayAttackDetected = 11,
    PacketIntegrityFailed = 12,
    UnauthorizedDevice = 13,

    // Analytics layer events
    AnomalyDetected = 20,
    ConsumptionDrop = 21,
    ConsumptionSpike = 22,
    ZeroConsumption = 23,
    ProfileDeviation = 24,

    // System events
    KeyRotation = 30,
    FirmwareUpdate = 31,
    SystemReboot = 32,
    WatchdogReset = 33,
    ConfigChange = 34,
};

enum class SecurityEventSeverity : uint8_t
{
    Info = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4,
};

enum class SourceLayer : uint8_t
{
    System = 0,
    Physical = 1,
    Network = 2,
    Analytics = 3,
    CrossLayer = 4,
};

// ============================================================================
// SECURITY EVENT
// ============================================================================
static constexpr size_t EVENT_DETAILS_MAX_LENGTH = 64;

struct SecurityEvent
{
    core::timestamp_t timestamp{0};
    SecurityEventType event_type{SecurityEventType::None};
    SecurityEventSeverity severity{SecurityEventSeverity::Info};
    SourceLayer source_layer{SourceLayer::System};
    uint8_t reserved{0};
    char details[EVENT_DETAILS_MAX_LENGTH]{};

    GS_CONSTEXPR SecurityEvent() noexcept = default;

    GS_CONSTEXPR bool is_valid() const noexcept
    {
        return event_type != SecurityEventType::None && timestamp > 0;
    }
};

// ============================================================================
// EVENT LOGGER — Circular buffer for security events
// ============================================================================
static constexpr size_t EVENT_LOG_CAPACITY = 64;

class EventLogger
{
public:
    EventLogger() noexcept = default;

    /**
     * @brief Log a new security event.
     * If buffer is full, the oldest event is overwritten (circular).
     */
    core::Result<void> log_event(SecurityEventType type,
                                 SecurityEventSeverity severity,
                                 SourceLayer layer,
                                 core::timestamp_t timestamp,
                                 const char* details = nullptr) noexcept
    {
        auto& slot = events_[write_index_];
        slot.timestamp = timestamp;
        slot.event_type = type;
        slot.severity = severity;
        slot.source_layer = layer;
        slot.reserved = 0;

        if (details != nullptr) {
            std::strncpy(slot.details, details, EVENT_DETAILS_MAX_LENGTH - 1);
            slot.details[EVENT_DETAILS_MAX_LENGTH - 1] = '\0';
        } else {
            slot.details[0] = '\0';
        }

        write_index_ = (write_index_ + 1) % EVENT_LOG_CAPACITY;
        if (count_ < EVENT_LOG_CAPACITY) {
            ++count_;
        }

        return core::Result<void>::ok();
    }

    /**
     * @brief Get the total number of logged events.
     */
    GS_NODISCARD size_t event_count() const noexcept
    {
        return count_;
    }

    /**
     * @brief Get an event by index (0 = oldest in buffer).
     */
    GS_NODISCARD core::Result<SecurityEvent> get_event(size_t index) const noexcept
    {
        if (index >= count_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        size_t actual_index = 0;
        if (count_ < EVENT_LOG_CAPACITY) {
            actual_index = index;
        } else {
            actual_index = (write_index_ + index) % EVENT_LOG_CAPACITY;
        }
        return core::Result<SecurityEvent>(events_[actual_index]);
    }

    /**
     * @brief Get events within a time range.
     * @param start Start timestamp (inclusive)
     * @param end End timestamp (inclusive)
     * @param out Output buffer
     * @param max_out Maximum events to return
     * @return Number of events found
     */
    size_t get_timeline(core::timestamp_t start,
                        core::timestamp_t end,
                        SecurityEvent* out,
                        size_t max_out) const noexcept
    {
        size_t found = 0;
        for (size_t i = 0; i < count_ && found < max_out; ++i) {
            auto result = get_event(i);
            if (result.is_ok()) {
                const auto& evt = result.value();
                if (evt.timestamp >= start && evt.timestamp <= end) {
                    out[found++] = evt;
                }
            }
        }
        return found;
    }

    /**
     * @brief Count events of a specific type.
     */
    GS_NODISCARD size_t count_by_type(SecurityEventType type) const noexcept
    {
        size_t count = 0;
        for (size_t i = 0; i < count_; ++i) {
            auto result = get_event(i);
            if (result.is_ok() && result.value().event_type == type) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Count events at or above a severity level.
     */
    GS_NODISCARD size_t count_by_severity(SecurityEventSeverity min_severity) const noexcept
    {
        size_t count = 0;
        for (size_t i = 0; i < count_; ++i) {
            auto result = get_event(i);
            if (result.is_ok() && static_cast<uint8_t>(result.value().severity) >=
                                      static_cast<uint8_t>(min_severity)) {
                ++count;
            }
        }
        return count;
    }

    /**
     * @brief Get the most recent event.
     */
    GS_NODISCARD core::Result<SecurityEvent> latest() const noexcept
    {
        if (count_ == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        return get_event(count_ - 1);
    }

    /**
     * @brief Clear all logged events.
     */
    void clear() noexcept
    {
        count_ = 0;
        write_index_ = 0;
        for (auto& evt : events_) {
            evt = SecurityEvent{};
        }
    }

private:
    SecurityEvent events_[EVENT_LOG_CAPACITY]{};
    size_t write_index_{0};
    size_t count_{0};
};

} // namespace gridshield::forensics
