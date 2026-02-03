/**
 * @file packet.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "network/packet.hpp"
#include <cstring>

namespace gridshield::network {

SecurePacket::SecurePacket() noexcept 
    : is_valid_(false), next_sequence_(0) {
    for (size_t i = 0; i < MAX_PAYLOAD_SIZE; ++i) {
        payload_[i] = 0;
    }
}

core::Result<void> SecurePacket::build(PacketType type, 
                                       core::meter_id_t meter_id,
                                       core::Priority priority,
                                       const uint8_t* payload, 
                                       uint16_t payload_len,
                                       security::ICryptoEngine& crypto,
                                       const security::ECCKeyPair& keypair) noexcept {
    if (payload_len > MAX_PAYLOAD_SIZE) {
        return MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    if (!keypair.has_private_key()) {
        return MAKE_ERROR(core::ErrorCode::AuthenticationFailed);
    }
    
    // Build header
    header_.type = type;
    header_.meter_id = meter_id;
    header_.priority = priority;
    header_.sequence = next_sequence_++;
    header_.payload_length = payload_len;
    header_.timestamp = 0; // Set by platform layer
    
    // Copy payload
    if (payload != nullptr && payload_len > 0) {
        for (uint16_t i = 0; i < payload_len; ++i) {
            payload_[i] = payload[i];
        }
    }
    
    // Compute checksum
    auto crc_result = crypto.hash_sha256(payload_, payload_len, 
                                         reinterpret_cast<uint8_t*>(&header_.checksum));
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
    if (buffer == nullptr || buffer_len < sizeof(PacketHeader) + sizeof(PacketFooter)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Parse header
    for (size_t i = 0; i < sizeof(PacketHeader); ++i) {
        reinterpret_cast<uint8_t*>(&header_)[i] = buffer[i];
    }
    
    // Verify magic numbers
    if (header_.magic_header != MAGIC_HEADER) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify payload length
    if (header_.payload_length > MAX_PAYLOAD_SIZE) {
        return MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    size_t expected_size = sizeof(PacketHeader) + header_.payload_length + sizeof(PacketFooter);
    if (buffer_len < expected_size) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Copy payload
    const uint8_t* payload_ptr = buffer + sizeof(PacketHeader);
    for (uint16_t i = 0; i < header_.payload_length; ++i) {
        payload_[i] = payload_ptr[i];
    }
    
    // Parse footer
    const uint8_t* footer_ptr = payload_ptr + header_.payload_length;
    for (size_t i = 0; i < sizeof(PacketFooter); ++i) {
        reinterpret_cast<uint8_t*>(&footer_)[i] = footer_ptr[i];
    }
    
    if (footer_.magic_footer != MAGIC_FOOTER) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    // Verify integrity
    auto verify_result = verify_integrity(crypto);
    if (verify_result.is_error()) {
        return verify_result.error();
    }
    
    // Verify signature
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
    for (size_t i = 0; i < sizeof(PacketHeader); ++i) {
        sign_data[i] = reinterpret_cast<const uint8_t*>(&header_)[i];
    }
    for (uint16_t i = 0; i < header_.payload_length; ++i) {
        sign_data[sizeof(PacketHeader) + i] = payload_[i];
    }
    
    auto sig_verify = crypto.verify(server_keypair, sign_data, 
                                    sizeof(PacketHeader) + header_.payload_length,
                                    footer_.signature);
    if (sig_verify.is_error() || !sig_verify.value()) {
        return MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    
    is_valid_ = true;
    return core::Result<void>();
}

core::Result<size_t> SecurePacket::serialize(uint8_t* buffer, size_t buffer_size) const noexcept {
    if (!is_valid_) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    size_t required_size = sizeof(PacketHeader) + header_.payload_length + sizeof(PacketFooter);
    if (buffer_size < required_size) {
        return MAKE_ERROR(core::ErrorCode::BufferOverflow);
    }
    
    // Serialize header
    for (size_t i = 0; i < sizeof(PacketHeader); ++i) {
        buffer[i] = reinterpret_cast<const uint8_t*>(&header_)[i];
    }
    
    // Serialize payload
    for (uint16_t i = 0; i < header_.payload_length; ++i) {
        buffer[sizeof(PacketHeader) + i] = payload_[i];
    }
    
    // Serialize footer
    for (size_t i = 0; i < sizeof(PacketFooter); ++i) {
        buffer[sizeof(PacketHeader) + header_.payload_length + i] = 
            reinterpret_cast<const uint8_t*>(&footer_)[i];
    }
    
    return core::Result<size_t>(required_size);
}

core::Result<void> SecurePacket::verify_integrity(security::ICryptoEngine& crypto) const noexcept {
    uint32_t computed_checksum;
    auto result = crypto.hash_sha256(payload_, header_.payload_length, 
                                     reinterpret_cast<uint8_t*>(&computed_checksum));
    if (result.is_error()) {
        return result.error();
    }
    
    if (computed_checksum != header_.checksum) {
        return MAKE_ERROR(core::ErrorCode::IntegrityViolation);
    }
    
    return core::Result<void>();
}

core::Result<void> SecurePacket::compute_signature(security::ICryptoEngine& crypto,
                                                   const security::ECCKeyPair& keypair) noexcept {
    uint8_t sign_data[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE];
    
    // Combine header and payload for signing
    for (size_t i = 0; i < sizeof(PacketHeader); ++i) {
        sign_data[i] = reinterpret_cast<const uint8_t*>(&header_)[i];
    }
    for (uint16_t i = 0; i < header_.payload_length; ++i) {
        sign_data[sizeof(PacketHeader) + i] = payload_[i];
    }
    
    return crypto.sign(keypair, sign_data, 
                      sizeof(PacketHeader) + header_.payload_length,
                      footer_.signature);
}

// PacketTransport Implementation

PacketTransport::PacketTransport(platform::IPlatformComm& comm) noexcept
    : comm_(comm) {}

core::Result<void> PacketTransport::send_packet(const SecurePacket& packet,
                                                security::ICryptoEngine& crypto,
                                                const security::ECCKeyPair& keypair) noexcept {
    if (!packet.is_valid()) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(PacketFooter)];
    
    auto serialize_result = packet.serialize(buffer, sizeof(buffer));
    if (serialize_result.is_error()) {
        return serialize_result.error();
    }
    
    size_t packet_size = serialize_result.value();
    auto send_result = comm_.send(buffer, packet_size);
    if (send_result.is_error()) {
        return send_result.error();
    }
    
    if (send_result.value() != packet_size) {
        return MAKE_ERROR(core::ErrorCode::TransmissionFailed);
    }
    
    return core::Result<void>();
}

core::Result<SecurePacket> PacketTransport::receive_packet(security::ICryptoEngine& crypto,
                                                           const security::ECCKeyPair& keypair,
                                                           uint32_t timeout_ms) noexcept {
    uint8_t buffer[sizeof(PacketHeader) + MAX_PAYLOAD_SIZE + sizeof(PacketFooter)];
    
    auto recv_result = comm_.receive(buffer, sizeof(buffer), timeout_ms);
    if (recv_result.is_error()) {
        return recv_result.error();
    }
    
    size_t received_bytes = recv_result.value();
    if (received_bytes < sizeof(PacketHeader) + sizeof(PacketFooter)) {
        return MAKE_ERROR(core::ErrorCode::InvalidPacket);
    }
    
    SecurePacket packet;
    auto parse_result = packet.parse(buffer, received_bytes, crypto, keypair);
    if (parse_result.is_error()) {
        return parse_result.error();
    }
    
    return core::Result<SecurePacket>(std::move(packet));
}

} // namespace gridshield::network