/**
 * @file hw_crypto.hpp
 * @brief Hardware crypto acceleration interface
 *
 * @note Header-only, zero heap allocation.
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
static constexpr size_t HW_AES_KEY_SIZE = 32;
static constexpr size_t HW_AES_BLOCK_SIZE = 16;
static constexpr size_t HW_AES_GCM_IV_SIZE = 12;
static constexpr size_t HW_AES_GCM_TAG_SIZE = 16;
static constexpr size_t HW_SHA256_DIGEST_SIZE = 32;
static constexpr size_t HW_HMAC_SHA256_SIZE = 32;
static constexpr size_t HW_CRYPTO_MAX_DATA_SIZE = 4096;

// ============================================================================
// Types
// ============================================================================

enum class AesMode : uint8_t
{
    CBC = 0,
    GCM = 1
};

struct CryptoOutput
{
    std::array<uint8_t, HW_CRYPTO_MAX_DATA_SIZE> data{};
    size_t length{0};
    std::array<uint8_t, HW_AES_GCM_TAG_SIZE> tag{};
};

// ============================================================================
// Interface
// ============================================================================

class IHwCrypto
{
public:
    virtual ~IHwCrypto() = default;

    virtual core::Result<void> init() noexcept = 0;

    virtual core::Result<CryptoOutput> aes_gcm_encrypt(const uint8_t* key,
                                                       const uint8_t* iv,
                                                       const uint8_t* plaintext,
                                                       size_t plaintext_len,
                                                       const uint8_t* aad,
                                                       size_t aad_len) noexcept = 0;

    virtual core::Result<CryptoOutput> aes_gcm_decrypt(const uint8_t* key,
                                                       const uint8_t* iv,
                                                       const uint8_t* ciphertext,
                                                       size_t ciphertext_len,
                                                       const uint8_t* tag,
                                                       const uint8_t* aad,
                                                       size_t aad_len) noexcept = 0;

    virtual core::Result<std::array<uint8_t, HW_SHA256_DIGEST_SIZE>>
    sha256(const uint8_t* data, size_t len) noexcept = 0;

    virtual core::Result<std::array<uint8_t, HW_HMAC_SHA256_SIZE>> hmac_sha256(
        const uint8_t* key, size_t key_len, const uint8_t* data, size_t data_len) noexcept = 0;

    virtual bool has_hw_acceleration() const noexcept = 0;
};

// ============================================================================
// Mock Implementation
// ============================================================================

using Sha256Digest = std::array<uint8_t, HW_SHA256_DIGEST_SIZE>;
using HmacDigest = std::array<uint8_t, HW_HMAC_SHA256_SIZE>;

class HwCrypto final : public IHwCrypto
{
public:
    core::Result<void> init() noexcept override
    {
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<CryptoOutput> aes_gcm_encrypt(const uint8_t* key,
                                               const uint8_t* iv,
                                               const uint8_t* plaintext,
                                               size_t plaintext_len,
                                               const uint8_t* /*aad*/,
                                               size_t /*aad_len*/) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (key == nullptr || iv == nullptr || plaintext == nullptr) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        if (plaintext_len == 0 || plaintext_len > HW_CRYPTO_MAX_DATA_SIZE) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        CryptoOutput output{};
        for (size_t byte_idx = 0; byte_idx < plaintext_len; ++byte_idx) {
            output.data[byte_idx] = plaintext[byte_idx] ^ key[0];
        }
        output.length = plaintext_len;
        std::memcpy(output.tag.data(),
                    iv,
                    std::min(static_cast<size_t>(HW_AES_GCM_IV_SIZE), HW_AES_GCM_TAG_SIZE));
        encrypt_count_++;
        return core::Result<CryptoOutput>(GS_MOVE(output));
    }

    core::Result<CryptoOutput> aes_gcm_decrypt(const uint8_t* key,
                                               const uint8_t* /*iv*/,
                                               const uint8_t* ciphertext,
                                               size_t ciphertext_len,
                                               const uint8_t* /*tag*/,
                                               const uint8_t* /*aad*/,
                                               size_t /*aad_len*/) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (key == nullptr || ciphertext == nullptr) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        if (ciphertext_len == 0 || ciphertext_len > HW_CRYPTO_MAX_DATA_SIZE) {
            return core::Result<CryptoOutput>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        CryptoOutput output{};
        for (size_t byte_idx = 0; byte_idx < ciphertext_len; ++byte_idx) {
            output.data[byte_idx] = ciphertext[byte_idx] ^ key[0];
        }
        output.length = ciphertext_len;
        decrypt_count_++;
        return core::Result<CryptoOutput>(GS_MOVE(output));
    }

    core::Result<Sha256Digest> sha256(const uint8_t* data, size_t len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<Sha256Digest>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (data == nullptr || len == 0) {
            return core::Result<Sha256Digest>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        Sha256Digest digest{};
        uint8_t xor_val = 0;
        for (size_t byte_idx = 0; byte_idx < len; ++byte_idx) {
            xor_val ^= data[byte_idx];
        }
        digest.fill(xor_val);
        hash_count_++;
        return core::Result<Sha256Digest>(digest);
    }

    core::Result<HmacDigest> hmac_sha256(const uint8_t* key,
                                         size_t key_len,
                                         const uint8_t* data,
                                         size_t data_len) noexcept override
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<HmacDigest>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (key == nullptr || data == nullptr || key_len == 0 || data_len == 0) {
            return core::Result<HmacDigest>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        HmacDigest mac{};
        uint8_t xor_val = key[0];
        for (size_t byte_idx = 0; byte_idx < data_len; ++byte_idx) {
            xor_val ^= data[byte_idx];
        }
        mac.fill(xor_val);
        hmac_count_++;
        return core::Result<HmacDigest>(mac);
    }

    bool has_hw_acceleration() const noexcept override
    {
        return false;
    }

    // === Test helpers ===
    uint32_t encrypt_count() const noexcept
    {
        return encrypt_count_;
    }
    uint32_t decrypt_count() const noexcept
    {
        return decrypt_count_;
    }
    uint32_t hash_count() const noexcept
    {
        return hash_count_;
    }
    uint32_t hmac_count() const noexcept
    {
        return hmac_count_;
    }

private:
    bool initialized_{false};
    uint32_t encrypt_count_{0};
    uint32_t decrypt_count_{0};
    uint32_t hash_count_{0};
    uint32_t hmac_count_{0};
};

} // namespace gridshield::security
