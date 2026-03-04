/**
 * @file test_protocols.cpp
 * @brief Unit tests for v3.0.0 protocol enhancements
 *
 * Tests SNTP client, ESP-NOW mesh, remote config, and cert manager.
 */

#include "unity.h"

#include "network/esp_now_mesh.hpp"
#include "network/remote_config.hpp"
#include "network/sntp_client.hpp"
#include "security/cert_manager.hpp"


using namespace gridshield;
using namespace gridshield::network;
using namespace gridshield::security;

// ============================================================================
// SNTP Tests
// ============================================================================

static void test_sntp_init_success()
{
    SntpClient client;
    SntpConfig config{};
    static constexpr char SERVER[] = "pool.ntp.org";
    std::strncpy(config.servers[0].data(), SERVER, SNTP_MAX_HOST_LENGTH - 1);
    config.server_count = 1;
    auto result = client.init(config);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_sntp_init_no_servers()
{
    SntpClient client;
    SntpConfig config{};
    config.server_count = 0;
    auto result = client.init(config);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

static void test_sntp_sync_success()
{
    SntpClient client;
    SntpConfig config{};
    std::strncpy(config.servers[0].data(), "pool.ntp.org", SNTP_MAX_HOST_LENGTH - 1);
    config.server_count = 1;
    client.init(config);

    static constexpr uint64_t TEST_EPOCH = 1700000000;
    client.set_simulated_epoch(TEST_EPOCH);

    auto result = client.sync();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(client.is_synced());

    auto time_result = client.get_epoch_s();
    TEST_ASSERT_TRUE(time_result.is_ok());
    TEST_ASSERT_EQUAL(TEST_EPOCH, time_result.value());
}

static void test_sntp_sync_failure()
{
    SntpClient client;
    SntpConfig config{};
    std::strncpy(config.servers[0].data(), "pool.ntp.org", SNTP_MAX_HOST_LENGTH - 1);
    config.server_count = 1;
    client.init(config);
    client.set_simulate_failure(true);

    auto result = client.sync();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SntpSyncFailed, result.error().code);
    TEST_ASSERT_FALSE(client.is_synced());
}

static void test_sntp_get_time_before_sync()
{
    SntpClient client;
    SntpConfig config{};
    std::strncpy(config.servers[0].data(), "pool.ntp.org", SNTP_MAX_HOST_LENGTH - 1);
    config.server_count = 1;
    client.init(config);

    auto result = client.get_time();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SntpNotSynced, result.error().code);
}

// ============================================================================
// ESP-NOW Mesh Tests
// ============================================================================

static void test_mesh_init_success()
{
    EspNowMesh mesh;
    MeshConfig config{};
    config.channel = 6;
    auto result = mesh.init(config);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_mesh_init_invalid_channel()
{
    EspNowMesh mesh;
    MeshConfig config{};
    config.channel = 0; // invalid
    auto result = mesh.init(config);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

static void test_mesh_add_remove_peer()
{
    EspNowMesh mesh;
    MeshConfig config{};
    config.channel = 1;
    mesh.init(config);

    std::array<uint8_t, MESH_MAC_LENGTH> mac = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
    auto add_result = mesh.add_peer(mac, false);
    TEST_ASSERT_TRUE(add_result.is_ok());
    TEST_ASSERT_EQUAL(1, mesh.peer_count());

    auto rm_result = mesh.remove_peer(mac);
    TEST_ASSERT_TRUE(rm_result.is_ok());
    TEST_ASSERT_EQUAL(0, mesh.peer_count());
}

static void test_mesh_send_and_receive()
{
    EspNowMesh mesh;
    MeshConfig config{};
    config.channel = 1;
    mesh.init(config);

    std::array<uint8_t, MESH_MAC_LENGTH> mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    mesh.add_peer(mac, false);

    static constexpr uint8_t PAYLOAD[] = {0xDE, 0xAD};
    auto send_result = mesh.send(mac, PAYLOAD, sizeof(PAYLOAD));
    TEST_ASSERT_TRUE(send_result.is_ok());

    // Inject a received packet
    MeshPacket pkt{};
    pkt.header.payload_len = 2;
    pkt.payload[0] = 0xBE;
    pkt.payload[1] = 0xEF;
    mesh.inject_packet(pkt);

    TEST_ASSERT_TRUE(mesh.has_pending_packet());
    auto recv_result = mesh.receive();
    TEST_ASSERT_TRUE(recv_result.is_ok());
    TEST_ASSERT_EQUAL_UINT8(0xBE, recv_result.value().payload[0]);
}

static void test_mesh_send_failure()
{
    EspNowMesh mesh;
    MeshConfig config{};
    config.channel = 1;
    mesh.init(config);
    mesh.set_simulate_send_failure(true);

    std::array<uint8_t, MESH_MAC_LENGTH> mac = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06};
    static constexpr uint8_t DATA[] = {0x01};
    auto result = mesh.send(mac, DATA, sizeof(DATA));
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::MeshSendFailed, result.error().code);
}

// ============================================================================
// Remote Config Tests
// ============================================================================

