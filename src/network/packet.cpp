/**
 * @file packet.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Secure packet protocol implementation
 * @version 0.2
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 */

#include "network/packet.hpp"

#if PLATFORM_NATIVE
    #include <cstring>
#else
    #include <string.h>
#endif

namespace gridshield::network {

SecurePacket::SecurePacket() noexcept 
    : is_valid_(false), next_sequence_(0) {
    memset(payload_, 0, MAX_PAYLOAD_SIZE);
}

core::Result<void> SecurePacket::build(PacketType type, 
                                       core::meter_id_t meter_id,
                                       core::Priority priority,
                                       const uint8_t* payload, 
                                       uint16_t payload_len,
                                       security::ICryptoEngine& crypto,
                                       const security::ECCKeyPair& keypair) noexcept {
    if (UNLIKELY(payload_len > MAX_PAYLOAD_SIZE)) {
        return MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    if (UNLIKELY(!keypair.has_private_key())) {
        return MAKE_ERROR(core::ErrorCode::AuthenticationFailed);
    }
    
    // Build header
    header_.type = type;
    header_.meter_id = meter_id;
    header_.priority = priority;
    header_.sequence = next_sequence_++;
    header_.payload_length = payload_len;
    header_.timestamp = 0; // Set by caller if needed
    
    // Copy payload
    if (payload != nullptr && payload_len > 0) {
        memcpy(payload_, payload, payload_len);
    }
    
    // Compute checksum
    auto crc_result = crypto.hash_sha256(
        payload_, payload_len, 
        reinterpret_cast<uint8_t*>(&header_.checksum)
    );
    if (crc_result.is_error()) {
        return crc_result.error();
    }
    
    // Sign packet
    auto sign_result = compute_signature(crypto, keypair);
    if (sign_result.is_error()) {
        return sign_result.error();
    }
    
    is_valid_ = true;
    return core::Result<void>();
}

core::Result<void> SecurePacket::parse(const uint8_t* buffer, 
                                       size_t buffer_len,
                                       security::ICryptoEngine& crypto,
                                       const security::ECCKeyPair& server_keypair) noexcept {
    const size_t min_size = sizeof(PacketHeader) + sizeof(PacketFooter);
    
    if (UNLIKELY(buffer == nullptr || buffer_len < min_size)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Parse header
    memcpy(&header_, buffer, sizeof(PacketHeader));
    
    // Verify magic numbers
    if (UNLIKELY(header_.magic_header != MAGIC_HEADER)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify payload length
    if (UNLIKELY(header_.payload_length > MAX_PAYLOAD_SIZE)) {
        return MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    const size_t expected_size = sizeof(PacketHeader) + 
                                 header_.payload_length + 
                                 sizeof(PacketFooter);
    if (UNLIKELY(buffer_len < expected_size)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Copy payload
    const uint8_t* payload_ptr = buffer + sizeof(PacketHeader);
    memcpy(payload_, payload_ptr, header_.payload_length);
    
    // Parse footer
    const uint8_t* footer_ptr = payload_ptr + header_.payload_length;
    memcpy(&footer_, footer_ptr, sizeof(PacketFooter));
    
    if (UNLIKELY(footer_.magic_footer != MAGIC_FOOTER)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify integrity
    auto verify_result = verify_integrity(crypto);
    if (verify_result.is_error()) {
        return verify_result.error();
    }
    
    // Verify signature
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
    memcpy(sign_data, &header_, sizeof(PacketHeader));
    memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
    
    auto sig_verify = crypto.verify(
        server_keypair, 
        sign_data, 
        sizeof(PacketHeader) + header_.payload_length,
        footer_.signature
    );
    
    if (sig_verify.is_error() || !sig_verify.value()) {
        return MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    
    is_valid_ = true;
    return core::Result<void>();
}

core::Result<size_t> SecurePacket::serialize(uint8_t* buffer, size_t buffer_size) const noexcept {
    if (UNLIKELY(!is_valid_)) {
        return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::InvalidState));
    }
    
    const size_t required_size = sizeof(PacketHeader) + 
                                 header_.payload_length + 
                                 sizeof(PacketFooter);
    
    if (UNLIKELY(buffer_size < required_size)) {
        return core::Result<size_t>(MAKE_ERROR(core::ErrorCode::BufferOverflow));
    }
    
    // Serialize header
    memcpy(buffer, &header_, sizeof(PacketHeader));
    
    // Serialize payload
    memcpy(buffer + sizeof(PacketHeader), payload_, header_.payload_length);
    
    // Serialize footer
    memcpy(buffer + sizeof(PacketHeader) + header_.payload_length, 
           &footer_, sizeof(PacketFooter));
    
    return core::Result<size_t>(required_size);
}

core::Result<void> SecurePacket::verify_integrity(security::ICryptoEngine& crypto) const noexcept {
    uint32_t computed_checksum;
    auto result = crypto.hash_sha256(
        payload_, 
        header_.payload_length, 
        reinterpret_cast<uint8_t*>(&computed_checksum)
    );
    
    if (result.is_error()) {
        return result.error();
    }
    
    if (UNLIKELY(computed_checksum != header_.checksum)) {
        return MAKE_ERROR(core::ErrorCode::IntegrityViolation);
    }
    
    return core::Result<void>();
}

core::Result<void> SecurePacket::compute_signature(security::ICryptoEngine& crypto,
                                                   const security::ECCKeyPair& keypair) noexcept {
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
    
    // Combine header and payload
    memcpy(sign_data, &header_, sizeof(PacketHeader));
    memcpy(sign_data + sizeof(PacketHeader), payload_, header_.payload_length);
    
    return crypto.sign(
        keypair, 
        sign_data, 
        sizeof(PacketHeader) + header_.payload_length,
        footer_.signature
    );
}

// ============================================================================
// TRANSPORT
// ============================================================================

PacketTransport::PacketTransport(platform::IPlatformComm& comm) noexcept
    : comm_(comm) {}

core::Result<void> PacketTransport::send_packet(const SecurePacket& packet,
                                                security::ICryptoEngine& /*crypto*/,
                                                const security::ECCKeyPair& /*keypair*/) noexcept {
    if (UNLIKELY(!packet.is_valid())) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
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
    
    if (UNLIKELY(send_result.value() != packet_size)) {
        return MAKE_ERROR(core::ErrorCode::TransmissionFailed);
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
    
    if (UNLIKELY(received_bytes < min_size)) {
        return core::Result<SecurePacket>(MAKE_ERROR(core::ErrorCode::InvalidPacket));
    }
    
    SecurePacket packet;
    auto parse_result = packet.parse(buffer, received_bytes, crypto, keypair);
    
    if (parse_result.is_error()) {
        return core::Result<SecurePacket>(parse_result.error());
    }
    
    return core::Result<SecurePacket>(ZMOVE(packet));
}

} // namespace gridshield::network