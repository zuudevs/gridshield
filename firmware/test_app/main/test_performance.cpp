/**
 * @file test_performance.cpp
 * @brief Unit tests for v3.0.0 performance optimization
 *
 * Tests memory pool, zero-copy buffer, and hw crypto.
 */

#include "unity.h"

#include "security/hw_crypto.hpp"
#include "utils/memory_pool.hpp"
#include "utils/zero_copy_buffer.hpp"


using namespace gridshield;
using namespace gridshield::utils;
using namespace gridshield::security;

// ============================================================================
// Memory Pool Tests
// ============================================================================

static constexpr size_t TEST_BLOCK_SIZE = 64;
static constexpr size_t TEST_NUM_BLOCKS = 4;

static void test_pool_init()
{
    MemoryPool<TEST_BLOCK_SIZE, TEST_NUM_BLOCKS> pool;
    auto result = pool.init();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(TEST_NUM_BLOCKS, pool.available());
    TEST_ASSERT_EQUAL(0, pool.used());
}

static void test_pool_alloc_dealloc()
{
    MemoryPool<TEST_BLOCK_SIZE, TEST_NUM_BLOCKS> pool;
    pool.init();

    auto alloc_result = pool.allocate();
    TEST_ASSERT_TRUE(alloc_result.is_ok());
    TEST_ASSERT_NOT_NULL(alloc_result.value());
    TEST_ASSERT_EQUAL(1, pool.used());
    TEST_ASSERT_EQUAL(TEST_NUM_BLOCKS - 1, pool.available());

    auto dealloc_result = pool.deallocate(alloc_result.value());
    TEST_ASSERT_TRUE(dealloc_result.is_ok());
    TEST_ASSERT_EQUAL(0, pool.used());
}

static void test_pool_exhaustion()
{
    MemoryPool<TEST_BLOCK_SIZE, TEST_NUM_BLOCKS> pool;
    pool.init();

    // Allocate all blocks
    for (size_t block_idx = 0; block_idx < TEST_NUM_BLOCKS; ++block_idx) {
        auto result = pool.allocate();
        TEST_ASSERT_TRUE(result.is_ok());
    }
    TEST_ASSERT_EQUAL(0, pool.available());

    // Next allocation should fail
    auto result = pool.allocate();
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::PoolExhausted, result.error().code);
}

static void test_pool_double_free()
{
    MemoryPool<TEST_BLOCK_SIZE, TEST_NUM_BLOCKS> pool;
    pool.init();

    auto alloc = pool.allocate();
    pool.deallocate(alloc.value());

    // Double free should be detected
    auto result = pool.deallocate(alloc.value());
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::PoolDoubleFree, result.error().code);
}

// ============================================================================
// Zero-Copy Buffer Tests
// ============================================================================

static constexpr size_t TEST_BUF_CAP = 32;

static void test_zcbuf_init()
{
    ZeroCopyBuffer<TEST_BUF_CAP> buf;
    auto result = buf.init();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(buf.empty());
    TEST_ASSERT_EQUAL(TEST_BUF_CAP, buf.available());
}

static void test_zcbuf_write_read()
{
    ZeroCopyBuffer<TEST_BUF_CAP> buf;
    buf.init();

    static constexpr uint8_t DATA[] = {0x01, 0x02, 0x03, 0x04};
    auto wr = buf.write(DATA, sizeof(DATA));
    TEST_ASSERT_TRUE(wr.is_ok());
    TEST_ASSERT_EQUAL(sizeof(DATA), wr.value());
    TEST_ASSERT_EQUAL(sizeof(DATA), buf.used());

    std::array<uint8_t, 8> out{};
    auto rd = buf.read(out.data(), out.size());
    TEST_ASSERT_TRUE(rd.is_ok());
    TEST_ASSERT_EQUAL(sizeof(DATA), rd.value());
    TEST_ASSERT_EQUAL_UINT8(0x01, out[0]);
    TEST_ASSERT_EQUAL_UINT8(0x04, out[3]);
    TEST_ASSERT_TRUE(buf.empty());
}

