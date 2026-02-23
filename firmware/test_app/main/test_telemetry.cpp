/**
 * @file test_telemetry.cpp
 * @brief Unit tests for SystemTelemetry
 */

#include "unity.h"
#include "core/telemetry.hpp"

using namespace gridshield::core;

// ============================================================================
// Initial State
// ============================================================================

static void test_telem_initial_zero(void) {
    SystemTelemetry telem;
    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(0, c.packets_sent);
    TEST_ASSERT_EQUAL(0, c.total_errors);
    TEST_ASSERT_EQUAL(0, c.cycle_count);
    TEST_ASSERT_EQUAL(0, c.boot_count);
}

// ============================================================================
// Network counters
// ============================================================================

static void test_telem_network(void) {
    SystemTelemetry telem;
    telem.record_packet_sent(128);
    telem.record_packet_sent(64);
    telem.record_packet_received(256);
    telem.record_packet_failed();
    telem.record_network_retry();

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(2, c.packets_sent);
    TEST_ASSERT_EQUAL(192, c.bytes_sent);
    TEST_ASSERT_EQUAL(1, c.packets_received);
    TEST_ASSERT_EQUAL(256, c.bytes_received);
    TEST_ASSERT_EQUAL(1, c.packets_failed);
    TEST_ASSERT_EQUAL(1, c.network_retries);
}

// ============================================================================
// Security counters
// ============================================================================

static void test_telem_security(void) {
    SystemTelemetry telem;
    telem.record_crypto_op();
    telem.record_crypto_op();
    telem.record_key_rotation();
    telem.record_tamper_event();
    telem.record_tamper_acknowledged();

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(2, c.crypto_operations);
    TEST_ASSERT_EQUAL(1, c.key_rotations);
    TEST_ASSERT_EQUAL(1, c.tamper_events);
    TEST_ASSERT_EQUAL(1, c.tamper_acknowledged);
}

// ============================================================================
// Error counters
// ============================================================================

static void test_telem_errors(void) {
    SystemTelemetry telem;
    telem.record_error(false);
    telem.record_error(true);
    telem.record_error(false);

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(3, c.total_errors);
    TEST_ASSERT_EQUAL(1, c.critical_errors);
}

// ============================================================================
// Cycle counters
// ============================================================================

static void test_telem_cycles(void) {
    SystemTelemetry telem;
    telem.record_cycle(false);
    telem.record_cycle(false);
    telem.record_cycle(true);

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(3, c.cycle_count);
    TEST_ASSERT_EQUAL(1, c.cycle_overruns);
}

// ============================================================================
// Boot + Uptime
// ============================================================================

static void test_telem_uptime(void) {
    SystemTelemetry telem;
    telem.record_boot(1000);
    telem.update_uptime(6000);

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(1, c.boot_count);
    TEST_ASSERT_EQUAL(5000, c.uptime_ms);
}

// ============================================================================
// Reset
// ============================================================================

static void test_telem_reset(void) {
    SystemTelemetry telem;
    telem.record_packet_sent(100);
    telem.record_error(true);
    telem.reset();

    auto& c = telem.counters();
    TEST_ASSERT_EQUAL(0, c.packets_sent);
    TEST_ASSERT_EQUAL(0, c.total_errors);
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_telemetry_suite(void) {
    RUN_TEST(test_telem_initial_zero);
    RUN_TEST(test_telem_network);
    RUN_TEST(test_telem_security);
    RUN_TEST(test_telem_errors);
    RUN_TEST(test_telem_cycles);
    RUN_TEST(test_telem_uptime);
    RUN_TEST(test_telem_reset);
}
