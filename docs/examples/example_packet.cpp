/**
 * @file example_packet.cpp
 * @brief GridShield SecurePacket — Usage Example
 *
 * Demonstrates:
 *   - Building a signed packet
 *   - Serializing to wire format
 *   - Parsing from raw bytes
 *   - Using PacketTransport for send/receive
 */

#include "network/packet.hpp"
#include "platform/mock_platform.hpp"
#include "security/crypto.hpp"


using namespace gridshield;
using namespace gridshield::network;
using namespace gridshield::security;

void example_packet() {
  // ---------------------------------------------------------------
  // 1. Setup: crypto engine and keypair
  // ---------------------------------------------------------------
  platform::mock::MockCrypto mock_crypto;
  CryptoEngine crypto(mock_crypto);

  ECCKeyPair device_keypair;
  auto gen_result = crypto.generate_keypair(device_keypair);
  if (gen_result.is_error())
    return;

  // ---------------------------------------------------------------
  // 2. Build a Signed Packet
  // ---------------------------------------------------------------
  SecurePacket packet;
  const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};

  auto build_result =
      packet.build(PacketType::MeterData,    // Packet type
                   0x1234567890ABCDEF,       // Meter ID
                   core::Priority::Normal,   // Priority level
                   payload, sizeof(payload), // Payload data
                   crypto,                   // Crypto engine (for signing)
                   device_keypair // Device keypair (private key signs)
      );

  if (build_result.is_error()) {
    return;
  }

  // Access packet fields
  const PacketHeader &hdr = packet.header();
  // hdr.type        → PacketType::MeterData
  // hdr.meter_id    → 0x1234567890ABCDEF
  // hdr.priority    → Priority::Normal
  // hdr.sequence    → auto-incrementing sequence number
  // packet.payload()        → pointer to payload bytes
  // packet.payload_length() → 4
  // packet.is_valid()       → true
  (void)hdr;

  // ---------------------------------------------------------------
  // 3. Serialize to Wire Format
  // ---------------------------------------------------------------
  uint8_t wire_buffer[1024];
  auto ser_result = packet.serialize(wire_buffer, sizeof(wire_buffer));
  if (ser_result.is_error()) {
    return;
  }
  size_t wire_length = ser_result.value();

  // wire_buffer[0..wire_length] now contains:
  //   [Header][Payload][Signature][Footer]

  // ---------------------------------------------------------------
  // 4. Parse from Raw Bytes (receiver side)
  // ---------------------------------------------------------------
  SecurePacket received_packet;
  auto parse_result = received_packet.parse(
      wire_buffer, wire_length,
      crypto,        // Crypto engine (for verification)
      device_keypair // Server/sender public key (verifies signature)
  );

  if (parse_result.is_error()) {
    // Parse failed: invalid magic, checksum mismatch, or bad signature
    return;
  }

  // Parsed packet is verified and ready to use
  // received_packet.header().type == PacketType::MeterData
  // received_packet.payload_length() == 4

  // ---------------------------------------------------------------
  // 5. Tamper Alert Packet (highest priority)
  // ---------------------------------------------------------------
  SecurePacket alert_packet;
  const uint8_t alert_data[] = {0x01}; // Tamper type: CasingOpened

  auto alert_result = alert_packet.build(
      PacketType::TamperAlert, 0x1234567890ABCDEF, core::Priority::Emergency,
      alert_data, sizeof(alert_data), crypto, device_keypair);
  (void)alert_result;

  // ---------------------------------------------------------------
  // 6. Using PacketTransport (send over communication channel)
  // ---------------------------------------------------------------
  platform::mock::MockComm mock_comm;
  mock_comm.init();

  PacketTransport transport(mock_comm);

  auto send_result = transport.send_packet(packet, crypto, device_keypair);
  if (send_result.is_error()) {
    // Network error: disconnected, timeout, etc.
  }
}
