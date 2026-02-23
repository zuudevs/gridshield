/**
 * @file key_rotation.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Key Rotation Service
 */

#pragma once

#include "security/crypto.hpp"
#include "security/key_storage.hpp"

namespace gridshield::security {

class KeyRotationService
{
public:
    KeyRotationService(KeyStorage& storage, ICryptoEngine& crypto) noexcept
        : storage_(storage), crypto_(crypto)
    {}

    /**
     * @brief Rotate current primary key to backup and generate a new primary key
     *
     * Workflow: backup current → generate new → save → verify
     */
    core::Result<void> rotate() noexcept
    {
        ECCKeyPair current_primary;

        // 1. Backup current primary (if exists)
        auto load_res = storage_.load(current_primary, KeySlot::Primary);
        if (load_res.is_ok()) {
            GS_TRY(storage_.save(current_primary, KeySlot::Backup));
        }

        // 2. Generate new primary
        ECCKeyPair new_primary;
        GS_TRY(crypto_.generate_keypair(new_primary));

        // 3. Save new primary
        GS_TRY(storage_.save(new_primary, KeySlot::Primary));

        // 4. Verify the new key was stored correctly
        return verify_slot(KeySlot::Primary);
    }

    /**
     * @brief Restore primary key from backup slot
     *
     * Use when rotation fails or new key is corrupted.
     */
    core::Result<void> restore_from_backup() noexcept
    {
        ECCKeyPair backup;
        GS_TRY(storage_.load(backup, KeySlot::Backup));
        GS_TRY(storage_.save(backup, KeySlot::Primary));
        return verify_slot(KeySlot::Primary);
    }

    /**
     * @brief Check if a backup key exists
     */
    GS_NODISCARD bool has_backup() const noexcept
    {
        ECCKeyPair temp;
        return storage_.load(temp, KeySlot::Backup).is_ok();
    }

    /**
     * @brief Check if a primary key exists
     */
    GS_NODISCARD bool has_primary() const noexcept
    {
        ECCKeyPair temp;
        return storage_.load(temp, KeySlot::Primary).is_ok();
    }

    /**
     * @brief Initialize system keys if not present
     */
    core::Result<void> initialize_if_needed() noexcept
    {
        if (!has_primary()) {
            return rotate();
        }
        return core::Result<void>();
    }

private:
    /**
     * @brief Verify a key slot can be loaded back correctly
     */
    core::Result<void> verify_slot(KeySlot slot) const noexcept
    {
        ECCKeyPair verify;
        GS_TRY(storage_.load(verify, slot));
        if (!verify.has_private_key() || !verify.has_public_key()) {
            return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation);
        }
        return core::Result<void>();
    }

    KeyStorage& storage_;
    ICryptoEngine& crypto_;
};

} // namespace gridshield::security
