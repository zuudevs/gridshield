/**
 * @file test_byte_array.cpp
 * @brief Unit tests for ByteArray<N>
 */

#include "unity.h"
#include "core/types.hpp"
#include <cstring>

using namespace gridshield::core;

static void test_byte_array_initially_empty(void) {
    ByteArray<64> arr;
    TEST_ASSERT_EQUAL(0, arr.size());
    TEST_ASSERT_EQUAL(64, arr.capacity());
}

static void test_byte_array_append(void) {
    ByteArray<64> arr;
    const uint8_t data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    TEST_ASSERT_TRUE(arr.append(data, 4));
    TEST_ASSERT_EQUAL(4, arr.size());
    TEST_ASSERT_EQUAL(0xDE, arr[0]);
    TEST_ASSERT_EQUAL(0xAD, arr[1]);
    TEST_ASSERT_EQUAL(0xBE, arr[2]);
    TEST_ASSERT_EQUAL(0xEF, arr[3]);
}

static void test_byte_array_append_multiple(void) {
    ByteArray<64> arr;
    const uint8_t d1[] = {0x01, 0x02};
    const uint8_t d2[] = {0x03, 0x04, 0x05};
    TEST_ASSERT_TRUE(arr.append(d1, 2));
    TEST_ASSERT_TRUE(arr.append(d2, 3));
    TEST_ASSERT_EQUAL(5, arr.size());
    TEST_ASSERT_EQUAL(0x05, arr[4]);
}

static void test_byte_array_overflow(void) {
    ByteArray<4> arr;
    const uint8_t data[] = {0x01, 0x02, 0x03};
    TEST_ASSERT_TRUE(arr.append(data, 3));
    // Only 1 byte left, trying to append 2
    const uint8_t extra[] = {0x04, 0x05};
    TEST_ASSERT_FALSE(arr.append(extra, 2));
    TEST_ASSERT_EQUAL(3, arr.size()); // unchanged
}

static void test_byte_array_clear(void) {
    ByteArray<32> arr;
    const uint8_t data[] = {0xAA, 0xBB, 0xCC};
    arr.append(data, 3);
    arr.clear();
    TEST_ASSERT_EQUAL(0, arr.size());
}

static void test_byte_array_data_pointer(void) {
    ByteArray<16> arr;
    const uint8_t data[] = {0x42, 0x43};
    arr.append(data, 2);

    const uint8_t* ptr = arr.data();
    TEST_ASSERT_NOT_NULL(ptr);
    TEST_ASSERT_EQUAL(0x42, ptr[0]);
    TEST_ASSERT_EQUAL(0x43, ptr[1]);
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_byte_array_suite(void) {
    RUN_TEST(test_byte_array_initially_empty);
    RUN_TEST(test_byte_array_append);
    RUN_TEST(test_byte_array_append_multiple);
    RUN_TEST(test_byte_array_overflow);
    RUN_TEST(test_byte_array_clear);
    RUN_TEST(test_byte_array_data_pointer);
}
