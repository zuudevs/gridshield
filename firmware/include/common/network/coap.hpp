/**
 * @file coap.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief CoAP (Constrained Application Protocol) client interface
 * @version 1.0
 * @date 2026-02-25
 *
 * Lightweight CoAP client for constrained IoT devices.
 * Uses UDP transport with optional DTLS security.
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
// COAP CONSTANTS
// ============================================================================
static constexpr uint16_t COAP_DEFAULT_PORT = 5683;
static constexpr uint16_t COAP_DTLS_PORT = 5684;
static constexpr size_t COAP_MAX_URI_LENGTH = 128;
static constexpr size_t COAP_MAX_PAYLOAD_SIZE = 256;
static constexpr uint32_t COAP_DEFAULT_TIMEOUT_MS = 5000;

// ============================================================================
// COAP METHOD
// ============================================================================
enum class CoapMethod : uint8_t
{
    Get = 1,
    Post = 2,
    Put = 3,
    Delete = 4
};

// ============================================================================
// COAP RESPONSE CODE
// ============================================================================
enum class CoapResponseCode : uint8_t
{
    Created = 65,       // 2.01
    Deleted = 66,       // 2.02
    Valid = 67,         // 2.03
    Changed = 68,       // 2.04
    Content = 69,       // 2.05
    BadRequest = 128,   // 4.00
    Unauthorized = 129, // 4.01
    NotFound = 132,     // 4.04
    ServerError = 160   // 5.00
};

// ============================================================================
// COAP CONFIGURATION
// ============================================================================
struct CoapConfig
{
    const char* server_uri{};
    uint16_t port{COAP_DEFAULT_PORT};
    uint32_t timeout_ms{COAP_DEFAULT_TIMEOUT_MS};
    bool use_dtls{false};

    GS_CONSTEXPR CoapConfig() noexcept = default;
};

// ============================================================================
// COAP CLIENT INTERFACE
// ============================================================================
class ICoapClient
{
public:
    virtual ~ICoapClient() noexcept = default;

    /**
     * @brief Initialize the CoAP client.
     */
    virtual core::Result<void> init(const CoapConfig& config) noexcept = 0;

    /**
     * @brief Send a CoAP request.
     * @param method HTTP-like method (GET, POST, PUT, DELETE)
     * @param uri_path Resource path (e.g. "/sensor/temperature")
     * @param payload Request body
     * @param payload_len Body length
     */
    virtual core::Result<CoapResponseCode> request(CoapMethod method,
                                                   const char* uri_path,
                                                   const uint8_t* payload,
                                                   size_t payload_len) noexcept = 0;

    /**
     * @brief Send a GET request with no payload.
     */
    virtual core::Result<CoapResponseCode> get(const char* uri_path) noexcept = 0;

    /**
     * @brief Send a POST request with payload.
     */
    virtual core::Result<CoapResponseCode>
    post(const char* uri_path, const uint8_t* payload, size_t payload_len) noexcept = 0;

    /**
     * @brief Shut down the CoAP client.
     */
    virtual void shutdown() noexcept = 0;
};

} // namespace gridshield::network
