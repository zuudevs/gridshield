/**
 * @file test_retry.cpp
 * @brief Unit tests for RetryExecutor
 */

#include "unity.h"
#include "network/retry.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::network;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

static MockTime g_time;

static int g_call_count;
static int g_succeed_on_attempt;

static core::Result<void> mock_operation() {
    ++g_call_count;
    if (g_call_count >= g_succeed_on_attempt) {
        return core::Result<void>();
    }
    return GS_MAKE_ERROR(core::ErrorCode::TransmissionFailed);
}

static void reset_mock(int succeed_on = 1) {
    g_call_count = 0;
    g_succeed_on_attempt = succeed_on;
}

// ============================================================================
// Succeed on First Try
// ============================================================================

static void test_retry_succeed_first(void) {
    reset_mock(1);
    RetryPolicy policy(3, 100, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute(mock_operation);
    TEST_ASSERT_TRUE(result.succeeded);
    TEST_ASSERT_EQUAL(1, result.attempts);
    TEST_ASSERT_EQUAL(core::ErrorCode::Success, result.last_error);
}

// ============================================================================
// Succeed on Retry
// ============================================================================

static void test_retry_succeed_on_second(void) {
    reset_mock(2);
    RetryPolicy policy(3, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute(mock_operation);
    TEST_ASSERT_TRUE(result.succeeded);
    TEST_ASSERT_EQUAL(2, result.attempts);
}

static void test_retry_succeed_on_last(void) {
    reset_mock(4); // Succeed on attempt 4 (max_retries=3, so attempt 0,1,2,3)
    RetryPolicy policy(3, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute(mock_operation);
    TEST_ASSERT_TRUE(result.succeeded);
    TEST_ASSERT_EQUAL(4, result.attempts);
}

// ============================================================================
// Exhaust All Retries
// ============================================================================

static void test_retry_exhaust_all(void) {
    reset_mock(999); // Never succeed
    RetryPolicy policy(3, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute(mock_operation);
    TEST_ASSERT_FALSE(result.succeeded);
    TEST_ASSERT_EQUAL(4, result.attempts); // 1 initial + 3 retries
    TEST_ASSERT_EQUAL(core::ErrorCode::TransmissionFailed, result.last_error);
}

// ============================================================================
// Zero Retries
// ============================================================================

static void test_retry_zero_retries(void) {
    reset_mock(999);
    RetryPolicy policy(0, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute(mock_operation);
    TEST_ASSERT_FALSE(result.succeeded);
    TEST_ASSERT_EQUAL(1, result.attempts);
}

// ============================================================================
// execute_result() wrapper
// ============================================================================

static void test_retry_execute_result_ok(void) {
    reset_mock(1);
    RetryPolicy policy(3, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute_result(mock_operation);
    TEST_ASSERT_TRUE(result.is_ok());
}

static void test_retry_execute_result_error(void) {
    reset_mock(999);
    RetryPolicy policy(2, 10, 5000, 2);
    RetryExecutor retry(policy, g_time);

    auto result = retry.execute_result(mock_operation);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Default Policy
// ============================================================================

static void test_retry_default_policy(void) {
    RetryPolicy policy;
    TEST_ASSERT_EQUAL(3, policy.max_retries);
    TEST_ASSERT_EQUAL(100, policy.base_delay_ms);
    TEST_ASSERT_EQUAL(5000, policy.max_delay_ms);
    TEST_ASSERT_EQUAL(2, policy.backoff_factor);
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_retry_suite(void) {
    RUN_TEST(test_retry_succeed_first);
    RUN_TEST(test_retry_succeed_on_second);
    RUN_TEST(test_retry_succeed_on_last);
    RUN_TEST(test_retry_exhaust_all);
    RUN_TEST(test_retry_zero_retries);
    RUN_TEST(test_retry_execute_result_ok);
    RUN_TEST(test_retry_execute_result_error);
    RUN_TEST(test_retry_default_policy);
}
