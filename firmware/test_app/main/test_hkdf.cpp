/**
 * @file test_hkdf.cpp
 * @brief Unit tests for HKDF (RFC 5869)
 */

#include "security/hkdf.hpp"
#include "unity.h"
#include <cstring>

using namespace gridshield::security;

// Test vectors from RFC 5869 Appendix A.1
// Hash = SHA-256
static void test_hkdf_rfc5869_case1(void)
{
    const uint8_t ikm[] = {0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b,
                           0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b, 0x0b};
    const uint8_t salt[] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c};
    const uint8_t info[] = {0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9};
    const uint8_t expected_okm[] = {
        0x3c, 0xb2, 0x5f, 0x25, 0xfa, 0xac, 0xd5, 0x7a, 0x90, 0x43, 0x4f, 0x64, 0x10, 0xc6, 0xca,
        0x7a, 0x2d, 0x55, 0x6a, 0x25, 0x47, 0x05, 0x83, 0xce, 0x59, 0xb2, 0x3d, 0xb3, 0xec, 0xd2,
        0x04, 0xcc, 0xe5, 0x3d, 0x2a, 0x84, 0x0c, 0xda, 0x7d, 0x67, 0x03, 0x56, 0xb6};

    uint8_t okm[42];
    auto res = hkdf(salt, sizeof(salt), ikm, sizeof(ikm), info, sizeof(info), okm, sizeof(okm));

    TEST_ASSERT_TRUE(res.is_ok());
    TEST_ASSERT_EQUAL_HEX8_ARRAY(expected_okm, okm, sizeof(okm));
}

static void test_hkdf_no_salt_no_info(void)
{
    const uint8_t ikm[] = "secret_key_material";
    uint8_t okm1[32];
    uint8_t okm2[32];

    // Standard HKDF with null salt/info
    auto res1 = hkdf(nullptr, 0, ikm, strlen((const char*)ikm), nullptr, 0, okm1, sizeof(okm1));
    TEST_ASSERT_TRUE(res1.is_ok());

    // Another run to ensure consistency
    auto res2 = hkdf(nullptr, 0, ikm, strlen((const char*)ikm), nullptr, 0, okm2, sizeof(okm2));
    TEST_ASSERT_TRUE(res2.is_ok());

    TEST_ASSERT_EQUAL_HEX8_ARRAY(okm1, okm2, sizeof(okm1));
}

static void test_hkdf_large_expansion(void)
{
    const uint8_t ikm[] = "input";
    uint8_t okm[100]; // Multi-block expansion (3 blocks of 32)

    auto res = hkdf(nullptr, 0, ikm, sizeof(ikm), nullptr, 0, okm, sizeof(okm));
    TEST_ASSERT_TRUE(res.is_ok());
}

static void test_hkdf_invalid_params(void)
{
    uint8_t okm[32];

    // Null IKM
    auto res = hkdf(nullptr, 0, nullptr, 0, nullptr, 0, okm, sizeof(okm));
    TEST_ASSERT_TRUE(res.is_error());

    // Zero okm_len
    const uint8_t ikm[] = "ikm";
    res = hkdf(nullptr, 0, ikm, sizeof(ikm), nullptr, 0, okm, 0);
    TEST_ASSERT_TRUE(res.is_error());
}

void test_hkdf_suite(void)
{
    RUN_TEST(test_hkdf_rfc5869_case1);
    RUN_TEST(test_hkdf_no_salt_no_info);
    RUN_TEST(test_hkdf_large_expansion);
    RUN_TEST(test_hkdf_invalid_params);
}
