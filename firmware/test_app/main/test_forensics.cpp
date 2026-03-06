/**
 * @file test_forensics.cpp
 * @brief Unit tests for v3.1.0 forensics module
 *
 * Tests EventLogger, IncidentReportGenerator, and cross-layer correlation.
 */

#include "unity.h"

#include "forensics/event_logger.hpp"
#include "forensics/incident_report.hpp"

using namespace gridshield;
using namespace gridshield::forensics;

// ============================================================================
// EventLogger Tests
// ============================================================================

static void test_event_log_basic()
{
    EventLogger logger;
    TEST_ASSERT_EQUAL(0, logger.event_count());

    auto result = logger.log_event(SecurityEventType::CasingOpened,
                                   SecurityEventSeverity::High,
                                   SourceLayer::Physical,
                                   1000,
                                   "Front panel removed");
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(1, logger.event_count());

    auto evt = logger.get_event(0);
    TEST_ASSERT_TRUE(evt.is_ok());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(SecurityEventType::CasingOpened),
                      static_cast<uint8_t>(evt.value().event_type));
    TEST_ASSERT_EQUAL(1000, evt.value().timestamp);
    TEST_ASSERT_EQUAL_STRING("Front panel removed", evt.value().details);
}

static void test_event_log_multiple()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     nullptr);
    logger.log_event(SecurityEventType::SignatureVerifyFailed,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     3000,
                     nullptr);
    TEST_ASSERT_EQUAL(3, logger.event_count());
}

static void test_event_log_latest()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     nullptr);

    auto latest = logger.latest();
    TEST_ASSERT_TRUE(latest.is_ok());
    TEST_ASSERT_EQUAL(2000, latest.value().timestamp);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(SecurityEventType::AnomalyDetected),
                      static_cast<uint8_t>(latest.value().event_type));
}

static void test_event_log_empty_latest()
{
    EventLogger logger;
    auto latest = logger.latest();
    TEST_ASSERT_TRUE(latest.is_error());
}

static void test_event_log_timeline()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     nullptr);
    logger.log_event(SecurityEventType::SignatureVerifyFailed,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     3000,
                     nullptr);
    logger.log_event(SecurityEventType::SystemReboot,
                     SecurityEventSeverity::Info,
                     SourceLayer::System,
                     5000,
                     nullptr);

    // Get events in [1500, 3500] range
    SecurityEvent out[4];
    size_t found = logger.get_timeline(1500, 3500, out, 4);
    TEST_ASSERT_EQUAL(2, found);
    TEST_ASSERT_EQUAL(2000, out[0].timestamp);
    TEST_ASSERT_EQUAL(3000, out[1].timestamp);
}

static void test_event_log_count_by_type()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     2000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     3000,
                     nullptr);

    TEST_ASSERT_EQUAL(2, logger.count_by_type(SecurityEventType::CasingOpened));
    TEST_ASSERT_EQUAL(1, logger.count_by_type(SecurityEventType::AnomalyDetected));
    TEST_ASSERT_EQUAL(0, logger.count_by_type(SecurityEventType::SystemReboot));
}

static void test_event_log_count_by_severity()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     nullptr);
    logger.log_event(SecurityEventType::SignatureVerifyFailed,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     3000,
                     nullptr);
    logger.log_event(SecurityEventType::SystemReboot,
                     SecurityEventSeverity::Info,
                     SourceLayer::System,
                     4000,
                     nullptr);

    // >= High
    TEST_ASSERT_EQUAL(2, logger.count_by_severity(SecurityEventSeverity::High));
    // >= Critical
    TEST_ASSERT_EQUAL(1, logger.count_by_severity(SecurityEventSeverity::Critical));
    // >= Info (all)
    TEST_ASSERT_EQUAL(4, logger.count_by_severity(SecurityEventSeverity::Info));
}

static void test_event_log_clear()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     nullptr);
    TEST_ASSERT_EQUAL(2, logger.event_count());

    logger.clear();
    TEST_ASSERT_EQUAL(0, logger.event_count());
    TEST_ASSERT_TRUE(logger.latest().is_error());
}

static void test_event_log_circular_overflow()
{
    EventLogger logger;

    // Fill the buffer completely (capacity = 64)
    for (size_t i = 0; i < EVENT_LOG_CAPACITY + 5; ++i) {
        logger.log_event(SecurityEventType::AnomalyDetected,
                         SecurityEventSeverity::Low,
                         SourceLayer::Analytics,
                         static_cast<core::timestamp_t>(1000 + i * 100),
                         nullptr);
    }

    // Should be capped at capacity
    TEST_ASSERT_EQUAL(EVENT_LOG_CAPACITY, logger.event_count());

    // Oldest event should be the 6th event (first 5 were overwritten)
    auto oldest = logger.get_event(0);
    TEST_ASSERT_TRUE(oldest.is_ok());
    TEST_ASSERT_EQUAL(1500, oldest.value().timestamp); // 1000 + 5*100

    // Latest should be the last event
    auto latest = logger.latest();
    TEST_ASSERT_TRUE(latest.is_ok());
    // 1000 + (64+4) * 100 = 1000 + 6800 = 7800
    TEST_ASSERT_EQUAL(7800, latest.value().timestamp);
}

