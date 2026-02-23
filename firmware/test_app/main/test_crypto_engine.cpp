/**
 * @file test_crypto_engine.cpp
 * @brief Unit tests for CryptoEngine (ECC + AES-GCM + SHA-256)
 */

#include "unity.h"
#include "security/crypto.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::security;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

static MockCrypto g_mock_crypto;

static CryptoEngine& get_engine() {
    static CryptoEngine engine(g_mock_crypto);
    return engine;
}

// ============================================================================
// Keypair Generation
// ============================================================================

static void test_crypto_generate_keypair(void) {
    auto& engine = get_engine();
    ECCKeyPair kp;

    auto result = engine.generate_keypair(kp);
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(kp.has_private_key());
    TEST_ASSERT_TRUE(kp.has_public_key());
}

// ============================================================================
// ECDSA Sign + Verify
// ============================================================================

static void test_crypto_sign_verify(void) {
    auto& engine = get_engine();
    ECCKeyPair kp;
    engine.generate_keypair(kp);

    const uint8_t message[] = "GridShield test message";
    uint8_t signature[ECC_SIGNATURE_SIZE];

    auto sign_res = engine.sign(kp, message, sizeof(message), signature);
    TEST_ASSERT_TRUE(sign_res.is_ok());

    auto verify_res = engine.verify(kp, message, sizeof(message), signature);
    TEST_ASSERT_TRUE(verify_res.is_ok());
    TEST_ASSERT_TRUE(verify_res.value());
}

static void test_crypto_verify_wrong_message(void) {
    auto& engine = get_engine();
    ECCKeyPair kp;
    engine.generate_keypair(kp);

    const uint8_t message[] = "Original message";
    const uint8_t wrong[] = "Wrong message!!!";
    uint8_t signature[ECC_SIGNATURE_SIZE];

    engine.sign(kp, message, sizeof(message), signature);

    auto verify_res = engine.verify(kp, wrong, sizeof(wrong), signature);
    TEST_ASSERT_TRUE(verify_res.is_ok());
    TEST_ASSERT_FALSE(verify_res.value());
}

static void test_crypto_sign_null_params(void) {
    auto& engine = get_engine();
    ECCKeyPair kp;
    engine.generate_keypair(kp);

    uint8_t signature[ECC_SIGNATURE_SIZE];

    auto result = engine.sign(kp, nullptr, 10, signature);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// SHA-256
// ============================================================================

static void test_crypto_hash_sha256(void) {
    auto& engine = get_engine();

    const uint8_t data[] = "Hello GridShield";
    uint8_t hash[SHA256_HASH_SIZE];

    auto result = engine.hash_sha256(data, sizeof(data), hash);
    TEST_ASSERT_TRUE(result.is_ok());

    // Hash should be non-zero
    bool all_zero = true;
    for (size_t i = 0; i < SHA256_HASH_SIZE; ++i) {
        if (hash[i] != 0) { all_zero = false; break; }
    }
    TEST_ASSERT_FALSE(all_zero);
}

static void test_crypto_hash_null_params(void) {
    auto& engine = get_engine();
    uint8_t hash[SHA256_HASH_SIZE];

    auto result = engine.hash_sha256(nullptr, 10, hash);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Random Bytes
// ============================================================================

static void test_crypto_random_bytes(void) {
    auto& engine = get_engine();

    uint8_t buf[32];
    memset(buf, 0, sizeof(buf));

    auto result = engine.random_bytes(buf, sizeof(buf));
    TEST_ASSERT_TRUE(result.is_ok());

    // Should have generated something non-zero
    bool all_zero = true;
    for (size_t i = 0; i < sizeof(buf); ++i) {
        if (buf[i] != 0) { all_zero = false; break; }
    }
    TEST_ASSERT_FALSE(all_zero);
}

static void test_crypto_random_null_buffer(void) {
    auto& engine = get_engine();

    auto result = engine.random_bytes(nullptr, 32);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_crypto_engine_suite(void) {
    RUN_TEST(test_crypto_generate_keypair);
    RUN_TEST(test_crypto_sign_verify);
    RUN_TEST(test_crypto_verify_wrong_message);
    RUN_TEST(test_crypto_sign_null_params);
    RUN_TEST(test_crypto_hash_sha256);
    RUN_TEST(test_crypto_hash_null_params);
    RUN_TEST(test_crypto_random_bytes);
    RUN_TEST(test_crypto_random_null_buffer);
}
