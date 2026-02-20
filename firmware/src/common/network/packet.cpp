/**
 * @file packet.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Packet protocol implementation with cryptographic authentication
 * @version 0.5
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#include "network/packet.hpp"

#if GS_PLATFORM_NATIVE
    #include <cstring>
#else
    #include <string.h>
#endif

namespace gridshield {
namespace network {

SecurePacket::SecurePacket() noexcept 
    : is_valid_(false), next_sequence_(0) {
#if GS_PLATFORM_NATIVE
    std::memset(payload_, 0, MAX_PAYLOAD_SIZE);
#else
    memset(payload_, 0, MAX_PAYLOAD_SIZE);
#endif
}

core::Result<void> SecurePacket::build(
    PacketType type, 
    core::meter_id_t meter_id,
    core::Priority priority,
    const uint8_t* payload, 
    uint16_t payload_len,
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair) noexcept {
    
    if (GS_UNLIKELY(payload_len > MAX_PAYLOAD_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    if (GS_UNLIKELY(!keypair.has_private_key())) {
        return GS_MAKE_ERROR(core::ErrorCode::AuthenticationFailed);
    }
    
    // Build header
    header_.type = type;
    header_.meter_id = meter_id;
    header_.priority = priority;
    header_.sequence = next_sequence_++;
    header_.payload_length = payload_len;
    header_.timestamp = 0; // Set by platform time if needed
    
    // Copy payload
    if (payload != nullptr && payload_len > 0) {
#if GS_PLATFORM_NATIVE
        std::memcpy(payload_, payload, payload_len);
#else
        memcpy(payload_, payload, payload_len);
#endif
    }
    
    // Compute checksum (first 4 bytes of SHA256)
    uint8_t hash[security::SHA256_HASH_SIZE];
    GS_TRY(crypto.hash_sha256(payload_, payload_len, hash));
#if GS_PLATFORM_NATIVE
    std::memcpy(&header_.checksum, hash, sizeof(uint32_t));
#else
    memcpy(&header_.checksum, hash, sizeof(uint32_t));
#endif
    
    // Sign packet
    GS_TRY(compute_signature(crypto, keypair));
    
    is_valid_ = true;
    return core::Result<void>();
}

core::Result<void> SecurePacket::parse(
    const uint8_t* buffer, 
    size_t buffer_len,
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& server_keypair) noexcept {
    
    const size_t min_size = sizeof(PacketHeader) + sizeof(PacketFooter);
    
    if (GS_UNLIKELY(buffer == nullptr || buffer_len < min_size)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Parse header
#if GS_PLATFORM_NATIVE
    std::memcpy(&header_, buffer, sizeof(PacketHeader));
#else
    memcpy(&header_, buffer, sizeof(PacketHeader));
#endif
    
    // Verify magic numbers
    if (GS_UNLIKELY(header_.magic_header != MAGIC_HEADER)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify payload length
    if (GS_UNLIKELY(header_.payload_length > MAX_PAYLOAD_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    const size_t expected_size = sizeof(PacketHeader) + 
                                 header_.payload_length + 
                                 sizeof(PacketFooter);
    if (GS_UNLIKELY(buffer_len < expected_size)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Copy payload
    const uint8_t* payload_ptr = buffer + sizeof(PacketHeader);
#if GS_PLATFORM_NATIVE
    std::memcpy(payload_, payload_ptr, header_.payload_length);
#else
    memcpy(payload_, payload_ptr, header_.payload_length);
#endif
    
    // Parse footer
    const uint8_t* footer_ptr = payload_ptr + header_.payload_length;
#if GS_PLATFORM_NATIVE
    std::memcpy(&footer_, footer_ptr, sizeof(PacketFooter));
#else
    memcpy(&footer_, footer_ptr, sizeof(PacketFooter));
#endif
    
    if (GS_UNLIKELY(footer_.magic_footer != MAGIC_FOOTER)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify integrity
    GS_TRY(verify_integrity(crypto));
    
    // Verify signature
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
#if GS_PLATFORM_NATIVE
    std::memcpy(sign_data, &header_, sizeof(PacketHeader));
    std::memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
#else
    memcpy(sign_data, &header_, sizeof(PacketHeader));
    memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
#endif
    
    auto sig_verify = crypto.verify(
        server_keypair, 
        sign_data, 
        sizeof(PacketHeader) + header_.payload_length,
        footer_.signature
    );
    
    if (sig_verify.is_error() || !sig_verify.value()) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    
    is_valid_ = true;
    return core::Result<void>();
}

core::Result<size_t> SecurePacket::serialize(uint8_t* buffer, size_t buffer_size) const noexcept {
    if (GS_UNLIKELY(!is_valid_)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidState));
    }
    
    const size_t required_size = sizeof(PacketHeader) + 
                                 header_.payload_length + 
                                 sizeof(PacketFooter);
    
    if (GS_UNLIKELY(buffer_size < required_size)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
    }
    
    // Serialize header
#if GS_PLATFORM_NATIVE
    std::memcpy(buffer, &header_, sizeof(PacketHeader));
    std::memcpy(buffer + sizeof(PacketHeader), payload_, header_.payload_length);
    std::memcpy(buffer + sizeof(PacketHeader) + header_.payload_length, 
           &footer_, sizeof(PacketFooter));
#else
    memcpy(buffer, &header_, sizeof(PacketHeader));
    memcpy(buffer + sizeof(PacketHeader), payload_, header_.payload_length);
    memcpy(buffer + sizeof(PacketHeader) + header_.payload_length, 
           &footer_, sizeof(PacketFooter));
#endif
    
    return core::Result<size_t>(required_size);
}

core::Result<void> SecurePacket::verify_integrity(security::ICryptoEngine& crypto) const noexcept {
    uint8_t hash[security::SHA256_HASH_SIZE];
    GS_TRY(crypto.hash_sha256(payload_, header_.payload_length, hash));
    
    uint32_t computed_checksum;
#if GS_PLATFORM_NATIVE
    std::memcpy(&computed_checksum, hash, sizeof(uint32_t));
#else
    memcpy(&computed_checksum, hash, sizeof(uint32_t));
#endif
    
    if (GS_UNLIKELY(computed_checksum != header_.checksum)) {
        return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation);
    }
    
    return core::Result<void>();
}

core::Result<void> SecurePacket::compute_signature(
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair) noexcept {
    
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
    
    // Combine header and payload
#if GS_PLATFORM_NATIVE
    std::memcpy(sign_data, &header_, sizeof(PacketHeader));
    std::memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
#else
    memcpy(sign_data, &header_, sizeof(PacketHeader));
    memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
#endif
    
    return crypto.sign(
        keypair, 
        sign_data, 
        sizeof(PacketHeader) + header_.payload_length,
        footer_.signature
    );
}

// ============================================================================
// TRANSPORT IMPLEMENTATION
// ============================================================================

PacketTransport::PacketTransport(platform::IPlatformComm& comm) noexcept
    : comm_(comm) {}

core::Result<void> PacketTransport::send_packet(
    const SecurePacket& packet,
    security::ICryptoEngine& /*crypto*/,
    const security::ECCKeyPair& /*keypair*/) noexcept {
    
    if (GS_UNLIKELY(!packet.is_valid())) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(PacketFooter)];
    
    auto serialize_result = packet.serialize(buffer, sizeof(buffer));
    if (serialize_result.is_error()) {
        return serialize_result.error();
    }
    
    const size_t packet_size = serialize_result.value();
    auto send_result = comm_.send(buffer, packet_size);
    
    if (send_result.is_error()) {
        return send_result.error();
    }
    
    if (GS_UNLIKELY(send_result.value() != packet_size)) {
        return GS_MAKE_ERROR(core::ErrorCode::TransmissionFailed);
    }
    
    return core::Result<void>();
}

core::Result<SecurePacket> PacketTransport::receive_packet(
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair,
    uint32_t timeout_ms) noexcept {
    
    uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(PacketFooter)];
    
    auto recv_result = comm_.receive(buffer, sizeof(buffer), timeout_ms);
    if (recv_result.is_error()) {
        return core::Result<SecurePacket>(recv_result.error());
    }
    
    const size_t received_bytes = recv_result.value();
    const size_t min_size = sizeof(PacketHeader) + sizeof(PacketFooter);
    
    if (GS_UNLIKELY(received_bytes < min_size)) {
        return core::Result<SecurePacket>(GS_MAKE_ERROR(core::ErrorCode::InvalidPacket));
    }
    
    SecurePacket packet;
    auto parse_result = packet.parse(buffer, received_bytes, crypto, keypair);
    
    if (parse_result.is_error()) {
        return core::Result<SecurePacket>(parse_result.error());
    }
    
    return core::Result<SecurePacket>(GS_MOVE(packet));
}

} // namespace network
} // namespace gridshield