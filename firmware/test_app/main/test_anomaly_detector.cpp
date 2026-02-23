/**
 * @file test_anomaly_detector.cpp
 * @brief Unit tests for AnomalyDetector
 */

#include "unity.h"
#include "analytics/detector.hpp"

using namespace gridshield;
using namespace gridshield::analytics;
using namespace gridshield::core;

// Helper: create a baseline profile
static ConsumptionProfile make_baseline(uint32_t avg_wh = 1200, uint16_t threshold = 30) {
    ConsumptionProfile profile;
    for (size_t i = 0; i < PROFILE_HISTORY_SIZE; ++i) {
        profile.hourly_avg_wh[i] = avg_wh;
    }
    profile.daily_avg_wh = avg_wh;
    profile.weekly_avg_wh = avg_wh;
    profile.variance_threshold = threshold;
    profile.profile_confidence = 80;
    return profile;
}

// Helper: create a meter reading
static MeterReading make_reading(uint32_t energy_wh, uint64_t timestamp = 1000) {
    MeterReading r;
    r.energy_wh = energy_wh;
    r.voltage_mv = 220000;
    r.current_ma = 500;
    r.power_factor = 950;
    r.timestamp = timestamp;
    return r;
}

// ============================================================================
// Initialization
// ============================================================================

static void test_detector_initialize(void) {
    AnomalyDetector detector;
    auto profile = make_baseline();
    auto result = detector.initialize(profile);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_detector_get_profile(void) {
    AnomalyDetector detector;
    auto profile = make_baseline(1500);
    detector.initialize(profile);
    const auto& p = detector.get_profile();
    TEST_ASSERT_EQUAL(1500, p.daily_avg_wh);
}

// ============================================================================
// Normal Reading Analysis
// ============================================================================

static void test_detector_normal_reading(void) {
    AnomalyDetector detector;
    detector.initialize(make_baseline(1200, 30));

    // Reading within 30% of baseline (1200 ± 360)
    auto result = detector.analyze(make_reading(1100));
    TEST_ASSERT_TRUE(result.is_ok());
    auto report = result.value();
    TEST_ASSERT_EQUAL(AnomalyType::None, report.type);
    TEST_ASSERT_EQUAL(AnomalySeverity::None, report.severity);
}

// ============================================================================
// Anomalous Spike
// ============================================================================

static void test_detector_spike_detection(void) {
    AnomalyDetector detector;
    detector.initialize(make_baseline(1200, 30));

    // Reading is 2400 wh — 100% deviation from 1200 baseline
    auto result = detector.analyze(make_reading(2400));
    TEST_ASSERT_TRUE(result.is_ok());
    auto report = result.value();
    TEST_ASSERT_NOT_EQUAL(AnomalyType::None, report.type);
    TEST_ASSERT_NOT_EQUAL(AnomalySeverity::None, report.severity);
    TEST_ASSERT_GREATER_THAN(30, report.deviation_percent);
}

// ============================================================================
// Zero Consumption
// ============================================================================

static void test_detector_zero_consumption(void) {
    AnomalyDetector detector;
    detector.initialize(make_baseline(1200, 30));

    auto result = detector.analyze(make_reading(0));
    TEST_ASSERT_TRUE(result.is_ok());
    auto report = result.value();
    // Zero consumption should be flagged
    TEST_ASSERT_NOT_EQUAL(AnomalyType::None, report.type);
}

// ============================================================================
// Profile Reset
// ============================================================================

static void test_detector_reset_profile(void) {
    AnomalyDetector detector;
    detector.initialize(make_baseline(1200));
    auto result = detector.reset_profile();
    TEST_ASSERT_TRUE(result.is_ok());
}

// ============================================================================
// Cross-Layer Validation
// ============================================================================

static void test_cross_layer_no_investigation(void) {
    CrossLayerValidation v;
    TEST_ASSERT_FALSE(v.requires_investigation());
    TEST_ASSERT_EQUAL(Priority::Normal, v.get_priority());
}

static void test_cross_layer_physical_and_consumption(void) {
    CrossLayerValidation v;
    v.physical_tamper_detected = true;
    v.consumption_anomaly_detected = true;
    TEST_ASSERT_TRUE(v.requires_investigation());
    TEST_ASSERT_EQUAL(Priority::Critical, v.get_priority());
}

static void test_cross_layer_all_flags(void) {
    CrossLayerValidation v;
    v.physical_tamper_detected = true;
    v.network_anomaly_detected = true;
    v.consumption_anomaly_detected = true;
    TEST_ASSERT_TRUE(v.requires_investigation());
    TEST_ASSERT_EQUAL(Priority::Emergency, v.get_priority());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_anomaly_detector_suite(void) {
    RUN_TEST(test_detector_initialize);
    RUN_TEST(test_detector_get_profile);
    RUN_TEST(test_detector_normal_reading);
    RUN_TEST(test_detector_spike_detection);
    RUN_TEST(test_detector_zero_consumption);
    RUN_TEST(test_detector_reset_profile);
    RUN_TEST(test_cross_layer_no_investigation);
    RUN_TEST(test_cross_layer_physical_and_consumption);
    RUN_TEST(test_cross_layer_all_flags);
}
