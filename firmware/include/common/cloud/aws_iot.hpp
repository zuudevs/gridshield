/**
 * @file aws_iot.hpp
 * @brief AWS IoT Core connector
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "cloud/cloud_connector.hpp"
#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::cloud {

// ============================================================================
// AWS IoT Constants
// ============================================================================
static constexpr const char* AWS_TELEMETRY_PREFIX = "gridshield/telemetry/";
static constexpr const char* AWS_COMMAND_PREFIX = "gridshield/commands/";
static constexpr uint16_t AWS_IOT_DEFAULT_PORT = 8883;
static constexpr size_t AWS_SHADOW_MAX_SIZE = 256;

// ============================================================================
// Types
// ============================================================================

struct DeviceShadowState
{
    struct Reported
    {
        uint32_t voltage_mv{0};
        uint32_t current_ma{0};
        uint32_t energy_wh{0};
        int16_t temperature_c10{0};
        bool tamper_detected{false};
        bool online{false};
    } reported{};

    struct Desired
    {
        uint32_t reading_interval_ms{0};
        bool ota_requested{false};
        bool reset_requested{false};
    } desired{};

    uint32_t version{0};
};

// ============================================================================
// AWS IoT Core Connector
// ============================================================================

class AwsIotConnector final : public ICloudConnector
{
public:
    core::Result<void> init(const CloudConfig& config) noexcept override
    {
        if (config.endpoint[0] == '\0') {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (config.device_id[0] == '\0') {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        config_ = config;
        config_.provider = CloudProvider::AwsIotCore;
        state_ = CloudState::Disconnected;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> connect() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (simulate_connect_failure_) {
            state_ = CloudState::Error;
            return GS_MAKE_ERROR(core::ErrorCode::CloudConnectionFailed);
        }
        state_ = CloudState::Connected;
        shadow_.reported.online = true;
        return core::Result<void>{};
    }

    core::Result<void> disconnect() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        shadow_.reported.online = false;
        state_ = CloudState::Disconnected;
        return core::Result<void>{};
    }

    core::Result<void> publish_telemetry(const TelemetryPayload& payload) noexcept override
    {
        if (state_ != CloudState::Connected && state_ != CloudState::Ready) {
            return GS_MAKE_ERROR(core::ErrorCode::CloudPublishFailed);
        }
        shadow_.reported.voltage_mv = payload.voltage_mv;
        shadow_.reported.current_ma = payload.current_ma;
        shadow_.reported.energy_wh = payload.energy_wh;
        shadow_.reported.temperature_c10 = payload.temperature_c10;
        shadow_.reported.tamper_detected = payload.tamper_detected;
        publish_count_++;
        return core::Result<void>{};
    }

    core::Result<void> subscribe_commands() noexcept override
    {
        if (state_ != CloudState::Connected && state_ != CloudState::Ready) {
            return GS_MAKE_ERROR(core::ErrorCode::CloudSubscribeFailed);
        }
        state_ = CloudState::Ready;
        return core::Result<void>{};
    }

    bool has_pending_command() const noexcept override
    {
        return pending_command_.valid;
    }

    core::Result<CloudCommand> receive_command() noexcept override
    {
        if (!pending_command_.valid) {
            return core::Result<CloudCommand>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }
        auto cmd = pending_command_;
        pending_command_ = CloudCommand{};
        return core::Result<CloudCommand>(GS_MOVE(cmd));
    }

    CloudState state() const noexcept override
    {
        return state_;
    }
    bool is_ready() const noexcept override
    {
        return state_ == CloudState::Ready;
    }
    CloudProvider provider() const noexcept override
    {
        return CloudProvider::AwsIotCore;
    }

    const DeviceShadowState& shadow() const noexcept
    {
        return shadow_;
    }
    uint32_t publish_count() const noexcept
    {
        return publish_count_;
    }

    // === Test helpers ===
    void set_simulate_connect_failure(bool fail) noexcept
    {
        simulate_connect_failure_ = fail;
    }
    void inject_command(const CloudCommand& cmd) noexcept
    {
        pending_command_ = cmd;
    }
    void set_desired_state(const DeviceShadowState::Desired& desired) noexcept
    {
        shadow_.desired = desired;
        shadow_.version++;
    }

private:
    CloudConfig config_{};
    CloudState state_{CloudState::Disconnected};
    DeviceShadowState shadow_{};
    CloudCommand pending_command_{};
    bool initialized_{false};
    bool simulate_connect_failure_{false};
    uint32_t publish_count_{0};
};

} // namespace gridshield::cloud
