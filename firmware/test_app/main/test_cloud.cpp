/**
 * @file test_cloud.cpp
 * @brief Unit tests for v3.0.0 cloud integration
 *
 * Tests cloud connector, AWS IoT, Azure IoT, and device provisioning.
 */

#include "unity.h"

#include "cloud/aws_iot.hpp"
#include "cloud/azure_iot.hpp"
#include "cloud/cloud_connector.hpp"
#include "cloud/device_provisioning.hpp"


using namespace gridshield;
using namespace gridshield::cloud;

// ============================================================================
// Helper: create a valid CloudConfig
// ============================================================================

static CloudConfig make_test_config()
{
    CloudConfig config{};
    std::strncpy(config.endpoint.data(), "test.iot.amazonaws.com", CLOUD_MAX_ENDPOINT_LENGTH - 1);
    std::strncpy(config.device_id.data(), "gridshield_001", CLOUD_MAX_DEVICE_ID_LENGTH - 1);
    return config;
}

// ============================================================================
// AWS IoT Tests
// ============================================================================

static void test_aws_init_success()
{
    AwsIotConnector aws;
    auto result = aws.init(make_test_config());
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(CloudProvider::AwsIotCore, aws.provider());
}

static void test_aws_init_empty_endpoint()
{
    AwsIotConnector aws;
    CloudConfig config{};
    auto result = aws.init(config);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

static void test_aws_connect_disconnect()
{
    AwsIotConnector aws;
    aws.init(make_test_config());

    auto conn = aws.connect();
    TEST_ASSERT_TRUE(conn.is_ok());
    TEST_ASSERT_EQUAL(CloudState::Connected, aws.state());
    TEST_ASSERT_TRUE(aws.shadow().reported.online);

    auto disc = aws.disconnect();
    TEST_ASSERT_TRUE(disc.is_ok());
    TEST_ASSERT_EQUAL(CloudState::Disconnected, aws.state());
    TEST_ASSERT_FALSE(aws.shadow().reported.online);
}

static void test_aws_publish_telemetry()
{
    AwsIotConnector aws;
    aws.init(make_test_config());
    aws.connect();

    TelemetryPayload payload{};
    static constexpr uint32_t TEST_VOLTAGE = 230000;
    static constexpr uint32_t TEST_CURRENT = 5000;
    payload.voltage_mv = TEST_VOLTAGE;
    payload.current_ma = TEST_CURRENT;

    auto result = aws.publish_telemetry(payload);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(1, aws.publish_count());
    TEST_ASSERT_EQUAL(TEST_VOLTAGE, aws.shadow().reported.voltage_mv);
}

static void test_aws_publish_not_connected()
{
    AwsIotConnector aws;
    aws.init(make_test_config());

    TelemetryPayload payload{};
    auto result = aws.publish_telemetry(payload);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::CloudPublishFailed, result.error().code);
}

static void test_aws_connect_failure()
{
    AwsIotConnector aws;
    aws.init(make_test_config());
    aws.set_simulate_connect_failure(true);

    auto result = aws.connect();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::CloudConnectionFailed, result.error().code);
    TEST_ASSERT_EQUAL(CloudState::Error, aws.state());
}

static void test_aws_subscribe_and_receive()
{
    AwsIotConnector aws;
    aws.init(make_test_config());
    aws.connect();

    auto sub = aws.subscribe_commands();
    TEST_ASSERT_TRUE(sub.is_ok());
    TEST_ASSERT_TRUE(aws.is_ready());

    // Inject command
    CloudCommand cmd{};
    std::strncpy(cmd.topic.data(), "gridshield/commands/reset", CLOUD_MAX_TOPIC_LENGTH - 1);
    cmd.valid = true;
    aws.inject_command(cmd);

    TEST_ASSERT_TRUE(aws.has_pending_command());
    auto recv = aws.receive_command();
    TEST_ASSERT_TRUE(recv.is_ok());
    TEST_ASSERT_EQUAL_STRING("gridshield/commands/reset", recv.value().topic.data());
}

// ============================================================================
// Azure IoT Tests
// ============================================================================

