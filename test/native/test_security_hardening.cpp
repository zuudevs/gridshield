#include <unity.h>
#include "security/crypto.hpp"
#include "native/platform_native.hpp"
#include <vector>
#include <cstring>
#include <iostream>

using namespace gridshield::security;
using namespace gridshield::core;
using namespace gridshield::platform::native;

static NativeCrypto* g_crypto = nullptr;
static CryptoEngine* g_engine = nullptr;

void setUp(void) {
    g_crypto = new NativeCrypto();
    g_engine = new CryptoEngine(*g_crypto);
}

void tearDown(void) {
    delete g_engine;
    delete g_crypto;
    g_engine = nullptr;
    g_crypto = nullptr;
}

void test_aes_gcm_encrypt_decrypt(void) {
    uint8_t key[AES_KEY_SIZE];
    uint8_t nonce[NONCE_SIZE];
    
    // Generate random key and nonce
    TEST_ASSERT_TRUE(g_crypto->random_bytes(key, AES_KEY_SIZE).is_ok());
    TEST_ASSERT_TRUE(g_crypto->random_bytes(nonce, NONCE_SIZE).is_ok());
    
    const char* plaintext_str = "GridShield Secured Data";
    size_t pt_len = strlen(plaintext_str);
    
    uint8_t ciphertext[128];
    uint8_t tag[AES_GCM_TAG_SIZE];
    
    // Encrypt
    auto enc_result = g_engine->encrypt_aes_gcm(key, nonce, 
        reinterpret_cast<const uint8_t*>(plaintext_str), pt_len, 
        ciphertext, tag);
        
    TEST_ASSERT_TRUE(enc_result.is_ok());
    TEST_ASSERT_EQUAL_UINT32(pt_len, enc_result.value());
    
    // Decrypt
    uint8_t decrypted[128];
    auto dec_result = g_engine->decrypt_aes_gcm(key, nonce, 
        ciphertext, enc_result.value(), 
        tag, decrypted);
        
    TEST_ASSERT_TRUE(dec_result.is_ok());
    TEST_ASSERT_EQUAL_UINT32(pt_len, dec_result.value());
    
    // Verify content
    TEST_ASSERT_EQUAL_UINT8_ARRAY(reinterpret_cast<const uint8_t*>(plaintext_str), decrypted, pt_len);
}

void test_aes_gcm_tamper_tag(void) {
    uint8_t key[AES_KEY_SIZE];
    uint8_t nonce[NONCE_SIZE];
    g_crypto->random_bytes(key, AES_KEY_SIZE);
    g_crypto->random_bytes(nonce, NONCE_SIZE);
    
    const char* plaintext_str = "Critical Data";
    size_t pt_len = strlen(plaintext_str);
    
    uint8_t ciphertext[64];
    uint8_t tag[AES_GCM_TAG_SIZE];
    
    // Encrypt
    auto enc_res = g_engine->encrypt_aes_gcm(
        key, nonce, 
        reinterpret_cast<const uint8_t*>(plaintext_str), pt_len, 
        ciphertext, tag);
    TEST_ASSERT_TRUE(enc_res.is_ok());
    
    // Tamper with tag
    tag[0] ^= 0xFF;
    
    // Decrypt should fail
    uint8_t decrypted[64];
    auto dec_res = g_engine->decrypt_aes_gcm(
        key, nonce, ciphertext, enc_res.value(), tag, decrypted);
        
    TEST_ASSERT_FALSE(dec_res.is_ok());
    TEST_ASSERT_EQUAL(static_cast<int>(ErrorCode::IntegrityViolation), static_cast<int>(dec_res.error().code));
}

void test_aes_gcm_tamper_ciphertext(void) {
    uint8_t key[AES_KEY_SIZE];
    uint8_t nonce[NONCE_SIZE];
    g_crypto->random_bytes(key, AES_KEY_SIZE);
    g_crypto->random_bytes(nonce, NONCE_SIZE);
    
    const char* plaintext_str = "Tamper This";
    size_t pt_len = strlen(plaintext_str);
    
    uint8_t ciphertext[64];
    uint8_t tag[AES_GCM_TAG_SIZE];
    
    // Encrypt
    auto enc = g_engine->encrypt_aes_gcm(key, nonce, reinterpret_cast<const uint8_t*>(plaintext_str), pt_len, ciphertext, tag);
    TEST_ASSERT_TRUE(enc.is_ok());
    
    // Tamper with ciphertext
    ciphertext[0] ^= 0xFF;
    
    // Decrypt should fail
    uint8_t buf[64];
    auto dec = g_engine->decrypt_aes_gcm(key, nonce, ciphertext, pt_len, tag, buf);
    TEST_ASSERT_FALSE(dec.is_ok());
}

void test_rng_quality(void) {
    uint8_t buf1[32];
    uint8_t buf2[32];
    
    TEST_ASSERT_TRUE(g_crypto->random_bytes(buf1, 32).is_ok());
    TEST_ASSERT_TRUE(g_crypto->random_bytes(buf2, 32).is_ok());
    
    // Very unlikely to be identical
    TEST_ASSERT_NOT_EQUAL(0, std::memcmp(buf1, buf2, 32));
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_aes_gcm_encrypt_decrypt);
    RUN_TEST(test_aes_gcm_tamper_tag);
    RUN_TEST(test_aes_gcm_tamper_ciphertext);
    RUN_TEST(test_rng_quality);
    UNITY_END();
    return 0;
}
