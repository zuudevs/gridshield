/**
 * @file mqtt.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief MQTT client abstraction for telemetry publishing
 * @version 1.0
 * @date 2026-02-25
 *
 * Provides a platform-agnostic MQTT client interface with
 * publish/subscribe, QoS levels, and TLS support.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>

namespace gridshield::network {

// ============================================================================
// MQTT CONSTANTS
// ============================================================================
static constexpr size_t MQTT_MAX_TOPIC_LENGTH = 128;
static constexpr size_t MQTT_MAX_PAYLOAD_SIZE = 512;
static constexpr size_t MQTT_MAX_CLIENT_ID_LENGTH = 32;
static constexpr size_t MQTT_MAX_URI_LENGTH = 128;
static constexpr size_t MQTT_MAX_USERNAME_LENGTH = 64;
static constexpr size_t MQTT_MAX_PASSWORD_LENGTH = 64;
static constexpr uint16_t MQTT_DEFAULT_PORT = 8883;
static constexpr uint16_t MQTT_DEFAULT_PORT_NO_TLS = 1883;
static constexpr uint16_t MQTT_DEFAULT_KEEPALIVE_S = 60;

// ============================================================================
// MQTT QOS LEVELS
// ============================================================================
enum class MqttQos : uint8_t
{
    AtMostOnce = 0,  // Fire-and-forget
    AtLeastOnce = 1, // Acknowledged delivery
    ExactlyOnce = 2  // Guaranteed single delivery
};

// ============================================================================
// MQTT CONFIGURATION
// ============================================================================
struct MqttConfig
{
    const char* broker_uri{};
    const char* client_id{};
    const char* username{};
    const char* password{};
    uint16_t port{MQTT_DEFAULT_PORT};
    uint16_t keepalive_s{MQTT_DEFAULT_KEEPALIVE_S};
    bool use_tls{true};

    GS_CONSTEXPR MqttConfig() noexcept = default;
};

// ============================================================================
// MQTT CLIENT INTERFACE
// ============================================================================
class IMqttClient
{
public:
    virtual ~IMqttClient() noexcept = default;

    /**
     * @brief Connect to the MQTT broker.
     */
    virtual core::Result<void> connect(const MqttConfig& config) noexcept = 0;

    /**
     * @brief Disconnect from the broker.
     */
    virtual core::Result<void> disconnect() noexcept = 0;

    /**
     * @brief Publish a message to a topic.
     */
    virtual core::Result<void>
    publish(const char* topic, const uint8_t* data, size_t length, MqttQos qos) noexcept = 0;

    /**
     * @brief Subscribe to a topic.
     */
    virtual core::Result<void> subscribe(const char* topic, MqttQos qos) noexcept = 0;

    /**
     * @brief Unsubscribe from a topic.
     */
    virtual core::Result<void> unsubscribe(const char* topic) noexcept = 0;

    /**
     * @brief Check connection status.
     */
    GS_NODISCARD virtual bool is_connected() noexcept = 0;

    /**
     * @brief Process pending MQTT events (call periodically in main loop).
     */
    virtual core::Result<void> poll() noexcept = 0;
};

// ============================================================================
// MQTT TOPIC BUILDER
// ============================================================================

/**
 * @brief Builds standard GridShield MQTT topics.
 *
 * Topic format: gridshield/{meter_id}/{subtopic}
 * Examples:
 *   gridshield/1234567890ABCDEF/telemetry
 *   gridshield/1234567890ABCDEF/alert
 *   gridshield/1234567890ABCDEF/status
 */
struct MqttTopics
{
    static constexpr const char* PREFIX = "gridshield";
    static constexpr const char* TELEMETRY = "telemetry";
    static constexpr const char* ALERT = "alert";
    static constexpr const char* STATUS = "status";
    static constexpr const char* COMMAND = "command";
    static constexpr const char* OTA = "ota";
};

} // namespace gridshield::network
