/**
 * @file test_result.cpp
 * @brief Unit tests for Result<T> and Result<void> monad
 */

#include "core/error.hpp"
#include "unity.h"

using namespace gridshield::core;

// ============================================================================
// Result<void> Tests
// ============================================================================

static void test_result_void_default_is_ok(void)
{
    Result<void> r;
    TEST_ASSERT_TRUE(r.is_ok());
    TEST_ASSERT_FALSE(r.is_error());
}

static void test_result_void_error_construction(void)
{
    Result<void> r(GS_MAKE_ERROR(ErrorCode::SystemNotInitialized));
    TEST_ASSERT_FALSE(r.is_ok());
    TEST_ASSERT_TRUE(r.is_error());
    TEST_ASSERT_EQUAL(ErrorCode::SystemNotInitialized, r.error().code);
}

static void test_result_void_error_has_file_line(void)
{
    Result<void> r(GS_MAKE_ERROR(ErrorCode::CryptoFailure));
    TEST_ASSERT_NOT_NULL(r.error().file);
    TEST_ASSERT_GREATER_THAN(0, r.error().line);
}

// ============================================================================
// Result<T> Tests
// ============================================================================

static void test_result_value_construction(void)
{
    Result<int> r(42);
    TEST_ASSERT_TRUE(r.is_ok());
    TEST_ASSERT_EQUAL(42, r.value());
}

static void test_result_value_error(void)
{
    Result<int> r(GS_MAKE_ERROR(ErrorCode::InvalidParameter));
    TEST_ASSERT_TRUE(r.is_error());
    TEST_ASSERT_EQUAL(ErrorCode::InvalidParameter, r.error().code);
}

static void test_result_value_or_success(void)
{
    Result<int> r(99);
    TEST_ASSERT_EQUAL(99, r.value_or(0));
}

static void test_result_value_or_error(void)
{
    Result<int> r(GS_MAKE_ERROR(ErrorCode::Unknown));
    TEST_ASSERT_EQUAL(-1, r.value_or(-1));
}

static void test_result_move_construction(void)
{
    Result<int> r1(123);
    Result<int> r2(static_cast<Result<int>&&>(r1));
    TEST_ASSERT_TRUE(r2.is_ok());
    TEST_ASSERT_EQUAL(123, r2.value());
}

static void test_result_as_void_success(void)
{
    Result<int> r(42);
    Result<void> rv = r.as_void();
    TEST_ASSERT_TRUE(rv.is_ok());
}

static void test_result_as_void_error(void)
{
    Result<int> r(GS_MAKE_ERROR(ErrorCode::BufferOverflow));
    Result<void> rv = r.as_void();
    TEST_ASSERT_TRUE(rv.is_error());
    TEST_ASSERT_EQUAL(ErrorCode::BufferOverflow, rv.error().code);
}

// ============================================================================
// ErrorContext Tests
// ============================================================================

static void test_error_context_is_critical(void)
{
    ErrorContext hw_err(ErrorCode::HardwareFailure);
    TEST_ASSERT_TRUE(hw_err.is_critical());

    ErrorContext sec_err(ErrorCode::CryptoFailure);
    TEST_ASSERT_TRUE(sec_err.is_critical());

    ErrorContext sys_err(ErrorCode::SystemNotInitialized);
    TEST_ASSERT_FALSE(sys_err.is_critical());

    ErrorContext net_err(ErrorCode::NetworkTimeout);
    TEST_ASSERT_FALSE(net_err.is_critical());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_result_suite(void)
{
    RUN_TEST(test_result_void_default_is_ok);
    RUN_TEST(test_result_void_error_construction);
    RUN_TEST(test_result_void_error_has_file_line);
    RUN_TEST(test_result_value_construction);
    RUN_TEST(test_result_value_error);
    RUN_TEST(test_result_value_or_success);
    RUN_TEST(test_result_value_or_error);
    RUN_TEST(test_result_move_construction);
    RUN_TEST(test_result_as_void_success);
    RUN_TEST(test_result_as_void_error);
    RUN_TEST(test_error_context_is_critical);
}
