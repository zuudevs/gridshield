/**
 * @file test_sensors.cpp
 * @brief Unit tests for sensor drivers and SensorManager
 *
 * Tests ACS712, ZMPT101B, DS18B20, MPU6050 drivers and
 * the SensorManager aggregation using mock HAL implementations.
 */

#include "unity.h"

#include "hardware/sensor_manager.hpp"
#include "hardware/sensors/acs712.hpp"
#include "hardware/sensors/ds18b20.hpp"
#include "hardware/sensors/mpu6050.hpp"
#include "hardware/sensors/zmpt101b.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::hardware::sensors;
using namespace gridshield::platform::mock;

// ============================================================================
// ACS712 Tests
// ============================================================================

static void test_acs712_init_success()
{
    MockADC adc;
    ACS712Driver driver;
    ACS712Config config{};
    config.adc_channel = 0;
    config.variant = ACS712Variant::ACS712_20A;

    auto result = driver.init(adc, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(driver.is_initialized());
}

static void test_acs712_read_zero_current()
{
    MockADC adc;
    ACS712Driver driver;
    ACS712Config config{};
    config.adc_channel = 0;
    config.variant = ACS712Variant::ACS712_20A;

    driver.init(adc, config);

    // Set ADC to read 2500mV (zero current point) — need raw value
    // Mock read_mv returns (raw * 3300) / 4095. For 2500mV output,
    // raw = (2500 * 4095) / 3300 ≈ 3102
    static constexpr uint32_t RAW_FOR_2500_MV = 3102;
    adc.set_raw_value(0, RAW_FOR_2500_MV);

    auto result = driver.read_current_ma();
    TEST_ASSERT_TRUE(result.is_ok());

    // Should be approximately 0 mA (may have slight rounding)
    int32_t current = result.value();
    static constexpr int32_t TOLERANCE_MA = 50;
    TEST_ASSERT_INT32_WITHIN(TOLERANCE_MA, 0, current);
}

static void test_acs712_read_not_initialized()
{
    ACS712Driver driver;
    auto result = driver.read_current_ma();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SystemNotInitialized, result.error().code);
}

// ============================================================================
// ZMPT101B Tests
// ============================================================================

