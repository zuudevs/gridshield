/**
 * @file test_analytics.cpp
 * @brief Unit tests for v3.0.0 advanced analytics
 *
 * Tests TFLite runner, time series buffer, and ML anomaly detector.
 */

#include "unity.h"

#include "analytics/ml_anomaly.hpp"
#include "analytics/tflite_runner.hpp"
#include "analytics/time_series.hpp"


using namespace gridshield;
using namespace gridshield::analytics;

// ============================================================================
// TFLite Runner Tests
// ============================================================================

static void test_tflite_load_model()
{
    TfliteRunner runner;
    static constexpr uint8_t FAKE_MODEL[] = {0x18, 0x00, 0x00, 0x00};
    auto result = runner.load_model(FAKE_MODEL, sizeof(FAKE_MODEL));
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(runner.is_loaded());
}

static void test_tflite_invoke_success()
{
    TfliteRunner runner;
    static constexpr uint8_t MODEL[] = {0x01};
    runner.load_model(MODEL, sizeof(MODEL));

    static constexpr int32_t MOCK_OUTPUT[] = {750};
    runner.set_mock_output(MOCK_OUTPUT, 1);

    static constexpr int32_t INPUT[] = {100, 200, 300, 400, 500, 600};
    static constexpr size_t INPUT_SIZE = 6;
    runner.set_input(INPUT, INPUT_SIZE);

    auto result = runner.invoke();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(result.value().valid);
    static constexpr int32_t EXPECTED_SCORE = 750;
    TEST_ASSERT_EQUAL(EXPECTED_SCORE, result.value().output[0]);
    TEST_ASSERT_EQUAL(1, runner.invoke_count());
}

static void test_tflite_invoke_failure()
{
    TfliteRunner runner;
    static constexpr uint8_t MODEL[] = {0x01};
    runner.load_model(MODEL, sizeof(MODEL));
    runner.set_simulate_failure(true);

    static constexpr int32_t INPUT[] = {100};
    runner.set_input(INPUT, 1);

    auto result = runner.invoke();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::InferenceFailed, result.error().code);
}

static void test_tflite_unload()
{
    TfliteRunner runner;
    static constexpr uint8_t MODEL[] = {0x01};
    runner.load_model(MODEL, sizeof(MODEL));
    TEST_ASSERT_TRUE(runner.is_loaded());

    runner.unload();
    TEST_ASSERT_FALSE(runner.is_loaded());
}

// ============================================================================
// Time Series Tests
// ============================================================================

static void test_ts_init_and_push()
{
    TimeSeriesBuffer<16> ts;
    auto init_result = ts.init();
    TEST_ASSERT_TRUE(init_result.is_ok());
    TEST_ASSERT_EQUAL(0, ts.count());

    static constexpr int32_t VAL1 = 100;
    static constexpr int32_t VAL2 = 200;
    ts.push(VAL1, 1000);
    ts.push(VAL2, 2000);
    TEST_ASSERT_EQUAL(2, ts.count());

    auto latest = ts.latest();
    TEST_ASSERT_TRUE(latest.is_ok());
    TEST_ASSERT_EQUAL(VAL2, latest.value().value);
}

static void test_ts_moving_average()
{
    TimeSeriesBuffer<16> ts;
    ts.init();

    static constexpr int32_t VAL_100 = 100;
    static constexpr int32_t VAL_200 = 200;
    static constexpr int32_t VAL_300 = 300;
    ts.push(VAL_100, 1000);
    ts.push(VAL_200, 2000);
    ts.push(VAL_300, 3000);

    auto ma = ts.moving_average(3);
    TEST_ASSERT_TRUE(ma.is_ok());
    TEST_ASSERT_EQUAL(VAL_200, ma.value()); // (100+200+300)/3 = 200
}

static void test_ts_exponential_smooth()
{
    TimeSeriesBuffer<16> ts;
    ts.init();

    ts.push(100, 1000);
    ts.push(200, 2000);

    auto result = ts.exponential_smooth();
    TEST_ASSERT_TRUE(result.is_ok());
    // With alpha=0.2: EMA = 0.2*200 + 0.8*100 = 120
    static constexpr int32_t EXPECTED_EMA = 120;
    TEST_ASSERT_EQUAL(EXPECTED_EMA, result.value());
}

static void test_ts_forecast()
{
    TimeSeriesBuffer<16> ts;
    ts.init();

    // Linear trend: 100, 200, 300
    ts.push(100, 1000);
    ts.push(200, 2000);
    ts.push(300, 3000);

    static constexpr uint32_t HORIZON = 1000; // 1000s ahead = 1 interval
    auto forecast = ts.forecast(HORIZON);
    TEST_ASSERT_TRUE(forecast.is_ok());
    TEST_ASSERT_TRUE(forecast.value().valid);
    TEST_ASSERT_EQUAL(1, forecast.value().trend);             // upward trend
    TEST_ASSERT_TRUE(forecast.value().predicted_value > 300); // should predict > 300
}

