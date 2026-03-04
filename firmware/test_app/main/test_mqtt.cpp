/**
 * @file test_mqtt.cpp
 * @brief Unit tests for MQTT client interface and WiFi HAL
 *
 * Tests MockWiFi connectivity and MqttConfig validation.
 */

#include "unity.h"

#include "network/mqtt.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::network;
using namespace gridshield::platform;
using namespace gridshield::platform::mock;

// ============================================================================
// WiFi Mock Tests
// ============================================================================

static void test_wifi_init_success()
{
    MockWiFi wifi;
    auto result = wifi.init();
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_wifi_connect_disconnect()
{
    MockWiFi wifi;
    wifi.init();

    auto conn_result = wifi.connect("TestSSID", "TestPassword");
    TEST_ASSERT_TRUE(conn_result.is_ok());
    TEST_ASSERT_TRUE(wifi.is_connected());

    auto disc_result = wifi.disconnect();
    TEST_ASSERT_TRUE(disc_result.is_ok());
    TEST_ASSERT_FALSE(wifi.is_connected());
}

static void test_wifi_connect_before_init()
{
    MockWiFi wifi;
    auto result = wifi.connect("TestSSID", "TestPassword");
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SystemNotInitialized, result.error().code);
}

static void test_wifi_get_ip()
{
    MockWiFi wifi;
    wifi.init();
    wifi.connect("TestSSID", "TestPassword");

    std::array<char, MAX_IP_STRING_LENGTH> ip_buf{};
    auto result = wifi.get_ip(ip_buf.data(), ip_buf.size());
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(ip_buf[0] != '\0'); // IP string should not be empty
}

