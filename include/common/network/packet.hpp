/**
 * @file packet.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Secure packet protocol with ECDSA authentication
 * @version 0.3
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "../platform/platform.hpp"
#include "security/crypto.hpp"

namespace gridshield {
namespace network {

// ============================================================================
// PROTOCOL CONSTANTS
// ============================================================================
constexpr uint16_t PROTOCOL_VERSION = 0x0100;
constexpr uint16_t MAX_PAYLOAD_SIZE = 512;
constexpr uint8_t MAGIC_HEADER = 0xA5;
constexpr uint8_t MAGIC_FOOTER = 0x5A;

// ============================================================================
// PACKET TYPE
// ============================================================================
enum class PacketType : uint8_t {
    Invalid = 0,
    MeterData = 1,
    TamperAlert = 2,
    Heartbeat = 3,
    Command = 4,
    Acknowledgment = 5,
    KeyExchange = 6
};

// ============================================================================
// PACKET STRUCTURES
// ============================================================================
#pragma pack(push, 1)
struct PacketHeader {
    uint8_t magic_header;
    uint16_t version;
    PacketType type;
    core::Priority priority;
    core::meter_id_t meter_id;
    core::sequence_t sequence;
    uint16_t payload_length;
    core::timestamp_t timestamp;
    uint32_t checksum;
    
    PacketHeader() noexcept
        : magic_header(MAGIC_HEADER), version(PROTOCOL_VERSION),
          type(PacketType::Invalid), priority(core::Priority::Normal),
          meter_id(0), sequence(0), payload_length(0), 
          timestamp(0), checksum(0) {}
};

struct PacketFooter {
    uint8_t signature[security::ECC_SIGNATURE_SIZE];
    uint8_t magic_footer;
    
    PacketFooter() noexcept : magic_footer(MAGIC_FOOTER) {
#if GS_PLATFORM_NATIVE
        std::memset(signature, 0, security::ECC_SIGNATURE_SIZE);
#else
        memset(signature, 0, security::ECC_SIGNATURE_SIZE);
#endif
    }
};
#pragma pack(pop)

// ============================================================================
// SECURE PACKET
// ============================================================================
class SecurePacket {
public:
    SecurePacket() noexcept;
    
    core::Result<void> build(
        PacketType type, 
        core::meter_id_t meter_id,
        core::Priority priority,
        const uint8_t* payload, 
        uint16_t payload_len,
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair) noexcept;
    
    core::Result<void> parse(
        const uint8_t* buffer, 
        size_t buffer_len,
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& server_keypair) noexcept;
    
    core::Result<size_t> serialize(uint8_t* buffer, size_t buffer_size) const noexcept;
    
    GS_NODISCARD const PacketHeader& header() const noexcept { return header_; }
    GS_NODISCARD const uint8_t* payload() const noexcept { return payload_; }
    GS_NODISCARD uint16_t payload_length() const noexcept { return header_.payload_length; }
    
    GS_NODISCARD bool is_valid() const noexcept { return is_valid_; }
    
private:
    core::Result<void> verify_integrity(security::ICryptoEngine& crypto) const noexcept;
    core::Result<void> compute_signature(
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair) noexcept;
    
    PacketHeader header_;
    uint8_t payload_[MAX_PAYLOAD_SIZE];
    PacketFooter footer_;
    bool is_valid_;
    core::sequence_t next_sequence_;
};

// ============================================================================
// PACKET TRANSPORT INTERFACE
// ============================================================================
class IPacketTransport {
public:
    virtual ~IPacketTransport() noexcept = default;
    
    virtual core::Result<void> send_packet(
        const SecurePacket& packet,
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair) noexcept = 0;
    
    virtual core::Result<SecurePacket> receive_packet(
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair,
        uint32_t timeout_ms) noexcept = 0;
};

// ============================================================================
// PACKET TRANSPORT IMPLEMENTATION
// ============================================================================
class PacketTransport final : public IPacketTransport {
public:
    explicit PacketTransport(platform::IPlatformComm& comm) noexcept;
    ~PacketTransport() noexcept override = default;
    
    core::Result<void> send_packet(
        const SecurePacket& packet,
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair) noexcept override;
    
    core::Result<SecurePacket> receive_packet(
        security::ICryptoEngine& crypto,
        const security::ECCKeyPair& keypair,
        uint32_t timeout_ms) noexcept override;
    
private:
    platform::IPlatformComm& comm_;
};

} // namespace network
} // namespace gridshield