static void test_event_log_invalid_index()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);

    auto result = logger.get_event(5);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// IncidentReportGenerator Tests
// ============================================================================

static void test_report_single_layer_physical()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::PhysicalShock,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Physical,
                     1500,
                     nullptr);

    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_ok());

    const auto& report = result.value();
    TEST_ASSERT_TRUE(report.valid);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AttackType::PhysicalTampering),
                      static_cast<uint8_t>(report.attack_type));
    TEST_ASSERT_TRUE(report.physical_layer_affected);
    TEST_ASSERT_FALSE(report.network_layer_affected);
    TEST_ASSERT_EQUAL(2, report.total_events);
}

static void test_report_hybrid_attack()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::SignatureVerifyFailed,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     1200,
                     nullptr);
    logger.log_event(SecurityEventType::ConsumptionDrop,
                     SecurityEventSeverity::High,
                     SourceLayer::Analytics,
                     1400,
                     nullptr);

    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_ok());

    const auto& report = result.value();
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AttackType::HybridAttack),
                      static_cast<uint8_t>(report.attack_type));
    TEST_ASSERT_TRUE(report.physical_layer_affected);
    TEST_ASSERT_TRUE(report.network_layer_affected);
    TEST_ASSERT_TRUE(report.analytics_layer_affected);
    TEST_ASSERT_TRUE(report.confidence >= 50); // multi-layer + critical + temporal proximity
}

static void test_report_confidence_scoring()
{
    EventLogger logger;
    // Create a high-confidence scenario: multi-layer, critical, close temporal proximity
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Physical,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::ReplayAttackDetected,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     1100,
                     nullptr);
    logger.log_event(SecurityEventType::ZeroConsumption,
                     SecurityEventSeverity::High,
                     SourceLayer::Analytics,
                     1200,
                     nullptr);

    for (int i = 0; i < 8; ++i) {
        logger.log_event(SecurityEventType::AnomalyDetected,
                         SecurityEventSeverity::Medium,
                         SourceLayer::Analytics,
                         static_cast<core::timestamp_t>(1300 + i * 100),
                         nullptr);
    }

    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_ok());

    const auto& report = result.value();
    // Multi-layer (+30), critical (+25), 11 events (+15), close in time (+15) = 85
    TEST_ASSERT_TRUE(report.confidence >= 70);
}

static void test_report_empty_logger()
{
    EventLogger logger;
    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_error());
}

static void test_report_network_intrusion()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::SignatureVerifyFailed,
                     SecurityEventSeverity::High,
                     SourceLayer::Network,
                     1000,
                     nullptr);
    logger.log_event(SecurityEventType::UnauthorizedDevice,
                     SecurityEventSeverity::Critical,
                     SourceLayer::Network,
                     1100,
                     nullptr);

    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_ok());

    const auto& report = result.value();
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AttackType::NetworkIntrusion),
                      static_cast<uint8_t>(report.attack_type));
    TEST_ASSERT_FALSE(report.physical_layer_affected);
    TEST_ASSERT_TRUE(report.network_layer_affected);
}

static void test_report_event_snapshot()
{
    EventLogger logger;
    logger.log_event(SecurityEventType::CasingOpened,
                     SecurityEventSeverity::High,
                     SourceLayer::Physical,
                     1000,
                     "door open");
    logger.log_event(SecurityEventType::AnomalyDetected,
                     SecurityEventSeverity::Medium,
                     SourceLayer::Analytics,
                     2000,
                     "energy spike");

    IncidentReportGenerator gen;
    auto result = gen.generate_report(logger);
    TEST_ASSERT_TRUE(result.is_ok());

    const auto& report = result.value();
    TEST_ASSERT_EQUAL(2, report.event_count);
    TEST_ASSERT_EQUAL_STRING("door open", report.events[0].details);
    TEST_ASSERT_EQUAL_STRING("energy spike", report.events[1].details);
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_forensics_suite(void)
{
    // EventLogger tests
    RUN_TEST(test_event_log_basic);
    RUN_TEST(test_event_log_multiple);
    RUN_TEST(test_event_log_latest);
    RUN_TEST(test_event_log_empty_latest);
    RUN_TEST(test_event_log_timeline);
    RUN_TEST(test_event_log_count_by_type);
    RUN_TEST(test_event_log_count_by_severity);
    RUN_TEST(test_event_log_clear);
    RUN_TEST(test_event_log_circular_overflow);
    RUN_TEST(test_event_log_invalid_index);

    // IncidentReportGenerator tests
    RUN_TEST(test_report_single_layer_physical);
    RUN_TEST(test_report_hybrid_attack);
    RUN_TEST(test_report_confidence_scoring);
    RUN_TEST(test_report_empty_logger);
    RUN_TEST(test_report_network_intrusion);
    RUN_TEST(test_report_event_snapshot);
}
