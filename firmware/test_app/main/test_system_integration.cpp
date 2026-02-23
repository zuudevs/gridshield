/**
 * @file test_system_integration.cpp
 * @brief Integration tests — GridShieldSystem full lifecycle
 */

#include "unity.h"
#include "core/system.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::platform;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

struct SystemFixture {
    MockTime time;
    MockGPIO gpio;
    MockInterrupt interrupt;
    MockCrypto crypto;
    MockComm comm;
    MockStorage storage;
    PlatformServices services;
    GridShieldSystem system;

    SystemFixture() noexcept {
        services.time = &time;
        services.gpio = &gpio;
        services.interrupt = &interrupt;
        services.crypto = &crypto;
        services.storage = &storage;
        services.comm = &comm;
        comm.set_connected(true);
    }

    SystemConfig make_config() {
        SystemConfig config;
        config.meter_id = 0xDEADBEEF;
        config.heartbeat_interval_ms = 60000;
        config.reading_interval_ms = 5000;
        config.tamper_config.sensor_pin = 4;
        config.tamper_config.debounce_ms = 50;

        for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
            config.baseline_profile.hourly_avg_wh[i] = 1200;
        }
        config.baseline_profile.daily_avg_wh = 1200;
        config.baseline_profile.variance_threshold = 30;

        return config;
    }
};

// ============================================================================
// Full Lifecycle
// ============================================================================

static void test_integration_full_lifecycle(void) {
    SystemFixture f;
    auto config = f.make_config();

    // Initialize
    auto init_res = f.system.initialize(config, f.services);
    TEST_ASSERT_TRUE_MESSAGE(init_res.is_ok(), "initialize failed");

    // Start
    auto start_res = f.system.start();
    TEST_ASSERT_TRUE_MESSAGE(start_res.is_ok(), "start failed");

    // Run 5 cycles
    for (int i = 0; i < 5; ++i) {
        auto cycle_res = f.system.process_cycle();
        TEST_ASSERT_TRUE_MESSAGE(cycle_res.is_ok(), "process_cycle failed");
    }

    // Stop
    auto stop_res = f.system.stop();
    TEST_ASSERT_TRUE_MESSAGE(stop_res.is_ok(), "stop failed");

    // Shutdown
    auto shutdown_res = f.system.shutdown();
    TEST_ASSERT_TRUE_MESSAGE(shutdown_res.is_ok(), "shutdown failed");
}

// ============================================================================
// Double Init Should Fail
// ============================================================================

static void test_integration_double_init(void) {
    SystemFixture f;
    auto config = f.make_config();

    f.system.initialize(config, f.services);
    auto result = f.system.initialize(config, f.services);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Start Without Init
// ============================================================================

static void test_integration_start_without_init(void) {
    SystemFixture f;
    auto result = f.system.start();
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Process Cycle Before Start
// ============================================================================

static void test_integration_cycle_before_start(void) {
    SystemFixture f;
    auto config = f.make_config();
    f.system.initialize(config, f.services);

    auto result = f.system.process_cycle();
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Reinitialize After Shutdown
// ============================================================================

static void test_integration_reinit_after_shutdown(void) {
    SystemFixture f;
    auto config = f.make_config();

    // First lifecycle
    f.system.initialize(config, f.services);
    f.system.start();
    f.system.process_cycle();
    f.system.shutdown();

    // Second lifecycle — need new system instance since shutdown clears state
    GridShieldSystem system2;
    auto init_res = system2.initialize(config, f.services);
    TEST_ASSERT_TRUE(init_res.is_ok());

    auto start_res = system2.start();
    TEST_ASSERT_TRUE(start_res.is_ok());

    auto cycle_res = system2.process_cycle();
    TEST_ASSERT_TRUE(cycle_res.is_ok());

    system2.shutdown();
}

// ============================================================================
// Invalid Platform Services
// ============================================================================

static void test_integration_invalid_platform(void) {
    SystemFixture f;
    auto config = f.make_config();

    PlatformServices empty;
    auto result = f.system.initialize(config, empty);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_system_integration_suite(void) {
    RUN_TEST(test_integration_full_lifecycle);
    RUN_TEST(test_integration_double_init);
    RUN_TEST(test_integration_start_without_init);
    RUN_TEST(test_integration_cycle_before_start);
    RUN_TEST(test_integration_reinit_after_shutdown);
    RUN_TEST(test_integration_invalid_platform);
}
