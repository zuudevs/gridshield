/**
 * @file test_secure_packet.cpp
 * @brief Unit tests for SecurePacket build/serialize
 */

#include "unity.h"
#include "network/packet.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::network;
using namespace gridshield::security;
using namespace gridshield::core;

// Shared mock instances for crypto
static platform::mock::MockCrypto mock_crypto;
static CryptoEngine* crypto_engine = nullptr;
static ECCKeyPair device_keypair;

static void test_packet_setup(void) {
    // Create crypto engine with mock platform crypto
    crypto_engine = new CryptoEngine(mock_crypto);
    TEST_ASSERT_NOT_NULL(crypto_engine);

    // Generate test keypair
    auto result = crypto_engine->generate_keypair(device_keypair);
    TEST_ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Packet Construction
// ============================================================================

static void test_packet_build_meter_data(void) {
    SecurePacket packet;
    const uint8_t payload[] = {0x01, 0x02, 0x03, 0x04};

    auto result = packet.build(
        PacketType::MeterData,
        0x1234567890ABCDEF,
        Priority::Normal,
        payload, sizeof(payload),
        *crypto_engine,
        device_keypair
    );

    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(packet.is_valid());
    TEST_ASSERT_EQUAL(PacketType::MeterData, packet.header().type);
    TEST_ASSERT_EQUAL(0x1234567890ABCDEF, packet.header().meter_id);
    TEST_ASSERT_EQUAL(4, packet.payload_length());
}

// ============================================================================
// Serialize + Roundtrip
// ============================================================================

static void test_packet_serialize(void) {
    SecurePacket packet;
    const uint8_t payload[] = {0xDE, 0xAD};

    auto build_result = packet.build(
        PacketType::Heartbeat,
        0xAAAABBBBCCCCDDDD,
        Priority::Low,
        payload, sizeof(payload),
        *crypto_engine,
        device_keypair
    );
    TEST_ASSERT_TRUE(build_result.is_ok());

    // Serialize to buffer
    uint8_t buffer[1024];
    auto ser_result = packet.serialize(buffer, sizeof(buffer));
    TEST_ASSERT_TRUE(ser_result.is_ok());
    TEST_ASSERT_GREATER_THAN(0, ser_result.value());

    // Verify magic bytes
    TEST_ASSERT_EQUAL(MAGIC_HEADER, buffer[0]);
}

// ============================================================================
// Header Fields
// ============================================================================

static void test_packet_header_defaults(void) {
    PacketHeader h;
    TEST_ASSERT_EQUAL(MAGIC_HEADER, h.magic_header);
    TEST_ASSERT_EQUAL(PROTOCOL_VERSION, h.version);
    TEST_ASSERT_EQUAL(PacketType::Invalid, h.type);
    TEST_ASSERT_EQUAL(Priority::Normal, h.priority);
}

static void test_packet_footer_defaults(void) {
    PacketFooter f;
    TEST_ASSERT_EQUAL(MAGIC_FOOTER, f.magic_footer);
}

// ============================================================================
// Tamper Alert Packet
// ============================================================================

static void test_packet_tamper_alert(void) {
    SecurePacket packet;
    const uint8_t alert_data[] = {0x01}; // tamper type

    auto result = packet.build(
        PacketType::TamperAlert,
        0xFFFFFFFFFFFFFFFF,
        Priority::Emergency,
        alert_data, sizeof(alert_data),
        *crypto_engine,
        device_keypair
    );

    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(PacketType::TamperAlert, packet.header().type);
    TEST_ASSERT_EQUAL(Priority::Emergency, packet.header().priority);
}

static void test_packet_cleanup(void) {
    if (crypto_engine) {
        delete crypto_engine;
        crypto_engine = nullptr;
    }
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_secure_packet_suite(void) {
    RUN_TEST(test_packet_setup);
    RUN_TEST(test_packet_build_meter_data);
    RUN_TEST(test_packet_serialize);
    RUN_TEST(test_packet_header_defaults);
    RUN_TEST(test_packet_footer_defaults);
    RUN_TEST(test_packet_tamper_alert);
    RUN_TEST(test_packet_cleanup);
}