static void test_zcbuf_overflow()
{
    ZeroCopyBuffer<4> buf;
    buf.init();

    static constexpr size_t OVERFLOW_SIZE = 5;
    static constexpr uint8_t BIG_DATA[OVERFLOW_SIZE] = {1, 2, 3, 4, 5};
    auto result = buf.write(BIG_DATA, OVERFLOW_SIZE);
    // Should write only 4 bytes (capacity)
    TEST_ASSERT_TRUE(result.is_ok());
    static constexpr size_t EXPECTED_WRITTEN = 4;
    TEST_ASSERT_EQUAL(EXPECTED_WRITTEN, result.value());
    TEST_ASSERT_TRUE(buf.full());
}

static void test_zcbuf_span_api()
{
    ZeroCopyBuffer<TEST_BUF_CAP> buf;
    buf.init();

    // Write using span API
    static constexpr size_t SPAN_SIZE = 4;
    auto ws = buf.write_span(SPAN_SIZE);
    TEST_ASSERT_TRUE(ws.is_ok());
    TEST_ASSERT_EQUAL(SPAN_SIZE, ws.value().size());
    ws.value()[0] = 0xAA;
    ws.value()[1] = 0xBB;
    buf.commit_write(2);

    // Read using span API
    auto rs = buf.read_span(2);
    TEST_ASSERT_TRUE(rs.is_ok());
    TEST_ASSERT_EQUAL_UINT8(0xAA, rs.value()[0]);
    TEST_ASSERT_EQUAL_UINT8(0xBB, rs.value()[1]);
}

// ============================================================================
// HW Crypto Tests
// ============================================================================

static void test_hwcrypto_init()
{
    HwCrypto crypto;
    auto result = crypto.init();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_FALSE(crypto.has_hw_acceleration());
}

static void test_hwcrypto_aes_roundtrip()
{
    HwCrypto crypto;
    crypto.init();

    static constexpr uint8_t KEY[HW_AES_KEY_SIZE] = {0x42};
    static constexpr uint8_t IV[HW_AES_GCM_IV_SIZE] = {0x01, 0x02, 0x03};
    static constexpr uint8_t PLAINTEXT[] = {0xDE, 0xAD, 0xBE, 0xEF};
    static constexpr size_t PT_LEN = sizeof(PLAINTEXT);

    auto enc = crypto.aes_gcm_encrypt(KEY, IV, PLAINTEXT, PT_LEN, nullptr, 0);
    TEST_ASSERT_TRUE(enc.is_ok());
    TEST_ASSERT_EQUAL(PT_LEN, enc.value().length);

    auto dec = crypto.aes_gcm_decrypt(
        KEY, IV, enc.value().data.data(), enc.value().length, enc.value().tag.data(), nullptr, 0);
    TEST_ASSERT_TRUE(dec.is_ok());
    TEST_ASSERT_EQUAL(PT_LEN, dec.value().length);
    // XOR roundtrip should restore original
    TEST_ASSERT_EQUAL_UINT8(PLAINTEXT[0], dec.value().data[0]);
    TEST_ASSERT_EQUAL_UINT8(PLAINTEXT[3], dec.value().data[3]);
}

static void test_hwcrypto_sha256()
{
    HwCrypto crypto;
    crypto.init();

    static constexpr uint8_t DATA[] = {0x01, 0x02, 0x03};
    auto result = crypto.sha256(DATA, sizeof(DATA));
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(1, crypto.hash_count());
}

static void test_hwcrypto_hmac()
{
    HwCrypto crypto;
    crypto.init();

    static constexpr uint8_t KEY[] = {0xAB, 0xCD};
    static constexpr uint8_t DATA[] = {0x01, 0x02};
    auto result = crypto.hmac_sha256(KEY, sizeof(KEY), DATA, sizeof(DATA));
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_EQUAL(1, crypto.hmac_count());
}

// ============================================================================
// TEST SUITE ENTRY POINT
// ============================================================================

extern "C" void test_performance_suite(void)
{
    // Memory Pool
    RUN_TEST(test_pool_init);
    RUN_TEST(test_pool_alloc_dealloc);
    RUN_TEST(test_pool_exhaustion);
    RUN_TEST(test_pool_double_free);

    // Zero-Copy Buffer
    RUN_TEST(test_zcbuf_init);
    RUN_TEST(test_zcbuf_write_read);
    RUN_TEST(test_zcbuf_overflow);
    RUN_TEST(test_zcbuf_span_api);

    // HW Crypto
    RUN_TEST(test_hwcrypto_init);
    RUN_TEST(test_hwcrypto_aes_roundtrip);
    RUN_TEST(test_hwcrypto_sha256);
    RUN_TEST(test_hwcrypto_hmac);
}
