/**
 * @file test_static_buffer.cpp
 * @brief Unit tests for StaticBuffer<T, N>
 */

#include "core/types.hpp"
#include "unity.h"

using namespace gridshield::core;

// ============================================================================
// Basic Operations
// ============================================================================

static void test_buffer_initially_empty(void)
{
    StaticBuffer<int, 8> buf;
    TEST_ASSERT_TRUE(buf.empty());
    TEST_ASSERT_EQUAL(0, buf.size());
    TEST_ASSERT_EQUAL(8, buf.capacity());
    TEST_ASSERT_FALSE(buf.full());
}

static void test_buffer_push_and_size(void)
{
    StaticBuffer<int, 4> buf;
    TEST_ASSERT_TRUE(buf.push(10));
    TEST_ASSERT_TRUE(buf.push(20));
    TEST_ASSERT_TRUE(buf.push(30));
    TEST_ASSERT_EQUAL(3, buf.size());
    TEST_ASSERT_EQUAL(10, buf[0]);
    TEST_ASSERT_EQUAL(20, buf[1]);
    TEST_ASSERT_EQUAL(30, buf[2]);
}

static void test_buffer_push_overflow(void)
{
    StaticBuffer<int, 2> buf;
    TEST_ASSERT_TRUE(buf.push(1));
    TEST_ASSERT_TRUE(buf.push(2));
    TEST_ASSERT_TRUE(buf.full());
    TEST_ASSERT_FALSE(buf.push(3)); // overflow
    TEST_ASSERT_EQUAL(2, buf.size());
}

static void test_buffer_pop_lifo(void)
{
    StaticBuffer<int, 4> buf;
    buf.push(10);
    buf.push(20);
    buf.push(30);

    int val = 0;
    TEST_ASSERT_TRUE(buf.pop(val));
    TEST_ASSERT_EQUAL(30, val); // LIFO
    TEST_ASSERT_EQUAL(2, buf.size());
}

static void test_buffer_pop_empty(void)
{
    StaticBuffer<int, 4> buf;
    int val = 0;
    TEST_ASSERT_FALSE(buf.pop(val));
}

static void test_buffer_pop_front_fifo(void)
{
    StaticBuffer<int, 4> buf;
    buf.push(10);
    buf.push(20);
    buf.push(30);

    int val = 0;
    TEST_ASSERT_TRUE(buf.pop_front(val));
    TEST_ASSERT_EQUAL(10, val); // FIFO — oldest first
    TEST_ASSERT_EQUAL(2, buf.size());
    TEST_ASSERT_EQUAL(20, buf[0]); // shifted
    TEST_ASSERT_EQUAL(30, buf[1]);
}

static void test_buffer_pop_front_empty(void)
{
    StaticBuffer<int, 4> buf;
    int val = 0;
    TEST_ASSERT_FALSE(buf.pop_front(val));
}

static void test_buffer_clear(void)
{
    StaticBuffer<int, 4> buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);
    buf.clear();
    TEST_ASSERT_TRUE(buf.empty());
    TEST_ASSERT_EQUAL(0, buf.size());
}

// ============================================================================
// Struct Type (MeterReading)
// ============================================================================

static void test_buffer_with_meter_reading(void)
{
    StaticBuffer<MeterReading, 4> buf;

    MeterReading r;
    r.energy_wh = 1200;
    r.voltage_mv = 220000;
    r.current_ma = 500;

    TEST_ASSERT_TRUE(buf.push(r));
    TEST_ASSERT_EQUAL(1, buf.size());
    TEST_ASSERT_EQUAL(1200, buf[0].energy_wh);
    TEST_ASSERT_EQUAL(220000, buf[0].voltage_mv);
    TEST_ASSERT_EQUAL(500, buf[0].current_ma);
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_static_buffer_suite(void)
{
    RUN_TEST(test_buffer_initially_empty);
    RUN_TEST(test_buffer_push_and_size);
    RUN_TEST(test_buffer_push_overflow);
    RUN_TEST(test_buffer_pop_lifo);
    RUN_TEST(test_buffer_pop_empty);
    RUN_TEST(test_buffer_pop_front_fifo);
    RUN_TEST(test_buffer_pop_front_empty);
    RUN_TEST(test_buffer_clear);
    RUN_TEST(test_buffer_with_meter_reading);
}
