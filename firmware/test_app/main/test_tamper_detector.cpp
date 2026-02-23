/**
 * @file test_tamper_detector.cpp
 * @brief Unit tests for TamperDetector
 */

#include "unity.h"
#include "hardware/tamper.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::hardware;
using namespace gridshield::platform;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

struct TamperTestFixture {
    MockTime time;
    MockGPIO gpio;
    MockInterrupt interrupt;
    MockCrypto crypto;
    MockComm comm;
    MockStorage storage;
    PlatformServices services;
    TamperDetector detector;

    TamperTestFixture() noexcept {
        services.time = &time;
        services.gpio = &gpio;
        services.interrupt = &interrupt;
        services.crypto = &crypto;
        services.storage = &storage;
        services.comm = &comm;
    }
};

static TamperConfig make_config(uint8_t pin = 4, uint16_t debounce = 50) {
    TamperConfig cfg;
    cfg.sensor_pin = pin;
    cfg.debounce_ms = debounce;
    cfg.sensitivity = 128;
    return cfg;
}

// ============================================================================
// Initialization
// ============================================================================

static void test_tamper_initialize(void) {
    TamperTestFixture f;
    auto result = f.detector.initialize(make_config(), f.services);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_tamper_double_init(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);
    auto result = f.detector.initialize(make_config(), f.services);
    // Second init should fail
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Start / Stop
// ============================================================================

static void test_tamper_start_stop(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);

    auto start_res = f.detector.start();
    TEST_ASSERT_TRUE(start_res.is_ok());

    auto stop_res = f.detector.stop();
    TEST_ASSERT_TRUE(stop_res.is_ok());
}

static void test_tamper_start_without_init(void) {
    TamperTestFixture f;
    auto result = f.detector.start();
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Tamper State
// ============================================================================

static void test_tamper_initial_state(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);
    TEST_ASSERT_FALSE(f.detector.is_tampered());
    TEST_ASSERT_EQUAL(TamperType::None, f.detector.get_tamper_type());
    TEST_ASSERT_EQUAL(0, f.detector.get_tamper_timestamp());
}

// ============================================================================
// Poll (no ISR trigger)
// ============================================================================

static void test_tamper_poll_no_trigger(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);
    f.detector.start();

    auto result = f.detector.poll();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_FALSE(f.detector.is_tampered());
}

// ============================================================================
// Reset
// ============================================================================

static void test_tamper_reset(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);
    f.detector.start();

    auto result = f.detector.reset();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_FALSE(f.detector.is_tampered());
}

// ============================================================================
// Acknowledge
// ============================================================================

static void test_tamper_acknowledge(void) {
    TamperTestFixture f;
    f.detector.initialize(make_config(), f.services);
    f.detector.start();

    auto result = f.detector.acknowledge_tamper();
    TEST_ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_tamper_detector_suite(void) {
    RUN_TEST(test_tamper_initialize);
    RUN_TEST(test_tamper_double_init);
    RUN_TEST(test_tamper_start_stop);
    RUN_TEST(test_tamper_start_without_init);
    RUN_TEST(test_tamper_initial_state);
    RUN_TEST(test_tamper_poll_no_trigger);
    RUN_TEST(test_tamper_reset);
    RUN_TEST(test_tamper_acknowledge);
}
