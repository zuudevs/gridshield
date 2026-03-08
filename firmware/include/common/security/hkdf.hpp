/**
 * @file hkdf.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief HKDF (HMAC-based Key Derivation Function) — RFC 5869
 * @version 1.0
 * @date 2026-02-23
 *
 * Derives per-session encryption keys from ECDH shared secrets.
 * Uses mbedTLS HMAC-SHA256 under the hood.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "security/crypto.hpp"

namespace gridshield::security {

// ============================================================================
// HKDF CONSTANTS
// ============================================================================
constexpr size_t HKDF_HASH_SIZE = SHA256_HASH_SIZE;          // 32 bytes
constexpr size_t HKDF_MAX_OKM_LENGTH = 255 * HKDF_HASH_SIZE; // RFC 5869 limit

// ============================================================================
// HKDF FUNCTIONS
// ============================================================================

/**
 * @brief HKDF-Extract: extract a pseudorandom key from input keying material.
 *
 * @param salt   Optional salt (if null, uses zeros)
 * @param salt_len Length of salt
 * @param ikm    Input keying material (e.g., ECDH shared secret)
 * @param ikm_len Length of IKM
 * @param prk_out Output PRK buffer (must be HKDF_HASH_SIZE bytes)
 * @return Result<void>
 */
core::Result<void> hkdf_extract(const uint8_t* salt,
                                size_t salt_len,
                                const uint8_t* ikm,
                                size_t ikm_len,
                                uint8_t* prk_out) noexcept;

/**
 * @brief HKDF-Expand: expand PRK to desired length output keying material.
 *
 * @param prk     Pseudorandom key (from hkdf_extract)
 * @param prk_len Length of PRK (typically HKDF_HASH_SIZE)
 * @param info    Optional context/application-specific info
 * @param info_len Length of info
 * @param okm_out Output keying material buffer
 * @param okm_len Desired length of OKM (max HKDF_MAX_OKM_LENGTH)
 * @return Result<void>
 */
core::Result<void> hkdf_expand(const uint8_t* prk,
                               size_t prk_len,
                               const uint8_t* info,
                               size_t info_len,
                               uint8_t* okm_out,
                               size_t okm_len) noexcept;

/**
 * @brief One-shot HKDF: extract + expand in a single call.
 *
 * @param salt     Optional salt
 * @param salt_len Length of salt
 * @param ikm      Input keying material
 * @param ikm_len  Length of IKM
 * @param info     Optional context info
 * @param info_len Length of info
 * @param okm_out  Output buffer
 * @param okm_len  Desired output length
 * @return Result<void>
 */
core::Result<void> hkdf(const uint8_t* salt,
                        size_t salt_len,
                        const uint8_t* ikm,
                        size_t ikm_len,
                        const uint8_t* info,
                        size_t info_len,
                        uint8_t* okm_out,
                        size_t okm_len) noexcept;

} // namespace gridshield::security
