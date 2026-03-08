/**
 * @file azure_iot.hpp
 * @brief Azure IoT Hub connector
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
// Azure IoT Constants
// ============================================================================
static constexpr uint16_t AZURE_IOT_DEFAULT_PORT = 8883;
static constexpr size_t AZURE_MAX_SAS_TOKEN_LENGTH = 256;
static constexpr uint32_t AZURE_SAS_DEFAULT_TTL_S = 3600;

// ============================================================================
// Types
// ============================================================================

struct SasToken
{
    std::array<char, AZURE_MAX_SAS_TOKEN_LENGTH> token{};
    uint64_t expiry_epoch_s{0};
    bool valid{false};
};

struct DeviceTwinState
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
// Azure IoT Hub Connector
// ============================================================================

class AzureIotConnector final : public ICloudConnector
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
        config_.provider = CloudProvider::AzureIotHub;
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
        twin_.reported.online = true;
        return core::Result<void>{};
    }

    core::Result<void> disconnect() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        twin_.reported.online = false;
        state_ = CloudState::Disconnected;
        return core::Result<void>{};
    }

    core::Result<void> publish_telemetry(const TelemetryPayload& payload) noexcept override
    {
        if (state_ != CloudState::Connected && state_ != CloudState::Ready) {
            return GS_MAKE_ERROR(core::ErrorCode::CloudPublishFailed);
        }
        twin_.reported.voltage_mv = payload.voltage_mv;
        twin_.reported.current_ma = payload.current_ma;
        twin_.reported.energy_wh = payload.energy_wh;
        twin_.reported.temperature_c10 = payload.temperature_c10;
        twin_.reported.tamper_detected = payload.tamper_detected;
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
        return CloudProvider::AzureIotHub;
    }

    const DeviceTwinState& twin() const noexcept
    {
        return twin_;
    }
    uint32_t publish_count() const noexcept
    {
        return publish_count_;
    }

    bool is_sas_expired(uint64_t current_epoch_s) const noexcept
    {
        return !sas_token_.valid || current_epoch_s >= sas_token_.expiry_epoch_s;
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
    void set_sas_token(const SasToken& token) noexcept
    {
        sas_token_ = token;
    }
    void set_desired_state(const DeviceTwinState::Desired& desired) noexcept
    {
        twin_.desired = desired;
        twin_.version++;
    }

private:
    CloudConfig config_{};
    CloudState state_{CloudState::Disconnected};
    DeviceTwinState twin_{};
    SasToken sas_token_{};
    CloudCommand pending_command_{};
    bool initialized_{false};
    bool simulate_connect_failure_{false};
    uint32_t publish_count_{0};
};

} // namespace gridshield::cloud