static void test_config_init_success()
{
    RemoteConfigManager mgr;
    RemoteConfigSettings settings{};
    settings.config_topic = "gridshield/config";
    auto result = mgr.init(settings);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_config_apply_and_get()
{
    RemoteConfigManager mgr;
    RemoteConfigSettings settings{};
    settings.config_topic = "gridshield/config";
    settings.validate_checksum = false;
    mgr.init(settings);

    ConfigSchema schema{};
    std::strncpy(schema.version.data(), "1.0.0", CONFIG_VERSION_LENGTH - 1);
    schema.entry_count = 1;
    std::strncpy(schema.entries[0].key.data(), "interval_ms", CONFIG_MAX_KEY_LENGTH - 1);
    std::strncpy(schema.entries[0].value.data(), "5000", CONFIG_MAX_VALUE_LENGTH - 1);
    schema.entries[0].active = true;

    auto apply_result = mgr.apply(schema);
    TEST_ASSERT_TRUE(apply_result.is_ok());

    auto get_result = mgr.get("interval_ms");
    TEST_ASSERT_TRUE(get_result.is_ok());
    TEST_ASSERT_EQUAL_STRING("5000", get_result.value());
}

static void test_config_rollback()
{
    RemoteConfigManager mgr;
    RemoteConfigSettings settings{};
    settings.config_topic = "gridshield/config";
    settings.validate_checksum = false;
    mgr.init(settings);

    // Apply first config
    ConfigSchema schema1{};
    schema1.entry_count = 1;
    std::strncpy(schema1.entries[0].key.data(), "key1", CONFIG_MAX_KEY_LENGTH - 1);
    std::strncpy(schema1.entries[0].value.data(), "valueA", CONFIG_MAX_VALUE_LENGTH - 1);
    schema1.entries[0].active = true;
    mgr.apply(schema1);

    // Apply second config
    ConfigSchema schema2{};
    schema2.entry_count = 1;
    std::strncpy(schema2.entries[0].key.data(), "key1", CONFIG_MAX_KEY_LENGTH - 1);
    std::strncpy(schema2.entries[0].value.data(), "valueB", CONFIG_MAX_VALUE_LENGTH - 1);
    schema2.entries[0].active = true;
    mgr.apply(schema2);

    // Rollback
    auto rb_result = mgr.rollback();
    TEST_ASSERT_TRUE(rb_result.is_ok());

    auto val = mgr.get("key1");
    TEST_ASSERT_TRUE(val.is_ok());
    TEST_ASSERT_EQUAL_STRING("valueA", val.value());
}

// ============================================================================
// Certificate Manager Tests
// ============================================================================

static void test_cert_init_and_load()
{
    CertManager mgr;
    auto init_result = mgr.init();
    TEST_ASSERT_TRUE(init_result.is_ok());

    static constexpr uint8_t FAKE_CERT[] = {0x30, 0x82, 0x01, 0x00};
    auto load_result = mgr.load_ca(FAKE_CERT, sizeof(FAKE_CERT), CertFormat::DER);
    TEST_ASSERT_TRUE(load_result.is_ok());
    TEST_ASSERT_TRUE(mgr.has_cert(CertType::CaCertificate));
}

static void test_cert_not_loaded()
{
    CertManager mgr;
    mgr.init();
    TEST_ASSERT_FALSE(mgr.has_cert(CertType::ClientCertificate));

    auto info_result = mgr.get_info(CertType::ClientCertificate);
    TEST_ASSERT_TRUE(info_result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::CertificateNotFound, info_result.error().code);
}

static void test_cert_expiry_check()
{
    CertManager mgr;
    mgr.init();

    static constexpr uint8_t FAKE_CERT[] = {0x30};
    mgr.load_ca(FAKE_CERT, sizeof(FAKE_CERT), CertFormat::DER);

    // Set expiry info
    CertInfo info{};
    static constexpr uint64_t EXPIRY_EPOCH = 1700000000;
    info.not_after = EXPIRY_EPOCH;
    mgr.set_cert_info(CertType::CaCertificate, info);

    // Before expiry
    static constexpr uint64_t BEFORE_EXPIRY = 1699999999;
    auto before = mgr.is_expired(CertType::CaCertificate, BEFORE_EXPIRY);
    TEST_ASSERT_TRUE(before.is_ok());
    TEST_ASSERT_FALSE(before.value());

    // After expiry
    static constexpr uint64_t AFTER_EXPIRY = 1700000001;
    auto after = mgr.is_expired(CertType::CaCertificate, AFTER_EXPIRY);
    TEST_ASSERT_TRUE(after.is_ok());
    TEST_ASSERT_TRUE(after.value());
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_protocols_suite(void)
{
    // SNTP
    RUN_TEST(test_sntp_init_success);
    RUN_TEST(test_sntp_init_no_servers);
    RUN_TEST(test_sntp_sync_success);
    RUN_TEST(test_sntp_sync_failure);
    RUN_TEST(test_sntp_get_time_before_sync);

    // ESP-NOW Mesh
    RUN_TEST(test_mesh_init_success);
    RUN_TEST(test_mesh_init_invalid_channel);
    RUN_TEST(test_mesh_add_remove_peer);
    RUN_TEST(test_mesh_send_and_receive);
    RUN_TEST(test_mesh_send_failure);

    // Remote Config
    RUN_TEST(test_config_init_success);
    RUN_TEST(test_config_apply_and_get);
    RUN_TEST(test_config_rollback);

    // Certificate Manager
    RUN_TEST(test_cert_init_and_load);
    RUN_TEST(test_cert_not_loaded);
    RUN_TEST(test_cert_expiry_check);
}
