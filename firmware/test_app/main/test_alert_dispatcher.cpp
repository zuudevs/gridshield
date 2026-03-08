/**
 * @file test_alert_dispatcher.cpp
 * @brief Unit tests for v3.3.0 AlertDispatcher module
 *
 * Tests rule-based security event dispatch, severity thresholds,
 * multiple rule matching, and edge cases.
 */

#include "unity.h"

#include "forensics/alert_dispatcher.hpp"

using namespace gridshield;
using namespace gridshield::forensics;

// ============================================================================
// Helper: create a SecurityEvent
// ============================================================================
static SecurityEvent
make_event(SecurityEventType type, SecurityEventSeverity severity, core::timestamp_t ts = 1000)
{
    SecurityEvent evt;
    evt.event_type = type;
    evt.severity = severity;
    evt.timestamp = ts;
    evt.source_layer = SourceLayer::System;
    return evt;
}

// ============================================================================
// AlertDispatcher Tests
// ============================================================================

static void test_alert_dispatcher_empty()
{
    AlertDispatcher dispatcher;
    TEST_ASSERT_EQUAL(0, dispatcher.rule_count());

    auto event = make_event(SecurityEventType::CasingOpened, SecurityEventSeverity::High);
    auto result = dispatcher.dispatch(event);
    TEST_ASSERT_FALSE(result.matched);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AlertAction::None), static_cast<uint8_t>(result.action));
}

static void test_alert_dispatcher_add_rule()
{
    AlertDispatcher dispatcher;
    auto res = dispatcher.add_rule(
        SecurityEventType::CasingOpened, SecurityEventSeverity::Medium, AlertAction::HttpPost);
    TEST_ASSERT_TRUE(res.is_ok());
    TEST_ASSERT_EQUAL(1, dispatcher.rule_count());

    auto rule = dispatcher.get_rule(0);
    TEST_ASSERT_TRUE(rule.is_ok());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(SecurityEventType::CasingOpened),
                      static_cast<uint8_t>(rule.value().event_type));
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AlertAction::HttpPost),
                      static_cast<uint8_t>(rule.value().action));
}

static void test_alert_dispatcher_basic_match()
{
    AlertDispatcher dispatcher;
    dispatcher.add_rule(
        SecurityEventType::CasingOpened, SecurityEventSeverity::Medium, AlertAction::HttpPost);

    // Event matches: same type, severity >= minimum
    auto event = make_event(SecurityEventType::CasingOpened, SecurityEventSeverity::High);
    auto result = dispatcher.dispatch(event);
    TEST_ASSERT_TRUE(result.matched);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AlertAction::HttpPost),
                      static_cast<uint8_t>(result.action));
}

static void test_alert_dispatcher_severity_threshold()
{
    AlertDispatcher dispatcher;
    dispatcher.add_rule(
        SecurityEventType::AnomalyDetected, SecurityEventSeverity::High, AlertAction::MqttPublish);

    // Below threshold — should NOT match
    auto low_event = make_event(SecurityEventType::AnomalyDetected, SecurityEventSeverity::Medium);
    auto result_low = dispatcher.dispatch(low_event);
    TEST_ASSERT_FALSE(result_low.matched);

    // At threshold — should match
    auto high_event = make_event(SecurityEventType::AnomalyDetected, SecurityEventSeverity::High);
    auto result_high = dispatcher.dispatch(high_event);
    TEST_ASSERT_TRUE(result_high.matched);

    // Above threshold — should match
    auto crit_event =
        make_event(SecurityEventType::AnomalyDetected, SecurityEventSeverity::Critical);
    auto result_crit = dispatcher.dispatch(crit_event);
    TEST_ASSERT_TRUE(result_crit.matched);
}

static void test_alert_dispatcher_no_type_match()
{
    AlertDispatcher dispatcher;
    dispatcher.add_rule(
        SecurityEventType::CasingOpened, SecurityEventSeverity::Info, AlertAction::LogOnly);

    // Different event type — should not match
    auto event = make_event(SecurityEventType::AnomalyDetected, SecurityEventSeverity::Critical);
    TEST_ASSERT_FALSE(dispatcher.matches(event));
}