static void test_zmpt101b_init_success()
{
    MockADC adc;
    ZMPT101BDriver driver;
    ZMPT101BConfig config{};
    config.adc_channel = 1;

    auto result = driver.init(adc, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(driver.is_initialized());
}

static void test_zmpt101b_read_voltage()
{
    MockADC adc;
    ZMPT101BDriver driver;
    ZMPT101BConfig config{};
    config.adc_channel = 1;
    static constexpr uint16_t ZMPT_VOLTAGE_RATIO = 500;
    config.voltage_ratio = ZMPT_VOLTAGE_RATIO; // x1000 fixed-point

    driver.init(adc, config);

    // Set ADC raw to simulate some voltage above zero point
    static constexpr uint32_t RAW_VALUE = 3500;
    adc.set_raw_value(1, RAW_VALUE);

    auto result = driver.read_voltage_mv();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(result.value() > 0);
}

// ============================================================================
// DS18B20 Tests
// ============================================================================

static void test_ds18b20_init_success()
{
    MockOneWire one_wire;
    DS18B20Driver driver;
    DS18B20Config config{};
    config.pin = 4;

    auto result = driver.init(one_wire, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(driver.is_initialized());
}

static void test_ds18b20_read_25_degrees()
{
    MockOneWire one_wire;
    DS18B20Driver driver;
    DS18B20Config config{};
    config.pin = 4;

    driver.init(one_wire, config);

    // 25.0°C = 400 raw (400 / 16 = 25.0)
    // Scratchpad bytes: [0x90, 0x01, ...]  (0x0190 = 400)
    static constexpr uint8_t TEMP_25C_LOW = 0x90;
    static constexpr uint8_t TEMP_25C_HIGH = 0x01;
    static constexpr size_t SCRATCHPAD_BYTES = 9;
    std::array<uint8_t, SCRATCHPAD_BYTES> scratchpad = {
        TEMP_25C_LOW, TEMP_25C_HIGH, 0, 0, 0, 0, 0, 0, 0};
    one_wire.inject_data(scratchpad.data(), scratchpad.size());

    auto result = driver.read_temperature_c10();
    TEST_ASSERT_TRUE(result.is_ok());

    // 25.0°C in 0.1°C = 250
    static constexpr int16_t EXPECTED_TEMP_C10 = 250;
    TEST_ASSERT_EQUAL_INT16(EXPECTED_TEMP_C10, result.value());
}

static void test_ds18b20_read_negative_temp()
{
    MockOneWire one_wire;
    DS18B20Driver driver;
    DS18B20Config config{};
    config.pin = 4;

    driver.init(one_wire, config);

    // -10.0°C = -160 raw = 0xFF60
    static constexpr uint8_t NEG_10C_LOW = 0x60;
    static constexpr uint8_t NEG_10C_HIGH = 0xFF;
    static constexpr size_t SCRATCHPAD_BYTES = 9;
    std::array<uint8_t, SCRATCHPAD_BYTES> scratchpad = {
        NEG_10C_LOW, NEG_10C_HIGH, 0, 0, 0, 0, 0, 0, 0};
    one_wire.inject_data(scratchpad.data(), scratchpad.size());

    auto result = driver.read_temperature_c10();
    TEST_ASSERT_TRUE(result.is_ok());

    // -10.0°C in 0.1°C = -100
    static constexpr int16_t EXPECTED_TEMP_C10 = -100;
    TEST_ASSERT_EQUAL_INT16(EXPECTED_TEMP_C10, result.value());
}

// ============================================================================
// MPU6050 Tests
// ============================================================================

static void test_mpu6050_init_success()
{
    MockI2C i2c;
    MPU6050Driver driver;
    MPU6050Config config{};

    // Set WHO_AM_I register to expected value
    i2c.set_register(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE);

    auto result = driver.init(i2c, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(driver.is_initialized());
}

static void test_mpu6050_init_wrong_whoami()
{
    MockI2C i2c;
    MPU6050Driver driver;
    MPU6050Config config{};

    // Set wrong WHO_AM_I value
    static constexpr uint8_t WRONG_WHO_AM_I = 0x00;
    i2c.set_register(MPU6050_REG_WHO_AM_I, WRONG_WHO_AM_I);

    auto result = driver.init(i2c, config);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SensorNotFound, result.error().code);
}

static void test_mpu6050_read_accel()
{
    MockI2C i2c;
    MPU6050Driver driver;
    MPU6050Config config{};

    i2c.set_register(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE);
    driver.init(i2c, config);

    // Set accelerometer data: X=0x4000 (1g = 16384), Y=0, Z=0
    static constexpr uint8_t ONE_G_HIGH = 0x40;
    static constexpr uint8_t ONE_G_LOW = 0x00;
    i2c.set_register(MPU6050_REG_ACCEL_XOUT_H, ONE_G_HIGH);
    i2c.set_register(MPU6050_REG_ACCEL_XOUT_H + 1, ONE_G_LOW);

    auto result = driver.read_accel();
    TEST_ASSERT_TRUE(result.is_ok());

    auto& accel = result.value();
    // X should be approximately 1000 mg (1g)
    static constexpr int32_t EXPECTED_MG = 1000;
    static constexpr int32_t TOLERANCE_MG = 10;
    TEST_ASSERT_INT32_WITHIN(TOLERANCE_MG, EXPECTED_MG, accel.x_mg());
}

static void test_mpu6050_shock_detection()
{
    MockI2C i2c;
    MPU6050Driver driver;
    MPU6050Config config{};
    static constexpr uint16_t SHOCK_THRESHOLD_MG = 1500;
    config.shock_threshold_mg = SHOCK_THRESHOLD_MG;

    i2c.set_register(MPU6050_REG_WHO_AM_I, MPU6050_WHO_AM_I_VALUE);
    driver.init(i2c, config);

    // Set X acceleration to 2g = 32768 raw = ~2000 mg (above 1500 mg threshold)
    static constexpr uint8_t TWO_G_HIGH = 0x80;
    static constexpr uint8_t TWO_G_LOW = 0x00;
    i2c.set_register(MPU6050_REG_ACCEL_XOUT_H, TWO_G_HIGH);
    i2c.set_register(MPU6050_REG_ACCEL_XOUT_H + 1, TWO_G_LOW);

    auto result = driver.detect_shock();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(result.value()); // Shock should be detected
}

// ============================================================================
// SensorManager Tests
// ============================================================================

static void test_sensor_manager_init_no_sensors()
{
    hardware::SensorManager manager;
    platform::PlatformServices platform{};
    hardware::SensorManagerConfig config{};

    auto result = manager.initialize(platform, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(manager.is_initialized());
}

static void test_sensor_manager_read_with_acs712()
{
    MockADC adc;
    hardware::SensorManager manager;

    platform::PlatformServices platform{};
    platform.adc = &adc;

    hardware::SensorManagerConfig config{};
    config.enable_acs712 = true;
    config.acs712_config.adc_channel = 0;
    config.acs712_config.variant = ACS712Variant::ACS712_20A;

    auto init_result = manager.initialize(platform, config);
    TEST_ASSERT_TRUE(init_result.is_ok());

    // Set some ADC value
    static constexpr uint32_t ADC_RAW = 3500;
    adc.set_raw_value(0, ADC_RAW);

    auto read_result = manager.read_all();
    TEST_ASSERT_TRUE(read_result.is_ok());
}

static void test_sensor_manager_to_meter_reading()
{
    hardware::SensorData data{};
    static constexpr uint32_t TEST_VOLTAGE = 220000;
    static constexpr uint32_t TEST_CURRENT = 5000;
    static constexpr uint32_t TEST_ENERGY = 1234;
    static constexpr core::timestamp_t TEST_TIMESTAMP = 1000;

    data.voltage_mv = TEST_VOLTAGE;
    data.current_ma = TEST_CURRENT;
    data.energy_wh = TEST_ENERGY;

    auto reading = hardware::SensorManager::to_meter_reading(data, TEST_TIMESTAMP);
    TEST_ASSERT_EQUAL_UINT32(TEST_VOLTAGE, reading.voltage_mv);
    TEST_ASSERT_EQUAL_UINT16(static_cast<uint16_t>(TEST_CURRENT), reading.current_ma);
    TEST_ASSERT_EQUAL_UINT32(TEST_ENERGY, reading.energy_wh);
    TEST_ASSERT_EQUAL_UINT32(TEST_TIMESTAMP, reading.timestamp);
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_sensors_suite(void)
{
    // ACS712
    RUN_TEST(test_acs712_init_success);
    RUN_TEST(test_acs712_read_zero_current);
    RUN_TEST(test_acs712_read_not_initialized);

    // ZMPT101B
    RUN_TEST(test_zmpt101b_init_success);
    RUN_TEST(test_zmpt101b_read_voltage);

    // DS18B20
    RUN_TEST(test_ds18b20_init_success);
    RUN_TEST(test_ds18b20_read_25_degrees);
    RUN_TEST(test_ds18b20_read_negative_temp);

    // MPU6050
    RUN_TEST(test_mpu6050_init_success);
    RUN_TEST(test_mpu6050_init_wrong_whoami);
    RUN_TEST(test_mpu6050_read_accel);
    RUN_TEST(test_mpu6050_shock_detection);

    // SensorManager
    RUN_TEST(test_sensor_manager_init_no_sensors);
    RUN_TEST(test_sensor_manager_read_with_acs712);
    RUN_TEST(test_sensor_manager_to_meter_reading);
}
