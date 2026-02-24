/**
 * @file degradation.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Graceful degradation framework for subsystem failure handling
 * @version 1.0
 * @date 2026-02-23
 *
 * When a non-critical subsystem fails, the system can continue
 * operating in a reduced capacity rather than halting entirely.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include <algorithm>
#include <array>

namespace gridshield::core {

// ============================================================================
// SERVICE IDENTIFIERS
// ============================================================================
enum class ServiceId : uint8_t
{
    Crypto = 0,
    Tamper = 1,
    Network = 2,
    Analytics = 3,
    Storage = 4,
    Watchdog = 5,
    Count = 6
};

// ============================================================================
// SERVICE HEALTH
// ============================================================================
enum class ServiceHealth : uint8_t
{
    Healthy = 0,  // Fully operational
    Degraded = 1, // Partial functionality
    Failed = 2,   // Non-functional
    Disabled = 3  // Intentionally off
};

// ============================================================================
// DEGRADATION POLICY
// ============================================================================
struct DegradationPolicy
{
    bool allow_without_crypto{false};   // CRITICAL — cannot operate
    bool allow_without_tamper{true};    // Can operate, reduced security
    bool allow_without_network{true};   // Can buffer data locally
    bool allow_without_analytics{true}; // Can operate without anomaly detection
    bool allow_without_storage{true};   // Can operate in-memory only
    static constexpr uint8_t DEFAULT_MAX_NETWORK_FAILURES = 5;
    uint8_t max_network_failures{DEFAULT_MAX_NETWORK_FAILURES}; // Disable network after N failures

    GS_CONSTEXPR DegradationPolicy() noexcept = default;
};

// ============================================================================
// DEGRADATION MANAGER
// ============================================================================
class DegradationManager
{
public:
    DegradationManager() noexcept = default;

    /**
     * @brief Initialize with a degradation policy
     */
    void set_policy(const DegradationPolicy& policy) noexcept
    {
        policy_ = policy;
        for (auto& hlt : health_) {
            hlt = ServiceHealth::Healthy;
        }
        for (auto& fct : failure_counts_) {
            fct = 0;
        }
    }

    /**
     * @brief Report a subsystem failure
     * @return true if the system can continue operating
     */
    bool report_failure(ServiceId service, ErrorCode error) noexcept // NOLINT(readability-make-member-function-const)
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) {
            return false;
        }

        ++failure_counts_[idx];
        health_[idx] = ServiceHealth::Failed;
        last_errors_[idx] = error;

        return can_continue();
    }

    /**
     * @brief Report a subsystem recovery
     */
    void report_recovery(ServiceId service) noexcept
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) { // NOLINT(readability-convert-member-functions-to-static)
            return;
        }

        health_[idx] = ServiceHealth::Healthy;
        failure_counts_[idx] = 0;
        last_errors_[idx] = ErrorCode::Success;
    }

    /**
     * @brief Mark a service as degraded (partial functionality)
     */
    void report_degraded(ServiceId service) noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) {
            return;
        }
        health_[idx] = ServiceHealth::Degraded;
    }

    /**
     * @brief Check if the system can continue operating
     */
    GS_NODISCARD bool can_continue() const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        // Crypto is always required
        if (!policy_.allow_without_crypto &&
            health_[static_cast<uint8_t>(ServiceId::Crypto)] == ServiceHealth::Failed) {
            return false;
        }

        if (!policy_.allow_without_tamper &&
            health_[static_cast<uint8_t>(ServiceId::Tamper)] == ServiceHealth::Failed) {
            return false;
        }

        if (!policy_.allow_without_network &&
            health_[static_cast<uint8_t>(ServiceId::Network)] == ServiceHealth::Failed) {
            return false;
        }

        if (!policy_.allow_without_analytics &&
            health_[static_cast<uint8_t>(ServiceId::Analytics)] == ServiceHealth::Failed) {
            return false;
        }

        return true;
    }

    /**
     * @brief Check if a specific service is available
     */
    GS_NODISCARD bool is_service_available(ServiceId service) const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) {
            return false;
        }
        return health_[idx] == ServiceHealth::Healthy || health_[idx] == ServiceHealth::Degraded;
    }

    /**
     * @brief Get health of a specific service
     */
    GS_NODISCARD ServiceHealth get_health(ServiceId service) const noexcept // NOLINT(readability-convert-member-functions-to-static)
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) {
            return ServiceHealth::Failed;
        }
        return health_[idx];
    }

    /**
     * @brief Get failure count for a service
     */
    GS_NODISCARD uint16_t get_failure_count(ServiceId service) const noexcept
    {
        auto idx = static_cast<uint8_t>(service);
        if (idx >= static_cast<uint8_t>(ServiceId::Count)) {
            return 0;
        }
        return failure_counts_[idx];
    }

    /**
     * @brief Check if any service is degraded or failed
     */
    GS_NODISCARD bool is_degraded() const noexcept
    {
        return std::any_of(health_.begin(), health_.end(), [](ServiceHealth hlt) {
            return hlt != ServiceHealth::Healthy;
        });
    }

private:
    static constexpr uint8_t MAX_SERVICES = static_cast<uint8_t>(ServiceId::Count);

    DegradationPolicy policy_;
    std::array<ServiceHealth, MAX_SERVICES> health_{};
    std::array<uint16_t, MAX_SERVICES> failure_counts_{};
    std::array<ErrorCode, MAX_SERVICES> last_errors_{};
};

} // namespace gridshield::core
