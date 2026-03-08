/**
 * @file remote_config.hpp
 * @brief Remote configuration manager
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::network {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t CONFIG_MAX_ENTRIES = 16;
static constexpr size_t CONFIG_MAX_KEY_LENGTH = 32;
static constexpr size_t CONFIG_MAX_VALUE_LENGTH = 64;
static constexpr size_t CONFIG_VERSION_LENGTH = 16;
static constexpr size_t CONFIG_MAX_PAYLOAD_SIZE = 2048;

// ============================================================================
// Types
// ============================================================================

enum class ConfigState : uint8_t
{
    Idle = 0,
    Fetching = 1,
    Validating = 2,
    Applied = 3,
    RolledBack = 4,
    Error = 5
};

struct ConfigEntry
{
    std::array<char, CONFIG_MAX_KEY_LENGTH> key{};
    std::array<char, CONFIG_MAX_VALUE_LENGTH> value{};
    bool active{false};
};

struct ConfigSchema
{
    std::array<char, CONFIG_VERSION_LENGTH> version{};
    std::array<ConfigEntry, CONFIG_MAX_ENTRIES> entries{};
    uint16_t entry_count{0};
    uint32_t checksum{0};
};

struct RemoteConfigSettings
{
    const char* config_topic{nullptr};
    uint32_t fetch_interval_ms{60000};
    bool validate_checksum{true};
};

// ============================================================================
// Remote Config Manager
// ============================================================================

class RemoteConfigManager
{
public:
    core::Result<void> init(const RemoteConfigSettings& settings) noexcept
    {
        if (settings.config_topic == nullptr) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        settings_ = settings;
        state_ = ConfigState::Idle;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> apply(const ConfigSchema& schema) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (schema.entry_count == 0 || schema.entry_count > CONFIG_MAX_ENTRIES) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (settings_.validate_checksum && schema.checksum == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::ConfigurationError);
        }

        // Save previous for rollback
        if (state_ == ConfigState::Applied) {
            previous_schema_ = current_schema_;
            has_previous_ = true;
        }

        current_schema_ = schema;
        state_ = ConfigState::Applied;
        apply_count_++;
        return core::Result<void>{};
    }

    core::Result<void> rollback() noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (!has_previous_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidState);
        }
        current_schema_ = previous_schema_;
        state_ = ConfigState::RolledBack;
        has_previous_ = false;
        return core::Result<void>{};
    }

    core::Result<const char*> get(const char* key) const noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<const char*>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (key == nullptr) {
            return core::Result<const char*>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        for (uint16_t entry_idx = 0; entry_idx < current_schema_.entry_count; ++entry_idx) {
            const auto& entry = current_schema_.entries[entry_idx];
            if (entry.active && std::strncmp(entry.key.data(), key, CONFIG_MAX_KEY_LENGTH) == 0) {
                return core::Result<const char*>(entry.value.data());
            }
        }
        return core::Result<const char*>(GS_MAKE_ERROR(core::ErrorCode::ConfigurationError));
    }

    ConfigState state() const noexcept
    {
        return state_;
    }
    const ConfigSchema& current() const noexcept
    {
        return current_schema_;
    }
    uint32_t apply_count() const noexcept
    {
        return apply_count_;
    }
    bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    RemoteConfigSettings settings_{};
    ConfigSchema current_schema_{};
    ConfigSchema previous_schema_{};
    ConfigState state_{ConfigState::Idle};
    bool initialized_{false};
    bool has_previous_{false};
    uint32_t apply_count_{0};
};

} // namespace gridshield::network
