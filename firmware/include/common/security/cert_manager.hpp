/**
 * @file cert_manager.hpp
 * @brief X.509 certificate manager
 *
 * @note Header-only, zero heap allocation, uses named constants.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::security {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t CERT_MAX_SIZE = 2048;
static constexpr size_t CERT_KEY_MAX_SIZE = 512;
static constexpr size_t CERT_NAME_MAX_LENGTH = 64;

// ============================================================================
// Types
// ============================================================================

enum class CertFormat : uint8_t
{
    DER = 0,
    PEM = 1
};
enum class CertType : uint8_t
{
    CaCertificate = 0,
    ClientCertificate = 1,
    ClientKey = 2
};

struct CertInfo
{
    std::array<char, CERT_NAME_MAX_LENGTH> subject{};
    std::array<char, CERT_NAME_MAX_LENGTH> issuer{};
    uint64_t not_before{0};
    uint64_t not_after{0};
    CertFormat format{CertFormat::DER};
    bool loaded{false};
};

// ============================================================================
// Interface
// ============================================================================

class ICertManager
{
public:
    virtual ~ICertManager() = default;

    virtual core::Result<void> init() noexcept = 0;
    virtual core::Result<void>
    load_ca(const uint8_t* data, size_t len, CertFormat format) noexcept = 0;
    virtual core::Result<void>
    load_client_cert(const uint8_t* data, size_t len, CertFormat format) noexcept = 0;
    virtual core::Result<void>
    load_client_key(const uint8_t* data, size_t len, CertFormat format) noexcept = 0;
    virtual core::Result<bool> is_expired(CertType type,
                                          uint64_t current_epoch_s) const noexcept = 0;
    virtual core::Result<CertInfo> get_info(CertType type) const noexcept = 0;
    virtual bool has_cert(CertType type) const noexcept = 0;
    virtual core::Result<const uint8_t*> get_cert_data(CertType type,
                                                       size_t& out_len) const noexcept = 0;
};

// ============================================================================
// Stub Implementation
// ============================================================================

class CertManager final : public ICertManager
{
    struct CertSlot
    {
        std::array<uint8_t, CERT_MAX_SIZE> data{};
        size_t data_len{0};
        CertInfo info{};
    };

    static constexpr size_t SLOT_COUNT = 3;

public:
    core::Result<void> init() noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> load_ca(const uint8_t* data, size_t len, CertFormat format) noexcept override
    {
        return load_cert(CertType::CaCertificate, data, len, format);
    }

    core::Result<void>
    load_client_cert(const uint8_t* data, size_t len, CertFormat format) noexcept override
    {
        return load_cert(CertType::ClientCertificate, data, len, format);
    }

    core::Result<void>
    load_client_key(const uint8_t* data, size_t len, CertFormat format) noexcept override
    {
        return load_cert(CertType::ClientKey, data, len, format);
    }

    core::Result<bool> is_expired(CertType type, uint64_t current_epoch_s) const noexcept override
    {
        const auto& slot = get_slot(type);
        if (!slot.info.loaded) {
            return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::CertificateNotFound));
        }
        bool expired = (current_epoch_s >= slot.info.not_after);
        return core::Result<bool>(expired);
    }

    core::Result<CertInfo> get_info(CertType type) const noexcept override
    {
        const auto& slot = get_slot(type);
        if (!slot.info.loaded) {
            return core::Result<CertInfo>(GS_MAKE_ERROR(core::ErrorCode::CertificateNotFound));
        }
        return core::Result<CertInfo>(slot.info);
    }

    bool has_cert(CertType type) const noexcept override
    {
        return get_slot(type).info.loaded;
    }

    core::Result<const uint8_t*> get_cert_data(CertType type,
                                               size_t& out_len) const noexcept override
    {
        const auto& slot = get_slot(type);
        if (!slot.info.loaded) {
            return core::Result<const uint8_t*>(
                GS_MAKE_ERROR(core::ErrorCode::CertificateNotFound));
        }
        out_len = slot.data_len;
        return core::Result<const uint8_t*>(slot.data.data());
    }

    // === Test helpers ===
    void set_cert_info(CertType type, const CertInfo& info) noexcept
    {
        auto& slot = get_slot_mut(type);
        slot.info = info;
        slot.info.loaded = true;
    }

private:
    core::Result<void>
    load_cert(CertType type, const uint8_t* data, size_t len, CertFormat format) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (data == nullptr || len == 0 || len > CERT_MAX_SIZE) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        auto& slot = get_slot_mut(type);
        std::memcpy(slot.data.data(), data, len);
        slot.data_len = len;
        slot.info.format = format;
        slot.info.loaded = true;
        return core::Result<void>{};
    }

    const CertSlot& get_slot(CertType type) const noexcept
    {
        return slots_[static_cast<size_t>(type)];
    }

    CertSlot& get_slot_mut(CertType type) noexcept
    {
        return slots_[static_cast<size_t>(type)];
    }

    std::array<CertSlot, SLOT_COUNT> slots_{};
    bool initialized_{false};
};

} // namespace gridshield::security
