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
#include "platform/platform.hpp"
#include "security/crypto.hpp"

namespace gridshield::security {

enum class KeySlot : uint8_t
{
    Primary = 0,
    Backup = 1,
    Count = 2
};

/**
 * @brief Manages secure storage of cryptographic keys
 *
 * Layout in storage per slot:
 * [MAGIC: 4B] [VERSION: 1B] [RSVD: 3B] [PUBKEY: 64B] [PRIVKEY: 32B] [CRC32: 4B]
 * Total: 108 bytes
 */
class KeyStorage
{
public:
    static constexpr uint32_t STORAGE_MAGIC = 0x47534B53; // "GSKS" (GridShield Key Storage)
    static constexpr uint8_t STORAGE_VERSION = 1;
    static constexpr size_t SLOT_SIZE = 108;
    static constexpr uint32_t BASE_ADDRESS = 0;

    explicit KeyStorage(platform::PlatformServices& platform) noexcept : platform_(platform)
    {}

    /**
     * @brief Save keypair to a specific slot
     */
    core::Result<void> save(const ECCKeyPair& keypair, KeySlot slot = KeySlot::Primary) noexcept
    {
        if (!keypair.has_private_key() || !keypair.has_public_key()) {
            return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
        }

        uint32_t address = get_address(slot);
        uint8_t buffer[SLOT_SIZE];
        memset(buffer, 0, SLOT_SIZE);

        // Header
        uint32_t magic = STORAGE_MAGIC;
        memcpy(buffer, &magic, 4);
        buffer[4] = STORAGE_VERSION;

        // Keys
        memcpy(buffer + 8, keypair.get_public_key(), ECCKeyPair::PUBLIC_KEY_SIZE);
        memcpy(buffer + 8 + ECCKeyPair::PUBLIC_KEY_SIZE,
               keypair.get_private_key(),
               ECCKeyPair::PRIVATE_KEY_SIZE);

        // Checksum
        auto crc_res = platform_.crypto->crc32(buffer, SLOT_SIZE - 4);
        if (crc_res.is_error())
            return crc_res.error();

        uint32_t crc = crc_res.value();
        memcpy(buffer + SLOT_SIZE - 4, &crc, 4);

        // Write to storage
        return platform_.storage->write(address, buffer, SLOT_SIZE).as_void();
    }

    /**
     * @brief Load keypair from a specific slot
     */
    core::Result<void> load(ECCKeyPair& keypair, KeySlot slot = KeySlot::Primary) noexcept
    {
        uint32_t address = get_address(slot);
        uint8_t buffer[SLOT_SIZE];

        auto read_res = platform_.storage->read(address, buffer, SLOT_SIZE);
        if (read_res.is_error())
            return read_res.error();

        // Verify Magic
        uint32_t magic;
        memcpy(&magic, buffer, 4);
        if (magic != STORAGE_MAGIC) {
            return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation);
        }

        // Verify Checksum
        auto crc_res = platform_.crypto->crc32(buffer, SLOT_SIZE - 4);
        if (crc_res.is_error())
            return crc_res.error();

        uint32_t stored_crc;
        memcpy(&stored_crc, buffer + SLOT_SIZE - 4, 4);

        if (crc_res.value() != stored_crc) {
            return GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation);
        }

        // Load Keys
        auto pub_res = keypair.load_public_key(buffer + 8, ECCKeyPair::PUBLIC_KEY_SIZE);
        if (pub_res.is_error())
            return pub_res.error();

        auto priv_res = keypair.load_private_key(buffer + 8 + ECCKeyPair::PUBLIC_KEY_SIZE,
                                                 ECCKeyPair::PRIVATE_KEY_SIZE);
        if (priv_res.is_error())
            return priv_res.error();

        return core::Result<void>{};
    }

    /**
     * @brief Erase keys from a specific slot
     */
    core::Result<void> erase(KeySlot slot = KeySlot::Primary) noexcept
    {
        return platform_.storage->erase(get_address(slot), SLOT_SIZE);
    }

private:
    uint32_t get_address(KeySlot slot) const noexcept
    {
        return BASE_ADDRESS + (static_cast<uint32_t>(slot) * SLOT_SIZE);
    }

    platform::PlatformServices& platform_;
}; // class KeyStorage

} // namespace gridshield::security