static void test_azure_init_success()
{
    AzureIotConnector azure;
    CloudConfig config{};
    std::strncpy(config.endpoint.data(), "myhub.azure-devices.net", CLOUD_MAX_ENDPOINT_LENGTH - 1);
    std::strncpy(config.device_id.data(), "gridshield_002", CLOUD_MAX_DEVICE_ID_LENGTH - 1);
    auto result = azure.init(config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(CloudProvider::AzureIotHub, azure.provider());
}

static void test_azure_publish_telemetry()
{
    AzureIotConnector azure;
    CloudConfig config{};
    std::strncpy(config.endpoint.data(), "myhub.azure-devices.net", CLOUD_MAX_ENDPOINT_LENGTH - 1);
    std::strncpy(config.device_id.data(), "gridshield_002", CLOUD_MAX_DEVICE_ID_LENGTH - 1);
    azure.init(config);
    azure.connect();

    TelemetryPayload payload{};
    static constexpr uint32_t TEST_ENERGY = 12345;
    payload.energy_wh = TEST_ENERGY;

    auto result = azure.publish_telemetry(payload);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(TEST_ENERGY, azure.twin().reported.energy_wh);
}

static void test_azure_sas_token_expiry()
{
    AzureIotConnector azure;
    CloudConfig config{};
    std::strncpy(config.endpoint.data(), "myhub.azure-devices.net", CLOUD_MAX_ENDPOINT_LENGTH - 1);
    std::strncpy(config.device_id.data(), "gridshield_002", CLOUD_MAX_DEVICE_ID_LENGTH - 1);
    azure.init(config);

    // No token set — should be expired
    static constexpr uint64_t CURRENT_TIME = 1700000000;
    TEST_ASSERT_TRUE(azure.is_sas_expired(CURRENT_TIME));

    // Set a valid token
    SasToken token{};
    static constexpr uint64_t TOKEN_EXPIRY = 1700003600;
    token.expiry_epoch_s = TOKEN_EXPIRY;
    token.valid = true;
    azure.set_sas_token(token);

    TEST_ASSERT_FALSE(azure.is_sas_expired(CURRENT_TIME));
    static constexpr uint64_t AFTER_EXPIRY = 1700003601;
    TEST_ASSERT_TRUE(azure.is_sas_expired(AFTER_EXPIRY));
}

// ============================================================================
// Device Provisioning Tests
// ============================================================================

static void test_provisioning_success()
{
    DeviceProvisioning prov;
    ProvisioningConfig config{};
    std::strncpy(config.registration_id.data(), "gridshield_001", PROV_MAX_REG_ID_LENGTH - 1);
    std::strncpy(config.scope_id.data(), "0ne001234", PROV_MAX_SCOPE_ID_LENGTH - 1);
    prov.init(config);

    ProvisioningCredentials sim_creds{};
    std::strncpy(
        sim_creds.assigned_endpoint.data(), "hub.iot.amazonaws.com", PROV_MAX_ENDPOINT_LENGTH - 1);
    std::strncpy(
        sim_creds.assigned_device_id.data(), "gridshield_001", PROV_MAX_DEVICE_ID_LENGTH - 1);
    prov.set_simulated_credentials(sim_creds);

    auto result = prov.provision();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(prov.is_provisioned());
    TEST_ASSERT_EQUAL_STRING("hub.iot.amazonaws.com", result.value().assigned_endpoint.data());
}

static void test_provisioning_failure()
{
    DeviceProvisioning prov;
    ProvisioningConfig config{};
    std::strncpy(config.registration_id.data(), "test_device", PROV_MAX_REG_ID_LENGTH - 1);
    std::strncpy(config.scope_id.data(), "scope_001", PROV_MAX_SCOPE_ID_LENGTH - 1);
    prov.init(config);
    prov.set_simulate_failure(true);

    auto result = prov.provision();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::CloudProvisioningFailed, result.error().code);
    TEST_ASSERT_FALSE(prov.is_provisioned());
}

static void test_provisioning_not_init()
{
    DeviceProvisioning prov;
    auto result = prov.provision();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SystemNotInitialized, result.error().code);
}

static void test_telemetry_from_reading()
{
    core::MeterReading reading{};
    static constexpr uint32_t READING_VOLTAGE = 220000;
    static constexpr uint16_t READING_CURRENT = 3000;
    static constexpr uint32_t READING_ENERGY = 9876;
    reading.voltage_mv = READING_VOLTAGE;
    reading.current_ma = READING_CURRENT;
    reading.energy_wh = READING_ENERGY;

    static constexpr int16_t TEMP = 250;
    auto payload = TelemetryPayload::from_reading(reading, TEMP, true, "dev_01");

    TEST_ASSERT_EQUAL(READING_VOLTAGE, payload.voltage_mv);
    TEST_ASSERT_EQUAL(READING_CURRENT, payload.current_ma);
    TEST_ASSERT_EQUAL(READING_ENERGY, payload.energy_wh);
    TEST_ASSERT_EQUAL(TEMP, payload.temperature_c10);
    TEST_ASSERT_TRUE(payload.tamper_detected);
    TEST_ASSERT_EQUAL_STRING("dev_01", payload.device_id.data());
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_cloud_suite(void)
{
    // AWS IoT
    RUN_TEST(test_aws_init_success);
    RUN_TEST(test_aws_init_empty_endpoint);
    RUN_TEST(test_aws_connect_disconnect);
    RUN_TEST(test_aws_publish_telemetry);
    RUN_TEST(test_aws_publish_not_connected);
    RUN_TEST(test_aws_connect_failure);
    RUN_TEST(test_aws_subscribe_and_receive);

    // Azure IoT
    RUN_TEST(test_azure_init_success);
    RUN_TEST(test_azure_publish_telemetry);
    RUN_TEST(test_azure_sas_token_expiry);

    // Device Provisioning
    RUN_TEST(test_provisioning_success);
    RUN_TEST(test_provisioning_failure);
    RUN_TEST(test_provisioning_not_init);

    // Telemetry
    RUN_TEST(test_telemetry_from_reading);
}
