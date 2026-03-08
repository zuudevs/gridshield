/**
 * @file test_key_rotation.cpp
 * @brief Unit tests for KeyRotationService
 */

#include "platform/mock_platform.hpp"
#include "security/key_rotation.hpp"
#include "unity.h"

using namespace gridshield;
using namespace gridshield::security;
using namespace gridshield::platform;
using namespace gridshield::platform::mock;

// ============================================================================
// Helpers
// ============================================================================

struct RotationFixture
{
    MockTime time;
    MockGPIO gpio;
    MockInterrupt interrupt;
    MockCrypto crypto;
    MockComm comm;
    MockStorage storage;
    PlatformServices services;

    RotationFixture() noexcept
    {
        services.time = &time;
        services.gpio = &gpio;
        services.interrupt = &interrupt;
        services.crypto = &crypto;
        services.storage = &storage;
        services.comm = &comm;
    }
};

// ============================================================================
// Initialize If Needed
// ============================================================================

static void test_rotation_init_creates_primary(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    TEST_ASSERT_FALSE(svc.has_primary());

    auto result = svc.initialize_if_needed();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(svc.has_primary());
}

static void test_rotation_init_skips_existing(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    // First init creates key
    svc.initialize_if_needed();

    // Load the key to compare later
    ECCKeyPair before;
    ks.load(before, KeySlot::Primary);

    // Second init should skip (not regenerate)
    auto result = svc.initialize_if_needed();
    TEST_ASSERT_TRUE(result.is_ok());

    ECCKeyPair after;
    ks.load(after, KeySlot::Primary);
    TEST_ASSERT_EQUAL_MEMORY(
        before.get_public_key(), after.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
}

// ============================================================================
// Rotation
// ============================================================================

static void test_rotation_creates_backup(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    svc.initialize_if_needed();
    TEST_ASSERT_FALSE(svc.has_backup());

    auto result = svc.rotate();
    TEST_ASSERT_TRUE(result.is_ok());
    TEST_ASSERT_TRUE(svc.has_backup());
    TEST_ASSERT_TRUE(svc.has_primary());
}

static void test_rotation_backup_has_old_primary(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    svc.initialize_if_needed();

    ECCKeyPair original;
    ks.load(original, KeySlot::Primary);

    svc.rotate();

    ECCKeyPair backup;
    ks.load(backup, KeySlot::Backup);

    // Backup should contain the original primary key
    TEST_ASSERT_EQUAL_MEMORY(
        original.get_public_key(), backup.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
}

static void test_rotation_primary_is_new_key(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    svc.initialize_if_needed();

    ECCKeyPair before;
    ks.load(before, KeySlot::Primary);

    svc.rotate();

    ECCKeyPair after;
    ks.load(after, KeySlot::Primary);

    // Primary should differ after rotation (new key generated)
    // Compare public keys — extremely unlikely to be the same
    TEST_ASSERT_FALSE(
        memcmp(before.get_public_key(), after.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE) == 0);
}

// ============================================================================
// Restore from Backup
// ============================================================================

static void test_restore_from_backup(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    svc.initialize_if_needed();

    ECCKeyPair original;
    ks.load(original, KeySlot::Primary);

    // Rotate (original → backup, new → primary)
    svc.rotate();

    // Restore backup → primary
    auto result = svc.restore_from_backup();
    TEST_ASSERT_TRUE(result.is_ok());

    ECCKeyPair restored;
    ks.load(restored, KeySlot::Primary);

    TEST_ASSERT_EQUAL_MEMORY(
        original.get_public_key(), restored.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
}

static void test_restore_no_backup_fails(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    // No backup exists
    auto result = svc.restore_from_backup();
    TEST_ASSERT_TRUE(result.is_error());
}

// ============================================================================
// Double Rotation
// ============================================================================

static void test_double_rotation(void)
{
    RotationFixture f;
    KeyStorage ks(f.services);
    CryptoEngine engine(f.crypto);
    KeyRotationService svc(ks, engine);

    svc.initialize_if_needed();
    svc.rotate(); // 1st rotation

    ECCKeyPair after_first;
    ks.load(after_first, KeySlot::Primary);

    svc.rotate(); // 2nd rotation

    // After 2nd rotation, backup should be the key from 1st rotation
    ECCKeyPair backup;
    ks.load(backup, KeySlot::Backup);

    TEST_ASSERT_EQUAL_MEMORY(
        after_first.get_public_key(), backup.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
}

// ============================================================================
// Suite Registration
// ============================================================================

void test_key_rotation_suite(void)
{
    RUN_TEST(test_rotation_init_creates_primary);
    RUN_TEST(test_rotation_init_skips_existing);
    RUN_TEST(test_rotation_creates_backup);
    RUN_TEST(test_rotation_backup_has_old_primary);
    RUN_TEST(test_rotation_primary_is_new_key);
    RUN_TEST(test_restore_from_backup);
    RUN_TEST(test_restore_no_backup_fails);
    RUN_TEST(test_double_rotation);
}
