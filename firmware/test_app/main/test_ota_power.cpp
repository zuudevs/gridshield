/**
 * @file test_ota_power.cpp
 * @brief Unit tests for OTA Manager and Power Manager
 */

#include "unity.h"

#include "platform/mock_platform.hpp"
#include "system/ota_manager.hpp"
#include "system/power_manager.hpp"


using namespace gridshield;
using namespace gridshield::system;

// ============================================================================
// OTA Manager Tests
// ============================================================================

static void test_ota_init()
{
    OtaManager ota;
    OtaConfig config{};
    config.update_url = "https://update.gridshield.io/firmware";
    config.require_signature = true;

    auto result = ota.init(config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(ota.is_initialized());
    TEST_ASSERT_EQUAL(OtaState::Idle, ota.get_state());
}

static void test_ota_check_update()
{
    OtaManager ota;
    OtaConfig config{};
    config.update_url = "https://update.gridshield.io/firmware";
    ota.init(config);

    auto result = ota.check_update();
    TEST_ASSERT_TRUE(result.is_ok());

    // Stub returns empty info (no update)
    auto& info = result.value();
    TEST_ASSERT_EQUAL_UINT32(0, info.size_bytes);
    TEST_ASSERT_EQUAL(OtaState::Idle, ota.get_state());
}

static void test_ota_start_update()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    OtaFirmwareInfo info{};
    static constexpr uint32_t FIRMWARE_SIZE = 524288; // 512 KB
    info.size_bytes = FIRMWARE_SIZE;

    auto result = ota.start_update(info);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(OtaState::Downloading, ota.get_state());
}

static void test_ota_start_update_invalid_size()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    OtaFirmwareInfo info{};
    info.size_bytes = 0; // Invalid

    auto result = ota.start_update(info);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, result.error().code);
}

static void test_ota_progress_tracking()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    OtaFirmwareInfo info{};
    static constexpr uint32_t FIRMWARE_SIZE = 100000;
    info.size_bytes = FIRMWARE_SIZE;
    ota.start_update(info);

    // Simulate halfway download
    static constexpr uint32_t HALF_DOWNLOADED = 50000;
    static constexpr uint8_t EXPECTED_PERCENT = 50;
    ota.simulate_download_progress(HALF_DOWNLOADED);

    auto prog_result = ota.get_progress();
    TEST_ASSERT_TRUE(prog_result.is_ok());
    TEST_ASSERT_EQUAL_UINT32(HALF_DOWNLOADED, prog_result.value().bytes_downloaded);
    TEST_ASSERT_EQUAL_UINT8(EXPECTED_PERCENT, prog_result.value().percent);
}

static void test_ota_abort()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    OtaFirmwareInfo info{};
    static constexpr uint32_t FIRMWARE_SIZE = 100000;
    info.size_bytes = FIRMWARE_SIZE;
    ota.start_update(info);

    auto result = ota.abort_update();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(OtaState::Idle, ota.get_state());
}

static void test_ota_confirm_firmware()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    TEST_ASSERT_FALSE(ota.is_firmware_confirmed());

    auto result = ota.confirm_firmware();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(ota.is_firmware_confirmed());
}

static void test_ota_rollback()
{
    OtaManager ota;
    OtaConfig config{};
    ota.init(config);

    auto result = ota.rollback();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(OtaState::RolledBack, ota.get_state());
}

static void test_ota_not_initialized()
{
    OtaManager ota;

    auto check = ota.check_update();
    TEST_ASSERT_TRUE(check.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SystemNotInitialized, check.error().code);

    auto abort = ota.abort_update();
    TEST_ASSERT_TRUE(abort.is_error());

    auto confirm = ota.confirm_firmware();
    TEST_ASSERT_TRUE(confirm.is_error());
}

// ============================================================================
// Power Manager Tests
// ============================================================================

static void test_power_init()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};

    auto result = pwr.init(platform, config);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(pwr.is_initialized());
    TEST_ASSERT_EQUAL(PowerState::Active, pwr.get_state());
}

static void test_power_light_sleep()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    pwr.init(platform, config);

    static constexpr uint32_t SLEEP_DURATION_MS = 5000;
    pwr.set_wake_reason(WakeReason::Timer);

    auto result = pwr.enter_light_sleep(SLEEP_DURATION_MS);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(WakeReason::Timer, result.value());
    TEST_ASSERT_EQUAL(PowerState::Active, pwr.get_state()); // Back to active after wake
}

static void test_power_light_sleep_invalid_duration()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    pwr.init(platform, config);

    // Too short
    static constexpr uint32_t TOO_SHORT_MS = 10;
    auto short_result = pwr.enter_light_sleep(TOO_SHORT_MS);
    TEST_ASSERT_TRUE(short_result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, short_result.error().code);

    // Too long
    static constexpr uint32_t TOO_LONG_MS = 5000000;
    auto long_result = pwr.enter_light_sleep(TOO_LONG_MS);
    TEST_ASSERT_TRUE(long_result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InvalidParameter, long_result.error().code);
}

