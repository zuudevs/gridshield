/**
 * @file sntp_client.hpp
 * @brief SNTP client interface for time synchronization
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::network {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t SNTP_MAX_SERVERS = 3;
static constexpr size_t SNTP_MAX_HOST_LENGTH = 64;
static constexpr uint32_t SNTP_DEFAULT_POLL_INTERVAL_S = 3600;
static constexpr uint16_t SNTP_DEFAULT_TIMEOUT_MS = 5000;

// ============================================================================
// Types
// ============================================================================

enum class SntpState : uint8_t
{
    NotStarted = 0,
    Syncing = 1,
    Synced = 2,
    Error = 3
};

struct SntpConfig
{
    std::array<std::array<char, SNTP_MAX_HOST_LENGTH>, SNTP_MAX_SERVERS> servers{};
    uint8_t server_count{0};
    uint32_t poll_interval_s{SNTP_DEFAULT_POLL_INTERVAL_S};
    uint16_t timeout_ms{SNTP_DEFAULT_TIMEOUT_MS};
};

struct TimeInfo
{
    uint64_t epoch_s{0};
    uint16_t accuracy_ms{0};
    bool valid{false};
};

// ============================================================================
// Interface
// ============================================================================

class ISntpClient
{
public:
    virtual ~ISntpClient() = default;

    virtual core::Result<void> init(const SntpConfig& config) noexcept = 0;
    virtual core::Result<void> sync() noexcept = 0;
    virtual core::Result<TimeInfo> get_time() const noexcept = 0;
    virtual core::Result<uint64_t> get_epoch_s() const noexcept = 0;
    virtual bool is_synced() const noexcept = 0;
    virtual SntpState state() const noexcept = 0;
};

// ============================================================================
// Stub Implementation
// ============================================================================

class SntpClient final : public ISntpClient
{
public:
    core::Result<void> init(const SntpConfig& config) noexcept override
    {
        if (config.server_count == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        config_ = config;
        state_ = SntpState::NotStarted;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> sync() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (simulate_failure_) {
            state_ = SntpState::Error;
            return GS_MAKE_ERROR(core::ErrorCode::SntpSyncFailed);
        }
        state_ = SntpState::Synced;
        time_info_.epoch_s = simulated_epoch_s_;
        time_info_.accuracy_ms = simulated_accuracy_ms_;
        time_info_.valid = true;
        return core::Result<void>{};
    }

    core::Result<TimeInfo> get_time() const noexcept override
    {
        if (state_ != SntpState::Synced) {
            return core::Result<TimeInfo>(GS_MAKE_ERROR(core::ErrorCode::SntpNotSynced));
        }
        return core::Result<TimeInfo>(time_info_);
    }

    core::Result<uint64_t> get_epoch_s() const noexcept override
    {
        if (state_ != SntpState::Synced) {
            return core::Result<uint64_t>(GS_MAKE_ERROR(core::ErrorCode::SntpNotSynced));
        }
        return core::Result<uint64_t>(time_info_.epoch_s);
    }

    bool is_synced() const noexcept override
    {
        return state_ == SntpState::Synced;
    }

    SntpState state() const noexcept override
    {
        return state_;
    }

    // === Test helpers ===
    void set_simulated_epoch(uint64_t epoch_s) noexcept
    {
        simulated_epoch_s_ = epoch_s;
    }
    void set_simulated_accuracy(uint16_t accuracy_ms) noexcept
    {
        simulated_accuracy_ms_ = accuracy_ms;
    }
    void set_simulate_failure(bool fail) noexcept
    {
        simulate_failure_ = fail;
    }

private:
    SntpConfig config_{};
    SntpState state_{SntpState::NotStarted};
    TimeInfo time_info_{};
    bool initialized_{false};
    bool simulate_failure_{false};
    uint64_t simulated_epoch_s_{1700000000};
    uint16_t simulated_accuracy_ms_{10};
};

} // namespace gridshield::network
