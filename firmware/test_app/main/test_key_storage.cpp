/**
 * @file test_key_storage.cpp
 * @brief Unit tests for KeyStorage
 */

#include "unity.h"
#include "security/key_storage.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::security;
using namespace gridshield::platform;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

struct KeyStorageTestFixture {
    MockTime time;
    MockGPIO gpio;
    MockInterrupt interrupt;
    MockCrypto crypto;
    MockComm comm;
    MockStorage storage;
    PlatformServices services;

    KeyStorageTestFixture() noexcept {
        services.time = &time;
        services.gpio = &gpio;
        services.interrupt = &interrupt;
        services.crypto = &crypto;
        services.storage = &storage;
        services.comm = &comm;
    }
};

// Generate a dummy keypair with known bytes
static ECCKeyPair make_test_keypair(MockCrypto& crypto) {
    ECCKeyPair kp;
    // Fill with deterministic data
    uint8_t pub[ECCKeyPair::PUBLIC_KEY_SIZE];
    uint8_t priv[ECCKeyPair::PRIVATE_KEY_SIZE];
    crypto.random_bytes(pub, ECCKeyPair::PUBLIC_KEY_SIZE);
    crypto.random_bytes(priv, ECCKeyPair::PRIVATE_KEY_SIZE);
    kp.load_public_key(pub, ECCKeyPair::PUBLIC_KEY_SIZE);
    kp.load_private_key(priv, ECCKeyPair::PRIVATE_KEY_SIZE);
    return kp;
}

// ============================================================================
// Save + Load
// ============================================================================

static void test_ks_save_and_load(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    auto kp = make_test_keypair(f.crypto);
    auto save_res = ks.save(kp, KeySlot::Primary);
    TEST_ASSERT_TRUE(save_res.is_ok());

    ECCKeyPair loaded;
    auto load_res = ks.load(loaded, KeySlot::Primary);
    TEST_ASSERT_TRUE(load_res.is_ok());
    TEST_ASSERT_TRUE(loaded.has_private_key());
    TEST_ASSERT_TRUE(loaded.has_public_key());
}

// ============================================================================
// Erase
// ============================================================================

static void test_ks_erase(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    auto kp = make_test_keypair(f.crypto);
    ks.save(kp, KeySlot::Primary);

    auto erase_res = ks.erase(KeySlot::Primary);
    TEST_ASSERT_TRUE(erase_res.is_ok());

    // Load after erase should fail (magic will be zeroed)
    ECCKeyPair loaded;
    auto load_res = ks.load(loaded, KeySlot::Primary);
    TEST_ASSERT_TRUE(load_res.is_error());
}

// ============================================================================
// Slots
// ============================================================================

static void test_ks_multiple_slots(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    auto kp1 = make_test_keypair(f.crypto);
    auto kp2 = make_test_keypair(f.crypto);

    TEST_ASSERT_TRUE(ks.save(kp1, KeySlot::Primary).is_ok());
    TEST_ASSERT_TRUE(ks.save(kp2, KeySlot::Backup).is_ok());

    ECCKeyPair loaded1, loaded2;
    TEST_ASSERT_TRUE(ks.load(loaded1, KeySlot::Primary).is_ok());
    TEST_ASSERT_TRUE(ks.load(loaded2, KeySlot::Backup).is_ok());
}

// ============================================================================
// Save without keys
// ============================================================================

static void test_ks_save_empty_keypair(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    ECCKeyPair empty;
    auto result = ks.save(empty, KeySlot::Primary);
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Load from empty storage
// ============================================================================

static void test_ks_load_empty_storage(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    ECCKeyPair loaded;
    auto result = ks.load(loaded, KeySlot::Primary);
    // Should fail — no magic header
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Key round-trip integrity (pub/priv keys match)
// ============================================================================

static void test_ks_key_roundtrip_integrity(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    auto kp = make_test_keypair(f.crypto);
    const uint8_t* orig_pub = kp.get_public_key();
    const uint8_t* orig_priv = kp.get_private_key();

    ks.save(kp, KeySlot::Primary);

    ECCKeyPair loaded;
    ks.load(loaded, KeySlot::Primary);

    TEST_ASSERT_EQUAL_MEMORY(orig_pub, loaded.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
    TEST_ASSERT_EQUAL_MEMORY(orig_priv, loaded.get_private_key(), ECCKeyPair::PRIVATE_KEY_SIZE);
}

// ============================================================================
// Erase backup slot doesn't affect primary
// ============================================================================

static void test_ks_erase_backup_preserves_primary(void) {
    KeyStorageTestFixture f;
    KeyStorage ks(f.services);

    auto kp = make_test_keypair(f.crypto);
    ks.save(kp, KeySlot::Primary);
    ks.save(kp, KeySlot::Backup);

    ks.erase(KeySlot::Backup);

    ECCKeyPair loaded;
    TEST_ASSERT_TRUE(ks.load(loaded, KeySlot::Primary).is_ok());

    ECCKeyPair loaded_backup;
    TEST_ASSERT_TRUE(ks.load(loaded_backup, KeySlot::Backup).is_error());
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_key_storage_suite(void) {
    RUN_TEST(test_ks_save_and_load);
    RUN_TEST(test_ks_erase);
    RUN_TEST(test_ks_multiple_slots);
    RUN_TEST(test_ks_save_empty_keypair);
    RUN_TEST(test_ks_load_empty_storage);
    RUN_TEST(test_ks_key_roundtrip_integrity);
    RUN_TEST(test_ks_erase_backup_preserves_primary);
}
