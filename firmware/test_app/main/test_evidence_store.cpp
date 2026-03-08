/**
 * @file test_evidence_store.cpp
 * @brief Unit tests for v3.2.0 EvidenceStore module
 *
 * Tests evidence preservation, hash chain integrity, and circular overflow.
 */

#include "unity.h"

#include "forensics/evidence_store.hpp"

using namespace gridshield;
using namespace gridshield::forensics;

// ============================================================================
// EvidenceStore Tests
// ============================================================================

static SensorSnapshot make_sensor(uint32_t energy, uint32_t voltage, uint32_t current)
{
    SensorSnapshot s;
    s.energy_wh = energy;
    s.voltage_mv = voltage;
    s.current_ma = current;
    return s;
}

static void test_evidence_store_basic()
{
    EvidenceStore store;
    TEST_ASSERT_EQUAL(0, store.evidence_count());

    auto result = store.preserve(SecurityEventType::CasingOpened,
                                 SecurityEventSeverity::High,
                                 SourceLayer::Physical,
                                 1000,
                                 make_sensor(500, 220000, 2000),
                                 "Panel opened");
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(1, store.evidence_count());

    auto snap = store.get_evidence(0);
    TEST_ASSERT_TRUE(snap.is_ok());
    TEST_ASSERT_EQUAL(1000, snap.value().timestamp);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(SecurityEventType::CasingOpened),
                      static_cast<uint8_t>(snap.value().event_type));
    TEST_ASSERT_EQUAL(500, snap.value().sensors.energy_wh);
    TEST_ASSERT_EQUAL_STRING("Panel opened", snap.value().notes);
}

static void test_evidence_store_multiple()
{
    EvidenceStore store;
    store.preserve(SecurityEventType::CasingOpened,
                   SecurityEventSeverity::High,
                   SourceLayer::Physical,
                   1000,
                   make_sensor(500, 220000, 2000),
                   nullptr);
    store.preserve(SecurityEventType::AnomalyDetected,
                   SecurityEventSeverity::Medium,
                   SourceLayer::Analytics,
                   2000,
                   make_sensor(1500, 230000, 6000),
                   nullptr);
    store.preserve(SecurityEventType::SignatureVerifyFailed,
                   SecurityEventSeverity::Critical,
                   SourceLayer::Network,
                   3000,
                   make_sensor(600, 219000, 2500),
                   nullptr);

    TEST_ASSERT_EQUAL(3, store.evidence_count());

    auto latest = store.latest();
    TEST_ASSERT_TRUE(latest.is_ok());
    TEST_ASSERT_EQUAL(3000, latest.value().timestamp);
}

static void test_evidence_store_hash_populated()
{
    EvidenceStore store;
    store.preserve(SecurityEventType::CasingOpened,
                   SecurityEventSeverity::High,
                   SourceLayer::Physical,
                   1000,
                   make_sensor(500, 220000, 2000),
                   nullptr);

    auto snap = store.get_evidence(0);
    TEST_ASSERT_TRUE(snap.is_ok());

    // Hash should be non-zero
    bool all_zero = true;
    for (size_t i = 0; i < EVIDENCE_HASH_SIZE; ++i) {
        if (snap.value().hash[i] != 0) {
            all_zero = false;
            break;
        }
    }
    TEST_ASSERT_FALSE(all_zero);

    // First entry's prev_hash should be all zeros
    bool prev_zero = true;
    for (size_t i = 0; i < EVIDENCE_HASH_SIZE; ++i) {
        if (snap.value().prev_hash[i] != 0) {
            prev_zero = false;
            break;
        }
    }
    TEST_ASSERT_TRUE(prev_zero);
}

