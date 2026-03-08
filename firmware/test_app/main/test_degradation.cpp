/**
 * @file test_degradation.cpp
 * @brief Unit tests for DegradationManager
 */

#include "core/degradation.hpp"
#include "unity.h"

using namespace gridshield::core;

// ============================================================================
// Default Policy
// ============================================================================

static void test_deg_default_healthy(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    mgr.set_policy(policy);

    TEST_ASSERT_TRUE(mgr.can_continue());
    TEST_ASSERT_FALSE(mgr.is_degraded());
}

// ============================================================================
// Non-critical failure
// ============================================================================

static void test_deg_network_failure_continues(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    policy.allow_without_network = true;
    mgr.set_policy(policy);

    bool can = mgr.report_failure(ServiceId::Network, ErrorCode::NetworkTimeout);
    TEST_ASSERT_TRUE(can);
    TEST_ASSERT_TRUE(mgr.is_degraded());
    TEST_ASSERT_FALSE(mgr.is_service_available(ServiceId::Network));
}

// ============================================================================
// Critical failure — crypto
// ============================================================================

static void test_deg_crypto_failure_halts(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    policy.allow_without_crypto = false;
    mgr.set_policy(policy);

    bool can = mgr.report_failure(ServiceId::Crypto, ErrorCode::CryptoFailure);
    TEST_ASSERT_FALSE(can);
}

// ============================================================================
// Recovery
// ============================================================================

static void test_deg_recovery(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    mgr.set_policy(policy);

    mgr.report_failure(ServiceId::Network, ErrorCode::NetworkTimeout);
    TEST_ASSERT_FALSE(mgr.is_service_available(ServiceId::Network));

    mgr.report_recovery(ServiceId::Network);
    TEST_ASSERT_TRUE(mgr.is_service_available(ServiceId::Network));
    TEST_ASSERT_EQUAL(0, mgr.get_failure_count(ServiceId::Network));
}

// ============================================================================
// Degraded state
// ============================================================================

static void test_deg_degraded_service(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    mgr.set_policy(policy);

    mgr.report_degraded(ServiceId::Analytics);
    TEST_ASSERT_TRUE(mgr.is_service_available(ServiceId::Analytics));
    TEST_ASSERT_TRUE(mgr.is_degraded());
    TEST_ASSERT_EQUAL(ServiceHealth::Degraded, mgr.get_health(ServiceId::Analytics));
}

// ============================================================================
// Failure count
// ============================================================================

static void test_deg_failure_count(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    mgr.set_policy(policy);

    mgr.report_failure(ServiceId::Network, ErrorCode::NetworkTimeout);
    mgr.report_failure(ServiceId::Network, ErrorCode::NetworkTimeout);
    mgr.report_failure(ServiceId::Network, ErrorCode::TransmissionFailed);
    TEST_ASSERT_EQUAL(3, mgr.get_failure_count(ServiceId::Network));
}

// ============================================================================
// Multiple service failures
// ============================================================================

static void test_deg_multiple_failures(void)
{
    DegradationManager mgr;
    DegradationPolicy policy;
    policy.allow_without_tamper = true;
    policy.allow_without_analytics = true;
    mgr.set_policy(policy);

    mgr.report_failure(ServiceId::Tamper, ErrorCode::HardwareFailure);
    mgr.report_failure(ServiceId::Analytics, ErrorCode::DataInvalid);

    TEST_ASSERT_TRUE(mgr.can_continue());
    TEST_ASSERT_FALSE(mgr.is_service_available(ServiceId::Tamper));
    TEST_ASSERT_FALSE(mgr.is_service_available(ServiceId::Analytics));
    TEST_ASSERT_TRUE(mgr.is_service_available(ServiceId::Crypto));
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_degradation_suite(void)
{
    RUN_TEST(test_deg_default_healthy);
    RUN_TEST(test_deg_network_failure_continues);
    RUN_TEST(test_deg_crypto_failure_halts);
    RUN_TEST(test_deg_recovery);
    RUN_TEST(test_deg_degraded_service);
    RUN_TEST(test_deg_failure_count);
    RUN_TEST(test_deg_multiple_failures);
}
