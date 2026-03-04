/**
 * @file esp_now_mesh.hpp
 * @brief ESP-NOW mesh networking interface
 *
 * Peer-to-peer mesh using ESP-NOW protocol with
 * unicast, broadcast, and stats tracking.
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
static constexpr size_t MESH_MAC_LENGTH = 6;
static constexpr size_t MESH_MAX_PAYLOAD = 250;
static constexpr size_t MESH_MAX_PEERS = 8;
static constexpr uint8_t MESH_DEFAULT_TTL = 3;
static constexpr size_t MESH_KEY_SIZE = 16;
static constexpr uint8_t MESH_MIN_CHANNEL = 1;
static constexpr uint8_t MESH_MAX_CHANNEL = 14;

// ============================================================================
// Types
// ============================================================================

struct MeshConfig
{
    uint8_t channel{1};
    uint8_t ttl{MESH_DEFAULT_TTL};
    std::array<uint8_t, MESH_KEY_SIZE> pmk{};
    bool encryption_enabled{false};
};

struct MeshPeer
{
    std::array<uint8_t, MESH_MAC_LENGTH> mac{};
    bool encrypted{false};
    bool active{false};
};

struct MeshPacketHeader
{
    std::array<uint8_t, MESH_MAC_LENGTH> src_mac{};
    std::array<uint8_t, MESH_MAC_LENGTH> dst_mac{};
    uint16_t payload_len{0};
    uint8_t ttl{0};
    uint8_t hop_count{0};
};

struct MeshPacket
{
    MeshPacketHeader header{};
    std::array<uint8_t, MESH_MAX_PAYLOAD> payload{};
};

struct MeshStats
{
    uint32_t tx_count{0};
    uint32_t rx_count{0};
    uint32_t tx_fail_count{0};
    uint32_t broadcast_count{0};
};

// ============================================================================
// Interface
// ============================================================================

class IEspNowMesh
{
public:
    virtual ~IEspNowMesh() = default;

    virtual core::Result<void> init(const MeshConfig& config) noexcept = 0;
    virtual core::Result<void> add_peer(const std::array<uint8_t, MESH_MAC_LENGTH>& mac,
                                        bool encrypted) noexcept = 0;
    virtual core::Result<void>
    remove_peer(const std::array<uint8_t, MESH_MAC_LENGTH>& mac) noexcept = 0;
    virtual core::Result<void> send(const std::array<uint8_t, MESH_MAC_LENGTH>& dst_mac,
                                    const uint8_t* data,
                                    size_t len) noexcept = 0;
    virtual core::Result<void> broadcast(const uint8_t* data, size_t len) noexcept = 0;
    virtual bool has_pending_packet() const noexcept = 0;
    virtual core::Result<MeshPacket> receive() noexcept = 0;
    virtual MeshStats get_stats() const noexcept = 0;
    virtual uint8_t peer_count() const noexcept = 0;
};

// ============================================================================
// Stub Implementation
// ============================================================================

class EspNowMesh final : public IEspNowMesh
{
public:
    core::Result<void> init(const MeshConfig& config) noexcept override
    {
        if (config.channel < MESH_MIN_CHANNEL || config.channel > MESH_MAX_CHANNEL) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        config_ = config;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> add_peer(const std::array<uint8_t, MESH_MAC_LENGTH>& mac,
                                bool encrypted) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        for (auto& peer : peers_) {
            if (!peer.active) {
                peer.mac = mac;
                peer.encrypted = encrypted;
                peer.active = true;
                return core::Result<void>{};
            }
        }
        return GS_MAKE_ERROR(core::ErrorCode::MeshPeerFull);
    }

    core::Result<void>
    remove_peer(const std::array<uint8_t, MESH_MAC_LENGTH>& mac) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        for (auto& peer : peers_) {
            if (peer.active && peer.mac == mac) {
                peer = MeshPeer{};
                return core::Result<void>{};
            }
        }
        return GS_MAKE_ERROR(core::ErrorCode::MeshInvalidPeer);
    }

    core::Result<void> send(const std::array<uint8_t, MESH_MAC_LENGTH>& /*dst_mac*/,
                            const uint8_t* data,
                            size_t len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (data == nullptr || len == 0 || len > MESH_MAX_PAYLOAD) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (simulate_send_failure_) {
            stats_.tx_fail_count++;
            return GS_MAKE_ERROR(core::ErrorCode::MeshSendFailed);
        }
        stats_.tx_count++;
        return core::Result<void>{};
    }

    core::Result<void> broadcast(const uint8_t* data, size_t len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (data == nullptr || len == 0 || len > MESH_MAX_PAYLOAD) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        stats_.broadcast_count++;
        return core::Result<void>{};
    }

    bool has_pending_packet() const noexcept override
    {
        return pending_packet_available_;
    }

    core::Result<MeshPacket> receive() noexcept override
    {
        if (!pending_packet_available_) {
            return core::Result<MeshPacket>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }
        auto pkt = injected_packet_;
        pending_packet_available_ = false;
        stats_.rx_count++;
        return core::Result<MeshPacket>(GS_MOVE(pkt));
    }

    MeshStats get_stats() const noexcept override
    {
        return stats_;
    }

    uint8_t peer_count() const noexcept override
    {
        uint8_t cnt = 0;
        for (const auto& peer : peers_) {
            if (peer.active)
                cnt++;
        }
        return cnt;
    }

    // === Test helpers ===
    void inject_packet(const MeshPacket& packet) noexcept
    {
        injected_packet_ = packet;
        pending_packet_available_ = true;
    }

    void set_simulate_send_failure(bool fail) noexcept
    {
        simulate_send_failure_ = fail;
    }

private:
    MeshConfig config_{};
    std::array<MeshPeer, MESH_MAX_PEERS> peers_{};
    MeshStats stats_{};
    MeshPacket injected_packet_{};
    bool initialized_{false};
    bool pending_packet_available_{false};
    bool simulate_send_failure_{false};
};

} // namespace gridshield::network