static void test_power_deep_sleep()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    pwr.init(platform, config);

    static constexpr uint32_t SLEEP_DURATION_MS = 60000;
    auto result = pwr.enter_deep_sleep(SLEEP_DURATION_MS);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(PowerState::DeepSleep, pwr.get_state());
}

static void test_power_adaptive_interval_mains()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    config.wake_interval_ms = POWER_DEFAULT_WAKE_INTERVAL_MS;
    config.adaptive_duty_cycle = true;
    pwr.init(platform, config);

    BatteryStatus battery{};
    battery.source = PowerSource::Mains;
    pwr.set_battery_status(battery);

    uint32_t interval = pwr.calculate_adaptive_interval();
    TEST_ASSERT_EQUAL_UINT32(POWER_DEFAULT_WAKE_INTERVAL_MS, interval);
}

static void test_power_adaptive_interval_battery_low()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    config.wake_interval_ms = POWER_DEFAULT_WAKE_INTERVAL_MS;
    config.adaptive_duty_cycle = true;
    pwr.init(platform, config);

    BatteryStatus battery{};
    battery.source = PowerSource::Battery;
    battery.voltage_mv = POWER_BATTERY_LOW_MV; // Low battery
    pwr.set_battery_status(battery);

    uint32_t interval = pwr.calculate_adaptive_interval();
    // Should be doubled at low battery
    static constexpr uint32_t LOW_MULTIPLIER = 2;
    TEST_ASSERT_EQUAL_UINT32(POWER_DEFAULT_WAKE_INTERVAL_MS * LOW_MULTIPLIER, interval);
}

static void test_power_adaptive_interval_battery_critical()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    config.wake_interval_ms = POWER_DEFAULT_WAKE_INTERVAL_MS;
    config.adaptive_duty_cycle = true;
    pwr.init(platform, config);

    BatteryStatus battery{};
    battery.source = PowerSource::Battery;
    battery.voltage_mv = POWER_BATTERY_CRITICAL_MV; // Critical
    pwr.set_battery_status(battery);

    uint32_t interval = pwr.calculate_adaptive_interval();
    // Should be 10x at critical battery
    static constexpr uint32_t CRITICAL_MULTIPLIER = 10;
    TEST_ASSERT_EQUAL_UINT32(POWER_DEFAULT_WAKE_INTERVAL_MS * CRITICAL_MULTIPLIER, interval);
}

static void test_power_read_battery()
{
    PowerManager pwr;
    platform::PlatformServices platform{};
    PowerConfig config{};
    pwr.init(platform, config);

    BatteryStatus expected{};
    expected.voltage_mv = POWER_BATTERY_FULL_MV;
    expected.percent = POWER_BATTERY_PERCENT_FULL;
    expected.charging = true;
    expected.source = PowerSource::USB;
    pwr.set_battery_status(expected);

    auto result = pwr.read_battery_status();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL_UINT16(POWER_BATTERY_FULL_MV, result.value().voltage_mv);
    TEST_ASSERT_EQUAL_UINT8(POWER_BATTERY_PERCENT_FULL, result.value().percent);
    TEST_ASSERT_TRUE(result.value().charging);
}

static void test_power_not_initialized()
{
    PowerManager pwr;

    static constexpr uint32_t SLEEP_MS = 5000;
    auto light = pwr.enter_light_sleep(SLEEP_MS);
    TEST_ASSERT_TRUE(light.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::SystemNotInitialized, light.error().code);

    auto deep = pwr.enter_deep_sleep(SLEEP_MS);
    TEST_ASSERT_TRUE(deep.is_error());

    auto battery = pwr.read_battery_status();
    TEST_ASSERT_TRUE(battery.is_error());
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_ota_power_suite(void)
{
    // OTA Manager
    RUN_TEST(test_ota_init);
    RUN_TEST(test_ota_check_update);
    RUN_TEST(test_ota_start_update);
    RUN_TEST(test_ota_start_update_invalid_size);
    RUN_TEST(test_ota_progress_tracking);
    RUN_TEST(test_ota_abort);
    RUN_TEST(test_ota_confirm_firmware);
    RUN_TEST(test_ota_rollback);
    RUN_TEST(test_ota_not_initialized);

    // Power Manager
    RUN_TEST(test_power_init);
    RUN_TEST(test_power_light_sleep);
    RUN_TEST(test_power_light_sleep_invalid_duration);
    RUN_TEST(test_power_deep_sleep);
    RUN_TEST(test_power_adaptive_interval_mains);
    RUN_TEST(test_power_adaptive_interval_battery_low);
    RUN_TEST(test_power_adaptive_interval_battery_critical);
    RUN_TEST(test_power_read_battery);
    RUN_TEST(test_power_not_initialized);
}