static void test_evidence_store_hash_chain_link()
{
    EvidenceStore store;
    store.preserve(SecurityEventType::CasingOpened,
                   SecurityEventSeverity::High,
                   SourceLayer::Physical,
                   1000,
                   make_sensor(500, 220000, 2000),
                   nullptr);
    store.preserve(SecurityEventType::AnomalyDetected,
                   SecurityEventSeverity::Medium,
                   SourceLayer::Analytics,
                   2000,
                   make_sensor(1500, 230000, 6000),
                   nullptr);

    auto first = store.get_evidence(0);
    auto second = store.get_evidence(1);
    TEST_ASSERT_TRUE(first.is_ok());
    TEST_ASSERT_TRUE(second.is_ok());

    // Second entry's prev_hash should equal first entry's hash
    TEST_ASSERT_EQUAL_UINT8_ARRAY(first.value().hash, second.value().prev_hash, EVIDENCE_HASH_SIZE);
}

static void test_evidence_store_verify_chain()
{
    EvidenceStore store;

    // Store 5 snapshots
    for (size_t i = 0; i < 5; ++i) {
        store.preserve(SecurityEventType::AnomalyDetected,
                       SecurityEventSeverity::Medium,
                       SourceLayer::Analytics,
                       static_cast<core::timestamp_t>(1000 + i * 1000),
                       make_sensor(static_cast<uint32_t>(500 + i * 100), 220000, 2000),
                       nullptr);
    }

    TEST_ASSERT_EQUAL(5, store.evidence_count());
    TEST_ASSERT_TRUE(store.verify_chain());
}

static void test_evidence_store_circular_overflow()
{
    EvidenceStore store;

    // Overflow: write capacity + 3
    for (size_t i = 0; i < EVIDENCE_STORE_CAPACITY + 3; ++i) {
        store.preserve(SecurityEventType::AnomalyDetected,
                       SecurityEventSeverity::Low,
                       SourceLayer::Analytics,
                       static_cast<core::timestamp_t>(1000 + i * 100),
                       make_sensor(static_cast<uint32_t>(500 + i), 220000, 2000),
                       nullptr);
    }

    TEST_ASSERT_EQUAL(EVIDENCE_STORE_CAPACITY, store.evidence_count());

    // Oldest should be the 4th entry (first 3 overwritten)
    auto oldest = store.get_evidence(0);
    TEST_ASSERT_TRUE(oldest.is_ok());
    TEST_ASSERT_EQUAL(1300, oldest.value().timestamp); // 1000 + 3*100

    // Latest should be the last written
    auto latest = store.latest();
    TEST_ASSERT_TRUE(latest.is_ok());
    // 1000 + (32+2)*100 = 1000 + 3400 = 4400
    TEST_ASSERT_EQUAL(4400, latest.value().timestamp);
}

static void test_evidence_store_latest_empty()
{
    EvidenceStore store;
    auto result = store.latest();
    TEST_ASSERT_TRUE(result.is_error());
}

static void test_evidence_store_sequence_counter()
{
    EvidenceStore store;
    store.preserve(SecurityEventType::CasingOpened,
                   SecurityEventSeverity::High,
                   SourceLayer::Physical,
                   1000,
                   make_sensor(500, 220000, 2000),
                   nullptr);
    store.preserve(SecurityEventType::AnomalyDetected,
                   SecurityEventSeverity::Medium,
                   SourceLayer::Analytics,
                   2000,
                   make_sensor(600, 221000, 2100),
                   nullptr);

    auto first = store.get_evidence(0);
    auto second = store.get_evidence(1);
    TEST_ASSERT_TRUE(first.is_ok());
    TEST_ASSERT_TRUE(second.is_ok());

    TEST_ASSERT_EQUAL(0, first.value().sequence);
    TEST_ASSERT_EQUAL(1, second.value().sequence);
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_evidence_store_suite(void)
{
    RUN_TEST(test_evidence_store_basic);
    RUN_TEST(test_evidence_store_multiple);
    RUN_TEST(test_evidence_store_hash_populated);
    RUN_TEST(test_evidence_store_hash_chain_link);
    RUN_TEST(test_evidence_store_verify_chain);
    RUN_TEST(test_evidence_store_circular_overflow);
    RUN_TEST(test_evidence_store_latest_empty);
    RUN_TEST(test_evidence_store_sequence_counter);
}