static void test_ts_stats()
{
    TimeSeriesBuffer<16> ts;
    ts.init();

    ts.push(50, 1000);
    ts.push(100, 2000);
    ts.push(150, 3000);

    auto st = ts.stats();
    TEST_ASSERT_TRUE(st.is_ok());
    TEST_ASSERT_EQUAL(3, st.value().sample_count);
    static constexpr int32_t EXPECTED_MIN = 50;
    static constexpr int32_t EXPECTED_MAX = 150;
    static constexpr int32_t EXPECTED_MEAN = 100;
    TEST_ASSERT_EQUAL(EXPECTED_MIN, st.value().min_value);
    TEST_ASSERT_EQUAL(EXPECTED_MAX, st.value().max_value);
    TEST_ASSERT_EQUAL(EXPECTED_MEAN, st.value().mean_value);
}

// ============================================================================
// ML Anomaly Tests
// ============================================================================

static void test_ml_init()
{
    TfliteRunner runner;
    MlAnomalyDetector detector;
    auto result = detector.init(&runner);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(detector.is_initialized());
    TEST_ASSERT_EQUAL(ML_DEFAULT_THRESHOLD_X1000, detector.threshold());
}

static void test_ml_score_normal()
{
    TfliteRunner runner;
    static constexpr uint8_t MODEL[] = {0x01};
    runner.load_model(MODEL, sizeof(MODEL));

    // Set a low anomaly score (below threshold)
    static constexpr int32_t LOW_SCORE[] = {300};
    runner.set_mock_output(LOW_SCORE, 1);

    MlAnomalyDetector detector;
    detector.init(&runner);

    SensorSnapshot snap{};
    static constexpr uint32_t SNAP_VOLTAGE = 230000;
    static constexpr uint32_t SNAP_CURRENT = 5000;
    snap.voltage_mv = SNAP_VOLTAGE;
    snap.current_ma = SNAP_CURRENT;

    auto result = detector.score(snap);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_FALSE(result.value().is_anomaly);
    TEST_ASSERT_EQUAL(300, result.value().score);
}

static void test_ml_score_anomaly()
{
    TfliteRunner runner;
    static constexpr uint8_t MODEL[] = {0x01};
    runner.load_model(MODEL, sizeof(MODEL));

    // Set high anomaly score (above default threshold of 700)
    static constexpr int32_t HIGH_SCORE[] = {900};
    runner.set_mock_output(HIGH_SCORE, 1);

    MlAnomalyDetector detector;
    detector.init(&runner);

    SensorSnapshot snap{};
    static constexpr uint32_t ANOMALY_VOLTAGE = 400000;
    snap.voltage_mv = ANOMALY_VOLTAGE; // abnormally high

    auto result = detector.score(snap);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(result.value().is_anomaly);
    TEST_ASSERT_EQUAL(900, result.value().score);
}

static void test_ml_feature_extraction()
{
    TfliteRunner runner;
    MlAnomalyDetector detector;
    detector.init(&runner);

    SensorSnapshot snap{};
    static constexpr uint32_t FE_VOLTAGE = 240000;
    static constexpr uint32_t FE_CURRENT = 10000;
    snap.voltage_mv = FE_VOLTAGE;
    snap.current_ma = FE_CURRENT;

    auto fv = detector.extract_features(snap);
    TEST_ASSERT_TRUE(fv.valid);
    // Voltage normalized: 240000 * 1000 / 240 = 1000000
    TEST_ASSERT_TRUE(fv.features[0] > 0);
    // Current normalized: 10000 * 1000 / 100 = 100000
    TEST_ASSERT_TRUE(fv.features[1] > 0);
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_analytics_suite(void)
{
    // TFLite
    RUN_TEST(test_tflite_load_model);
    RUN_TEST(test_tflite_invoke_success);
    RUN_TEST(test_tflite_invoke_failure);
    RUN_TEST(test_tflite_unload);

    // Time Series
    RUN_TEST(test_ts_init_and_push);
    RUN_TEST(test_ts_moving_average);
    RUN_TEST(test_ts_exponential_smooth);
    RUN_TEST(test_ts_forecast);
    RUN_TEST(test_ts_stats);

    // ML Anomaly
    RUN_TEST(test_ml_init);
    RUN_TEST(test_ml_score_normal);
    RUN_TEST(test_ml_score_anomaly);
    RUN_TEST(test_ml_feature_extraction);
}
