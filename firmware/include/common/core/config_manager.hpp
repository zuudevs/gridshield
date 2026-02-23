/**
 * @file config_manager.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Runtime configuration management with NVS persistence
 * @version 1.0
 * @date 2026-02-23
 *
 * Stores/loads SystemConfig from NVS with fallback to compiled defaults.
 *
 * NVS Layout:
 *   [MAGIC: 4B] [VERSION: 1B] [RSVD: 3B] [CONFIG: sizeof(SystemConfig)] [CRC32: 4B]
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/system.hpp"
#include "platform/platform.hpp"

namespace gridshield::core {

class ConfigManager {
public:
    static constexpr uint32_t CONFIG_MAGIC = 0x47534346; // "GSCF" (GridShield Config)
    static constexpr uint8_t CONFIG_VERSION = 1;
    static constexpr uint32_t CONFIG_ADDRESS = 512; // After key storage area
    static constexpr size_t HEADER_SIZE = 8; // magic(4) + version(1) + reserved(3)
    static constexpr size_t FOOTER_SIZE = 4; // crc32(4)
    static constexpr size_t TOTAL_SIZE = HEADER_SIZE + sizeof(SystemConfig) + FOOTER_SIZE;

    explicit ConfigManager(platform::PlatformServices& platform) noexcept
        : platform_(platform) {}

    /**
     * @brief Save current config to NVS
     */
    core::Result<void> save(const SystemConfig& config) noexcept {
        uint8_t buffer[TOTAL_SIZE];
        memset(buffer, 0, TOTAL_SIZE);

        // Header
        uint32_t magic = CONFIG_MAGIC;
        memcpy(buffer, &magic, 4);
        buffer[4] = CONFIG_VERSION;

        // Config data
        memcpy(buffer + HEADER_SIZE, &config, sizeof(SystemConfig));

        // CRC32
        auto crc_res = platform_.crypto->crc32(buffer, TOTAL_SIZE - FOOTER_SIZE);
        if (crc_res.is_error()) return crc_res.error();

        uint32_t crc = crc_res.value();
        memcpy(buffer + TOTAL_SIZE - FOOTER_SIZE, &crc, 4);

        return platform_.storage->write(CONFIG_ADDRESS, buffer, TOTAL_SIZE).as_void();
    }

    /**
     * @brief Load config from NVS
     */
    core::Result<SystemConfig> load() noexcept {
        uint8_t buffer[TOTAL_SIZE];

        auto read_res = platform_.storage->read(CONFIG_ADDRESS, buffer, TOTAL_SIZE);
        if (read_res.is_error()) {
            return core::Result<SystemConfig>(read_res.error());
        }

        // Verify magic
        uint32_t magic;
        memcpy(&magic, buffer, 4);
        if (magic != CONFIG_MAGIC) {
            return core::Result<SystemConfig>(
                GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation));
        }

        // Verify CRC
        auto crc_res = platform_.crypto->crc32(buffer, TOTAL_SIZE - FOOTER_SIZE);
        if (crc_res.is_error()) {
            return core::Result<SystemConfig>(crc_res.error());
        }

        uint32_t stored_crc;
        memcpy(&stored_crc, buffer + TOTAL_SIZE - FOOTER_SIZE, 4);
        if (crc_res.value() != stored_crc) {
            return core::Result<SystemConfig>(
                GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation));
        }

        // Deserialize config
        SystemConfig config;
        memcpy(&config, buffer + HEADER_SIZE, sizeof(SystemConfig));
        return core::Result<SystemConfig>(GS_MOVE(config));
    }

    /**
     * @brief Load config from NVS, or return defaults if not present
     */
    SystemConfig load_or_default(const SystemConfig& defaults) noexcept {
        auto result = load();
        if (result.is_ok()) {
            return result.value();
        }
        // Return compiled defaults
        return defaults;
    }

    /**
     * @brief Erase saved config from NVS
     */
    core::Result<void> erase() noexcept {
        return platform_.storage->erase(CONFIG_ADDRESS, TOTAL_SIZE);
    }

private:
    platform::PlatformServices& platform_;
};

} // namespace gridshield::core