static void test_alert_dispatcher_multiple_rules_highest_action()
{
    AlertDispatcher dispatcher;

    // Two rules for the same event type with different actions
    dispatcher.add_rule(SecurityEventType::SignatureVerifyFailed,
                        SecurityEventSeverity::Medium,
                        AlertAction::LogOnly);
    dispatcher.add_rule(
        SecurityEventType::SignatureVerifyFailed, SecurityEventSeverity::High, AlertAction::All);

    // Critical event matches both — should return highest action (All)
    auto event =
        make_event(SecurityEventType::SignatureVerifyFailed, SecurityEventSeverity::Critical);
    auto result = dispatcher.dispatch(event);
    TEST_ASSERT_TRUE(result.matched);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(AlertAction::All), static_cast<uint8_t>(result.action));
}

static void test_alert_dispatcher_remove_rule()
{
    AlertDispatcher dispatcher;
    dispatcher.add_rule(
        SecurityEventType::CasingOpened, SecurityEventSeverity::Info, AlertAction::LogOnly);
    dispatcher.add_rule(
        SecurityEventType::AnomalyDetected, SecurityEventSeverity::Medium, AlertAction::HttpPost);
    TEST_ASSERT_EQUAL(2, dispatcher.rule_count());

    auto res = dispatcher.remove_rule(0);
    TEST_ASSERT_TRUE(res.is_ok());
    TEST_ASSERT_EQUAL(1, dispatcher.rule_count());

    // Remaining rule should be the anomaly one
    auto rule = dispatcher.get_rule(0);
    TEST_ASSERT_TRUE(rule.is_ok());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(SecurityEventType::AnomalyDetected),
                      static_cast<uint8_t>(rule.value().event_type));
}

static void test_alert_dispatcher_clear_rules()
{
    AlertDispatcher dispatcher;
    for (int i = 0; i < 5; ++i) {
        dispatcher.add_rule(
            SecurityEventType::CasingOpened, SecurityEventSeverity::Info, AlertAction::LogOnly);
    }
    TEST_ASSERT_EQUAL(5, dispatcher.rule_count());

    dispatcher.clear_rules();
    TEST_ASSERT_EQUAL(0, dispatcher.rule_count());

    auto event = make_event(SecurityEventType::CasingOpened, SecurityEventSeverity::Critical);
    TEST_ASSERT_FALSE(dispatcher.matches(event));
}

static void test_alert_dispatcher_full_buffer()
{
    AlertDispatcher dispatcher;

    // Fill to capacity
    for (size_t i = 0; i < MAX_ALERT_RULES; ++i) {
        auto res = dispatcher.add_rule(
            SecurityEventType::CasingOpened, SecurityEventSeverity::Info, AlertAction::LogOnly);
        TEST_ASSERT_TRUE(res.is_ok());
    }
    TEST_ASSERT_EQUAL(MAX_ALERT_RULES, dispatcher.rule_count());

    // One more should fail
    auto res = dispatcher.add_rule(
        SecurityEventType::AnomalyDetected, SecurityEventSeverity::Info, AlertAction::HttpPost);
    TEST_ASSERT_TRUE(res.is_error());
}

static void test_alert_dispatcher_matches_helper()
{
    AlertDispatcher dispatcher;
    dispatcher.add_rule(
        SecurityEventType::PowerCutAttempt, SecurityEventSeverity::Low, AlertAction::MqttPublish);

    auto match_event =
        make_event(SecurityEventType::PowerCutAttempt, SecurityEventSeverity::Medium);
    TEST_ASSERT_TRUE(dispatcher.matches(match_event));

    auto no_match = make_event(SecurityEventType::PhysicalShock, SecurityEventSeverity::Critical);
    TEST_ASSERT_FALSE(dispatcher.matches(no_match));
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_alert_dispatcher_suite(void)
{
    RUN_TEST(test_alert_dispatcher_empty);
    RUN_TEST(test_alert_dispatcher_add_rule);
    RUN_TEST(test_alert_dispatcher_basic_match);
    RUN_TEST(test_alert_dispatcher_severity_threshold);
    RUN_TEST(test_alert_dispatcher_no_type_match);
    RUN_TEST(test_alert_dispatcher_multiple_rules_highest_action);
    RUN_TEST(test_alert_dispatcher_remove_rule);
    RUN_TEST(test_alert_dispatcher_clear_rules);
    RUN_TEST(test_alert_dispatcher_full_buffer);
    RUN_TEST(test_alert_dispatcher_matches_helper);
}
