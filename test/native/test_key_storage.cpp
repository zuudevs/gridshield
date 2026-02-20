#include <unity.h>
#include "platform_native.hpp"
#include "security/key_storage.hpp"
#include <cstring>

using namespace gridshield;

// Global test objects
static platform::native::NativeStorage* native_storage = nullptr;
static platform::native::NativeCrypto* native_crypto = nullptr;
static platform::PlatformServices services;

void setUp(void) {
    native_storage = new platform::native::NativeStorage();
    native_crypto = new platform::native::NativeCrypto();
    
    // Wire up services
    services.storage = native_storage;
    services.crypto = native_crypto;
    // Others not needed for this test
    services.time = nullptr;
    services.gpio = nullptr;
    services.interrupt = nullptr;
    services.comm = nullptr;
}

void tearDown(void) {
    delete native_storage;
    delete native_crypto;
    native_storage = nullptr;
    native_crypto = nullptr;
}

void test_key_storage_save_load(void) {
    security::KeyStorage key_storage(services);
    
    security::ECCKeyPair original_keypair;
    TEST_ASSERT_TRUE(original_keypair.generate().is_ok());
    
    // Save
    TEST_ASSERT_TRUE(key_storage.save(original_keypair).is_ok());
    
    // Load
    security::ECCKeyPair loaded_keypair;
    TEST_ASSERT_TRUE(key_storage.load(loaded_keypair).is_ok());
    
    // Verify public key match
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        original_keypair.get_public_key(),
        loaded_keypair.get_public_key(),
        security::ECCKeyPair::PUBLIC_KEY_SIZE
    );
    
    // Verify private key match
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        original_keypair.get_private_key(),
        loaded_keypair.get_private_key(),
        security::ECCKeyPair::PRIVATE_KEY_SIZE
    );
}

void test_key_storage_integrity_check(void) {
    security::KeyStorage key_storage(services);
    security::ECCKeyPair keypair;
    keypair.generate();
    key_storage.save(keypair);
    
    // Corrupt data in storage
    uint8_t buffer[108];
    services.storage->read(0, buffer, 108);
    buffer[50] ^= 0xFF; // Flip bits in key data
    services.storage->write(0, buffer, 108);
    
    // Load should fail with IntegrityViolation
    security::ECCKeyPair loaded;
    auto result = key_storage.load(loaded);
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::IntegrityViolation, result.error().code);
}

void test_key_storage_magic_check(void) {
    // Write garbage to storage default address
    uint8_t garbage[4] = {0xAA, 0xBB, 0xCC, 0xDD};
    services.storage->write(0, garbage, 4);
    
    security::KeyStorage key_storage(services);
    security::ECCKeyPair loaded;
    auto result = key_storage.load(loaded);
    
    // Should fail (Magic mismatch)
    TEST_ASSERT_TRUE(result.is_error());
    TEST_ASSERT_EQUAL(core::ErrorCode::IntegrityViolation, result.error().code);
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_key_storage_save_load);
    RUN_TEST(test_key_storage_integrity_check);
    RUN_TEST(test_key_storage_magic_check);
    return UNITY_END();
}
