/**
 * @file device_provisioning.hpp
 * @brief Zero-touch device provisioning
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::cloud {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t PROV_MAX_SCOPE_ID_LENGTH = 64;
static constexpr size_t PROV_MAX_REG_ID_LENGTH = 64;
static constexpr size_t PROV_MAX_ENDPOINT_LENGTH = 128;
static constexpr size_t PROV_MAX_DEVICE_ID_LENGTH = 64;
static constexpr size_t PROV_MAX_KEY_LENGTH = 64;

// ============================================================================
// Types
// ============================================================================

enum class AttestationMethod : uint8_t
{
    SymmetricKey = 0,
    X509Certificate = 1,
    TPM = 2
};

enum class ProvisioningState : uint8_t
{
    NotProvisioned = 0,
    Registering = 1,
    Provisioned = 2,
    Failed = 3
};

struct ProvisioningConfig
{
    std::array<char, PROV_MAX_ENDPOINT_LENGTH> service_endpoint{};
    std::array<char, PROV_MAX_SCOPE_ID_LENGTH> scope_id{};
    std::array<char, PROV_MAX_REG_ID_LENGTH> registration_id{};
    std::array<uint8_t, PROV_MAX_KEY_LENGTH> symmetric_key{};
    uint8_t key_length{0};
    AttestationMethod attestation{AttestationMethod::SymmetricKey};
};

struct ProvisioningCredentials
{
    std::array<char, PROV_MAX_ENDPOINT_LENGTH> assigned_endpoint{};
    std::array<char, PROV_MAX_DEVICE_ID_LENGTH> assigned_device_id{};
    bool valid{false};
};

// ============================================================================
// Interface
// ============================================================================

class IDeviceProvisioning
{
public:
    virtual ~IDeviceProvisioning() = default;

    virtual core::Result<void> init(const ProvisioningConfig& config) noexcept = 0;
    virtual core::Result<ProvisioningCredentials> provision() noexcept = 0;
    virtual bool is_provisioned() const noexcept = 0;
    virtual ProvisioningState state() const noexcept = 0;
    virtual core::Result<ProvisioningCredentials> get_credentials() const noexcept = 0;
};

// ============================================================================
// Stub Implementation
// ============================================================================

class DeviceProvisioning final : public IDeviceProvisioning
{
public:
    core::Result<void> init(const ProvisioningConfig& config) noexcept override
    {
        if (config.registration_id[0] == '\0') {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (config.scope_id[0] == '\0') {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        config_ = config;
        state_ = ProvisioningState::NotProvisioned;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<ProvisioningCredentials> provision() noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<ProvisioningCredentials>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (simulate_failure_) {
            state_ = ProvisioningState::Failed;
            return core::Result<ProvisioningCredentials>(
                GS_MAKE_ERROR(core::ErrorCode::CloudProvisioningFailed));
        }

        state_ = ProvisioningState::Registering;
        credentials_ = simulated_credentials_;
        credentials_.valid = true;
        state_ = ProvisioningState::Provisioned;

        return core::Result<ProvisioningCredentials>(credentials_);
    }

    bool is_provisioned() const noexcept override
    {
        return state_ == ProvisioningState::Provisioned;
    }

    ProvisioningState state() const noexcept override
    {
        return state_;
    }

    core::Result<ProvisioningCredentials> get_credentials() const noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<ProvisioningCredentials>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (state_ != ProvisioningState::Provisioned) {
            return core::Result<ProvisioningCredentials>(
                GS_MAKE_ERROR(core::ErrorCode::CloudProvisioningFailed));
        }
        return core::Result<ProvisioningCredentials>(credentials_);
    }

    // === Test helpers ===
    void set_simulated_credentials(const ProvisioningCredentials& creds) noexcept
    {
        simulated_credentials_ = creds;
    }
    void set_simulate_failure(bool fail) noexcept
    {
        simulate_failure_ = fail;
    }

private:
    ProvisioningConfig config_{};
    ProvisioningCredentials credentials_{};
    ProvisioningCredentials simulated_credentials_{};
    ProvisioningState state_{ProvisioningState::NotProvisioned};
    bool initialized_{false};
    bool simulate_failure_{false};
};

} // namespace gridshield::cloud
