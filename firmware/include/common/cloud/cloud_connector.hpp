/**
 * @file cloud_connector.hpp
 * @brief Abstract cloud connector interface
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::cloud {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t CLOUD_MAX_ENDPOINT_LENGTH = 128;
static constexpr size_t CLOUD_MAX_DEVICE_ID_LENGTH = 64;
static constexpr size_t CLOUD_MAX_PAYLOAD_SIZE = 512;
static constexpr size_t CLOUD_MAX_TOPIC_LENGTH = 128;
static constexpr size_t CLOUD_MAX_SUBSCRIPTIONS = 4;

// ============================================================================
// Types
// ============================================================================

enum class CloudProvider : uint8_t
{
    Generic = 0,
    AwsIotCore = 1,
    AzureIotHub = 2
};

enum class CloudState : uint8_t
{
    Disconnected = 0,
    Connecting = 1,
    Connected = 2,
    Subscribing = 3,
    Ready = 4,
    Error = 5
};

struct CloudConfig
{
    CloudProvider provider{CloudProvider::Generic};
    std::array<char, CLOUD_MAX_ENDPOINT_LENGTH> endpoint{};
    std::array<char, CLOUD_MAX_DEVICE_ID_LENGTH> device_id{};
    uint16_t port{8883};
    uint16_t keepalive_s{60};
    bool require_tls{true};
};

struct TelemetryPayload
{
    uint32_t voltage_mv{0};
    uint32_t current_ma{0};
    uint32_t energy_wh{0};
    int16_t temperature_c10{0};
    bool tamper_detected{false};
    uint64_t timestamp{0};
    std::array<char, CLOUD_MAX_DEVICE_ID_LENGTH> device_id{};

    static TelemetryPayload from_reading(const core::MeterReading& reading,
                                         int16_t temp_c10,
                                         bool tamper,
                                         const char* dev_id) noexcept
    {
        TelemetryPayload payload{};
        payload.voltage_mv = reading.voltage_mv;
        payload.current_ma = reading.current_ma;
        payload.energy_wh = reading.energy_wh;
        payload.temperature_c10 = temp_c10;
        payload.tamper_detected = tamper;
        payload.timestamp = reading.timestamp;
        if (dev_id != nullptr) {
            std::strncpy(payload.device_id.data(), dev_id, CLOUD_MAX_DEVICE_ID_LENGTH - 1);
        }
        return payload;
    }
};

struct CloudCommand
{
    std::array<char, CLOUD_MAX_TOPIC_LENGTH> topic{};
    std::array<uint8_t, CLOUD_MAX_PAYLOAD_SIZE> payload{};
    uint16_t payload_len{0};
    bool valid{false};
};

// ============================================================================
// Interface
// ============================================================================

class ICloudConnector
{
public:
    virtual ~ICloudConnector() = default;

    virtual core::Result<void> init(const CloudConfig& config) noexcept = 0;
    virtual core::Result<void> connect() noexcept = 0;
    virtual core::Result<void> disconnect() noexcept = 0;
    virtual core::Result<void> publish_telemetry(const TelemetryPayload& payload) noexcept = 0;
    virtual core::Result<void> subscribe_commands() noexcept = 0;
    virtual bool has_pending_command() const noexcept = 0;
    virtual core::Result<CloudCommand> receive_command() noexcept = 0;
    virtual CloudState state() const noexcept = 0;
    virtual bool is_ready() const noexcept = 0;
    virtual CloudProvider provider() const noexcept = 0;
};

} // namespace gridshield::cloud
