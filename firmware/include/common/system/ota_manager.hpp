/**
 * @file ota_manager.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Over-the-Air firmware update manager
 * @version 1.0
 * @date 2026-02-25
 *
 * Manages firmware OTA updates with signature verification,
 * rollback protection, and progress tracking.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <array>
#include <cstdint>

namespace gridshield::system {

// ============================================================================
// OTA CONSTANTS
// ============================================================================
static constexpr size_t OTA_MAX_URL_LENGTH = 256;
static constexpr size_t OTA_MAX_VERSION_LENGTH = 32;
static constexpr size_t OTA_SIGNATURE_SIZE = 64; // ECDSA P-256 signature
static constexpr size_t OTA_SHA256_DIGEST_SIZE = 32;
static constexpr size_t OTA_CHUNK_SIZE = 4096; // Download chunk size
static constexpr uint32_t OTA_DEFAULT_TIMEOUT_MS = 30000;
static constexpr uint8_t OTA_MAX_RETRY_COUNT = 3;
static constexpr uint8_t OTA_ROLLBACK_BOOT_COUNT = 3; // Boots before confirming

// ============================================================================
// OTA STATE
// ============================================================================
enum class OtaState : uint8_t
{
    Idle = 0,
    Checking = 1,
    Downloading = 2,
    Verifying = 3,
    Applying = 4,
    Rebooting = 5,
    RolledBack = 6,
    Failed = 7
};

// ============================================================================
// OTA FIRMWARE INFO
// ============================================================================
struct OtaFirmwareInfo
{
    char version[OTA_MAX_VERSION_LENGTH]{};
    char url[OTA_MAX_URL_LENGTH]{};
    uint32_t size_bytes{};
    std::array<uint8_t, OTA_SHA256_DIGEST_SIZE> sha256{};
    std::array<uint8_t, OTA_SIGNATURE_SIZE> signature{};
    bool force_update{false};

    GS_CONSTEXPR OtaFirmwareInfo() noexcept = default;
};

// ============================================================================
// OTA PROGRESS
// ============================================================================
struct OtaProgress
{
    uint32_t bytes_downloaded{};
    uint32_t total_bytes{};
    uint8_t percent{};

    GS_NODISCARD uint8_t calculate_percent() const noexcept
    {
        if (total_bytes == 0) {
            return 0;
        }
        static constexpr uint32_t PERCENT_SCALE = 100;
        return static_cast<uint8_t>((static_cast<uint64_t>(bytes_downloaded) * PERCENT_SCALE) /
                                    total_bytes);
    }
};

// ============================================================================
// OTA CONFIGURATION
// ============================================================================
struct OtaConfig
{
    const char* update_url{};
    uint32_t check_interval_ms{};
    uint32_t timeout_ms{OTA_DEFAULT_TIMEOUT_MS};
    uint8_t max_retries{OTA_MAX_RETRY_COUNT};
    bool auto_check{false};
    bool require_signature{true};

    GS_CONSTEXPR OtaConfig() noexcept = default;
};

// ============================================================================
// OTA MANAGER INTERFACE
// ============================================================================
class IOtaManager
{
public:
    virtual ~IOtaManager() noexcept = default;

    /// @brief Check for available firmware update.
    virtual core::Result<OtaFirmwareInfo> check_update() noexcept = 0;

    /// @brief Begin downloading and applying the firmware.
    virtual core::Result<void> start_update(const OtaFirmwareInfo& info) noexcept = 0;

    /// @brief Get current download/apply progress.
    virtual core::Result<OtaProgress> get_progress() noexcept = 0;

    /// @brief Abort an in-progress update.
    virtual core::Result<void> abort_update() noexcept = 0;

    /// @brief Confirm firmware after successful boot (prevents rollback).
    virtual core::Result<void> confirm_firmware() noexcept = 0;

    /// @brief Rollback to previous firmware.
    virtual core::Result<void> rollback() noexcept = 0;

    /// @brief Get current OTA state.
    GS_NODISCARD virtual OtaState get_state() const noexcept = 0;
};

// ============================================================================
// OTA MANAGER (Mock / Platform-Agnostic Stub)
// ============================================================================
class OtaManager : public IOtaManager
{
public:
    OtaManager() noexcept = default;

    core::Result<void> init(const OtaConfig& config) noexcept
    {
        config_ = config;
        state_ = OtaState::Idle;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<OtaFirmwareInfo> check_update() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<OtaFirmwareInfo>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        state_ = OtaState::Checking;

        // In a real impl, this would HTTP GET the update manifest from config_.update_url
        // For now, return "no update" by returning an empty info
        OtaFirmwareInfo info{};
        state_ = OtaState::Idle;
        return core::Result<OtaFirmwareInfo>(info);
    }

    core::Result<void> start_update(const OtaFirmwareInfo& info) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        if (info.size_bytes == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        current_info_ = info;
        progress_ = OtaProgress{};
        progress_.total_bytes = info.size_bytes;
        state_ = OtaState::Downloading;

        // Real implementation would:
        // 1. Begin HTTP download in chunks
        // 2. Write each chunk to OTA partition
        // 3. Verify SHA-256 after complete download
        // 4. Verify ECDSA signature if require_signature

        return core::Result<void>{};
    }

    core::Result<OtaProgress> get_progress() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<OtaProgress>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        progress_.percent = progress_.calculate_percent();
        return core::Result<OtaProgress>(progress_);
    }

    core::Result<void> abort_update() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        state_ = OtaState::Idle;
        progress_ = OtaProgress{};
        return core::Result<void>{};
    }

    core::Result<void> confirm_firmware() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        // Mark current firmware as valid (prevents auto-rollback)
        firmware_confirmed_ = true;
        return core::Result<void>{};
    }

    core::Result<void> rollback() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        state_ = OtaState::RolledBack;
        // Real implementation would call esp_ota_mark_app_invalid_rollback_and_reboot()
        return core::Result<void>{};
    }

    GS_NODISCARD OtaState get_state() const noexcept override
    {
        return state_;
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }
    GS_NODISCARD bool is_firmware_confirmed() const noexcept
    {
        return firmware_confirmed_;
    }

    // === Test helpers ===
    void simulate_download_progress(uint32_t bytes) noexcept
    {
        progress_.bytes_downloaded = bytes;
        progress_.percent = progress_.calculate_percent();
    }

    void simulate_state(OtaState state) noexcept
    {
        state_ = state;
    }

private:
    OtaConfig config_{};
    OtaFirmwareInfo current_info_{};
    OtaProgress progress_{};
    OtaState state_{OtaState::Idle};
    bool initialized_{false};
    bool firmware_confirmed_{false};
};

} // namespace gridshield::system