static void test_wifi_get_ip_null_buffer()
{
    MockWiFi wifi;
    wifi.init();

    auto result = wifi.get_ip(nullptr, 0);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

// ============================================================================
// MQTT Config Tests
// ============================================================================

static void test_mqtt_config_defaults()
{
    MqttConfig config{};
    TEST_ASSERT_EQUAL(MQTT_DEFAULT_PORT, config.port);
    TEST_ASSERT_EQUAL(MQTT_DEFAULT_KEEPALIVE_S, config.keepalive_s);
    TEST_ASSERT_TRUE(config.use_tls);
    TEST_ASSERT_NULL(config.broker_uri);
    TEST_ASSERT_NULL(config.client_id);
}

static void test_mqtt_config_custom()
{
    MqttConfig config{};
    config.broker_uri = "mqtt://broker.local";
    config.client_id = "gridshield_001";
    config.use_tls = false;
    config.port = MQTT_DEFAULT_PORT_NO_TLS;

    TEST_ASSERT_EQUAL_STRING("mqtt://broker.local", config.broker_uri);
    TEST_ASSERT_EQUAL_STRING("gridshield_001", config.client_id);
    TEST_ASSERT_FALSE(config.use_tls);
    TEST_ASSERT_EQUAL(MQTT_DEFAULT_PORT_NO_TLS, config.port);
}

// ============================================================================
// MQTT QoS Tests
// ============================================================================

static void test_mqtt_qos_values()
{
    static constexpr uint8_t QOS0_VALUE = 0;
    static constexpr uint8_t QOS1_VALUE = 1;
    static constexpr uint8_t QOS2_VALUE = 2;

    TEST_ASSERT_EQUAL_UINT8(QOS0_VALUE, static_cast<uint8_t>(MqttQos::AtMostOnce));
    TEST_ASSERT_EQUAL_UINT8(QOS1_VALUE, static_cast<uint8_t>(MqttQos::AtLeastOnce));
    TEST_ASSERT_EQUAL_UINT8(QOS2_VALUE, static_cast<uint8_t>(MqttQos::ExactlyOnce));
}

// ============================================================================
// MQTT Topics Tests
// ============================================================================

static void test_mqtt_topic_constants()
{
    TEST_ASSERT_EQUAL_STRING("gridshield", MqttTopics::PREFIX);
    TEST_ASSERT_EQUAL_STRING("telemetry", MqttTopics::TELEMETRY);
    TEST_ASSERT_EQUAL_STRING("alert", MqttTopics::ALERT);
    TEST_ASSERT_EQUAL_STRING("status", MqttTopics::STATUS);
    TEST_ASSERT_EQUAL_STRING("command", MqttTopics::COMMAND);
    TEST_ASSERT_EQUAL_STRING("ota", MqttTopics::OTA);
}

// ============================================================================
// ADC Mock Tests (peripheral bus testing)
// ============================================================================

static void test_adc_init_and_read()
{
    MockADC adc;
    auto init_result = adc.init(0, ADCAttenuation::Db12);
    TEST_ASSERT_TRUE(init_result.is_ok());

    static constexpr uint32_t TEST_RAW = 2048;
    adc.set_raw_value(0, TEST_RAW);

    auto raw_result = adc.read_raw(0);
    TEST_ASSERT_TRUE(raw_result.is_ok());
    TEST_ASSERT_EQUAL_UINT32(TEST_RAW, raw_result.value());

    auto mv_result = adc.read_mv(0);
    TEST_ASSERT_TRUE(mv_result.is_ok());
    TEST_ASSERT_TRUE(mv_result.value() > 0);
}

static void test_adc_invalid_channel()
{
    MockADC adc;
    static constexpr uint8_t INVALID_CHANNEL = 99;
    auto result = adc.init(INVALID_CHANNEL, ADCAttenuation::Db12);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

// ============================================================================
// I2C Mock Tests
// ============================================================================

static void test_i2c_init_and_readwrite()
{
    MockI2C i2c;
    static constexpr uint32_t I2C_FREQ_HZ = 400000;
    auto init_result = i2c.init(21, 22, I2C_FREQ_HZ);
    TEST_ASSERT_TRUE(init_result.is_ok());

    static constexpr uint8_t TEST_VALUE = 0xAB;
    static constexpr uint8_t TEST_REG = 0x10;
    static constexpr uint8_t TEST_ADDR = 0x68;

    auto write_result = i2c.write_reg(TEST_ADDR, TEST_REG, &TEST_VALUE, sizeof(TEST_VALUE));
    TEST_ASSERT_TRUE(write_result.is_ok());

    uint8_t read_val = 0;
    auto read_result = i2c.read_reg(TEST_ADDR, TEST_REG, &read_val, sizeof(read_val));
    TEST_ASSERT_TRUE(read_result.is_ok());
    TEST_ASSERT_EQUAL_UINT8(TEST_VALUE, read_val);
}

// ============================================================================
// UART Mock Tests
// ============================================================================

static void test_uart_init_and_write()
{
    MockUART uart;
    auto init_result = uart.init(0, 9600, 17, 16);
    TEST_ASSERT_TRUE(init_result.is_ok());

    static constexpr uint8_t TEST_DATA[] = {0x01, 0x02, 0x03};
    static constexpr size_t TEST_DATA_SIZE = sizeof(TEST_DATA);
    auto write_result = uart.write(0, TEST_DATA, TEST_DATA_SIZE);
    TEST_ASSERT_TRUE(write_result.is_ok());
    TEST_ASSERT_EQUAL(TEST_DATA_SIZE, write_result.value());
}

static void test_uart_inject_and_read()
{
    MockUART uart;
    uart.init(0, 9600, 17, 16);

    static constexpr uint8_t INJECT_DATA[] = {0xAA, 0xBB, 0xCC};
    static constexpr size_t INJECT_SIZE = sizeof(INJECT_DATA);
    uart.inject_rx_data(INJECT_DATA, INJECT_SIZE);

    std::array<uint8_t, 8> read_buf{};
    auto read_result = uart.read(0, read_buf.data(), read_buf.size(), 100);
    TEST_ASSERT_TRUE(read_result.is_ok());
    TEST_ASSERT_EQUAL(INJECT_SIZE, read_result.value());
    TEST_ASSERT_EQUAL_UINT8(0xAA, read_buf[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, read_buf[1]);
    TEST_ASSERT_EQUAL_UINT8(0xCC, read_buf[2]);
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_mqtt_suite(void)
{
    // WiFi
    RUN_TEST(test_wifi_init_success);
    RUN_TEST(test_wifi_connect_disconnect);
    RUN_TEST(test_wifi_connect_before_init);
    RUN_TEST(test_wifi_get_ip);
    RUN_TEST(test_wifi_get_ip_null_buffer);

    // MQTT Config
    RUN_TEST(test_mqtt_config_defaults);
    RUN_TEST(test_mqtt_config_custom);
    RUN_TEST(test_mqtt_qos_values);
    RUN_TEST(test_mqtt_topic_constants);

    // ADC peripheral
    RUN_TEST(test_adc_init_and_read);
    RUN_TEST(test_adc_invalid_channel);

    // I2C peripheral
    RUN_TEST(test_i2c_init_and_readwrite);

    // UART peripheral
    RUN_TEST(test_uart_init_and_write);
    RUN_TEST(test_uart_inject_and_read);
}
