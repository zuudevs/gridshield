/**
 * @file key_rotation.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Key Rotation Service
 */

#pragma once

#include "security/key_storage.hpp"
#include "security/crypto.hpp"

namespace gridshield::security {

class KeyRotationService {
public:
    KeyRotationService(KeyStorage& storage, ICryptoEngine& crypto) noexcept
        : storage_(storage), crypto_(crypto) {}

    /**
     * @brief Rotate current primary key to backup and generate a new primary key
     */
    core::Result<void> rotate() noexcept {
        ECCKeyPair current_primary;
        
        // 1. Try to load current primary
        auto load_res = storage_.load(current_primary, KeySlot::Primary);
        if (load_res.is_ok()) {
            // Move current primary to backup
            GS_TRY(storage_.save(current_primary, KeySlot::Backup));
        }

        // 2. Generate new primary
        ECCKeyPair new_primary;
        GS_TRY(crypto_.generate_keypair(new_primary));

        // 3. Save new primary
        return storage_.save(new_primary, KeySlot::Primary);
    }

    /**
     * @brief Initialize system keys if not present
     */
    core::Result<void> initialize_if_needed() noexcept {
        ECCKeyPair temp;
        if (storage_.load(temp, KeySlot::Primary).is_error()) {
             // Primary missing, generate it
             return rotate();
        }
        return core::Result<void>();
    }

private:
    KeyStorage& storage_;
    ICryptoEngine& crypto_;
};

} // namespace gridshield::security
