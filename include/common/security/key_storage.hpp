/**
 * @file key_storage.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Secure Key Storage Service
 * @version 0.1
 * @date 2026-02-20
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "security/crypto.hpp"
#include "platform/platform.hpp"

namespace gridshield {
namespace security {

/**
 * @brief Manages secure storage of cryptographic keys
 * 
 * Layout in storage:
 * [MAGIC: 4B] [VERSION: 1B] [RSVD: 3B] [PUBKEY: 64B] [PRIVKEY: 32B] [CRC32: 4B]
 * Total: 108 bytes
 */
class KeyStorage {
public:
    static constexpr uint32_t STORAGE_MAGIC = 0x47534B53; // "GSKS" (GridShield Key Storage)
    static constexpr uint8_t STORAGE_VERSION = 1;
    static constexpr size_t STORAGE_SIZE = 108;
    static constexpr uint32_t DEFAULT_ADDRESS = 0; // Start of EEPROM

    explicit KeyStorage(platform::PlatformServices& platform) noexcept
        : platform_(platform) {}

    /**
     * @brief Save keypair to storage
     */
    core::Result<void> save(const ECCKeyPair& keypair, uint32_t address = DEFAULT_ADDRESS) noexcept {
        if (!keypair.has_private_key() || !keypair.has_public_key()) {
            return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed); // Invalid keypair
        }

        uint8_t buffer[STORAGE_SIZE];
        memset(buffer, 0, STORAGE_SIZE);

        // Header
        uint32_t magic = STORAGE_MAGIC;
        memcpy(buffer, &magic, 4);
        buffer[4] = STORAGE_VERSION;

        // Keys
        memcpy(buffer + 8, keypair.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
        memcpy(buffer + 8 + ECCKeyPair::PUBLIC_KEY_SIZE, keypair.get_private_key(), ECCKeyPair::PRIVATE_KEY_SIZE);

        // Checksum (calculate over header + keys)
        auto crc_res = platform_.crypto->crc32(buffer, STORAGE_SIZE - 4);
        if (crc_res.is_error()) return crc_res.error();

        uint32_t crc = crc_res.value();
        memcpy(buffer + STORAGE_SIZE - 4, &crc, 4);

        // Write to storage
        return platform_.storage->write(address, buffer, STORAGE_SIZE).as_void();
    }

    /**
     * @brief Load keypair from storage
     */
    core::Result<void> load(ECCKeyPair& keypair, uint32_t address = DEFAULT_ADDRESS) noexcept {
        uint8_t buffer[STORAGE_SIZE];
        
        auto read_res = platform_.storage->read(address, buffer, STORAGE_SIZE);
        if (read_res.is_error()) return read_res.error();

        // Verify Magic
        uint32_t magic;
        memcpy(&magic, buffer, 4);
        if (magic != STORAGE_MAGIC) {
            return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation); // Not initialized or corrupted
        }

        // Verify Checksum
        auto crc_res = platform_.crypto->crc32(buffer, STORAGE_SIZE - 4);
        if (crc_res.is_error()) return crc_res.error();

        uint32_t stored_crc;
        memcpy(&stored_crc, buffer + STORAGE_SIZE - 4, 4);

        if (crc_res.value() != stored_crc) {
             return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation);
        }

        // Load Keys
        auto pub_res = keypair.load_public_key(buffer + 8, ECCKeyPair::PUBLIC_KEY_SIZE);
        if (pub_res.is_error()) return pub_res.error();

        auto priv_res = keypair.load_private_key(buffer + 8 + ECCKeyPair::PUBLIC_KEY_SIZE, ECCKeyPair::PRIVATE_KEY_SIZE);
        if (priv_res.is_error()) return priv_res.error();

        return core::Result<void>();
    }

    /**
     * @brief Erase keys from storage
     */
    core::Result<void> erase(uint32_t address = DEFAULT_ADDRESS) noexcept {
        return platform_.storage->erase(address, STORAGE_SIZE);
    }

private:
    platform::PlatformServices& platform_;
};

} // namespace security
} // namespace gridshield
