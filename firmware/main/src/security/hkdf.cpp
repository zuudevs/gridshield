/**
 * @file hkdf.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief HKDF implementation using mbedTLS HMAC-SHA256
 * @version 1.0
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2026
 */

#include "security/hkdf.hpp"

#include <cstring>

// mbedTLS (built into ESP-IDF)
#include "mbedtls/md.h"

namespace gridshield::security {

// ============================================================================
// HMAC-SHA256 helper (wraps mbedTLS)
// ============================================================================
static core::Result<void> hmac_sha256(const uint8_t *key, size_t key_len,
                                      const uint8_t *data, size_t data_len,
                                      uint8_t *mac_out) noexcept {
  const mbedtls_md_info_t *md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info == nullptr) {
    return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
  }

  int ret = mbedtls_md_hmac(md_info, key, key_len, data, data_len, mac_out);
  if (ret != 0) {
    return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
  }

  return core::Result<void>();
}

// ============================================================================
// HKDF-Extract (RFC 5869 Section 2.2)
// PRK = HMAC-Hash(salt, IKM)
// ============================================================================
core::Result<void> hkdf_extract(const uint8_t *salt, size_t salt_len,
                                const uint8_t *ikm, size_t ikm_len,
                                uint8_t *prk_out) noexcept {
  if (GS_UNLIKELY(ikm == nullptr || ikm_len == 0 || prk_out == nullptr)) {
    return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
  }

  // If salt is not provided, use a string of HashLen zeros
  uint8_t zero_salt[HKDF_HASH_SIZE];
  if (salt == nullptr || salt_len == 0) {
    memset(zero_salt, 0, HKDF_HASH_SIZE);
    salt = zero_salt;
    salt_len = HKDF_HASH_SIZE;
  }

  return hmac_sha256(salt, salt_len, ikm, ikm_len, prk_out);
}

// ============================================================================
// HKDF-Expand (RFC 5869 Section 2.3)
// T(0) = empty
// T(i) = HMAC-Hash(PRK, T(i-1) || info || i)
// OKM = T(1) || T(2) || ... || T(N)  truncated to okm_len
// ============================================================================
core::Result<void> hkdf_expand(const uint8_t *prk, size_t prk_len,
                               const uint8_t *info, size_t info_len,
                               uint8_t *okm_out, size_t okm_len) noexcept {
  if (GS_UNLIKELY(prk == nullptr || prk_len == 0 || okm_out == nullptr ||
                  okm_len == 0)) {
    return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
  }

  if (okm_len > HKDF_MAX_OKM_LENGTH) {
    return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
  }

  const mbedtls_md_info_t *md_info =
      mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info == nullptr) {
    return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
  }

  uint8_t t_block[HKDF_HASH_SIZE]; // T(i)
  size_t t_len = 0;                // Length of T(i-1), 0 for first iteration

  size_t offset = 0;
  uint8_t counter = 1;

  while (offset < okm_len) {
    // Build input: T(i-1) || info || counter
    mbedtls_md_context_t ctx;
    mbedtls_md_init(&ctx);

    int ret = mbedtls_md_setup(&ctx, md_info, 1 /* HMAC */);
    if (ret != 0) {
      mbedtls_md_free(&ctx);
      return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }

    ret = mbedtls_md_hmac_starts(&ctx, prk, prk_len);
    if (ret != 0) {
      mbedtls_md_free(&ctx);
      return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }

    // T(i-1)
    if (t_len > 0) {
      ret = mbedtls_md_hmac_update(&ctx, t_block, t_len);
      if (ret != 0) {
        mbedtls_md_free(&ctx);
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
      }
    }

    // info
    if (info != nullptr && info_len > 0) {
      ret = mbedtls_md_hmac_update(&ctx, info, info_len);
      if (ret != 0) {
        mbedtls_md_free(&ctx);
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
      }
    }

    // counter byte
    ret = mbedtls_md_hmac_update(&ctx, &counter, 1);
    if (ret != 0) {
      mbedtls_md_free(&ctx);
      return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }

    ret = mbedtls_md_hmac_finish(&ctx, t_block);
    mbedtls_md_free(&ctx);

    if (ret != 0) {
      return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }

    t_len = HKDF_HASH_SIZE;

    // Copy to output
    size_t copy_len =
        (okm_len - offset < HKDF_HASH_SIZE) ? (okm_len - offset) : HKDF_HASH_SIZE;
    memcpy(okm_out + offset, t_block, copy_len);
    offset += copy_len;
    ++counter;
  }

  // Zeroize sensitive intermediate data
  volatile uint8_t *vptr = t_block;
  for (size_t i = 0; i < HKDF_HASH_SIZE; ++i) {
    vptr[i] = 0;
  }

  return core::Result<void>();
}

// ============================================================================
// One-shot HKDF
// ============================================================================
core::Result<void> hkdf(const uint8_t *salt, size_t salt_len,
                        const uint8_t *ikm, size_t ikm_len,
                        const uint8_t *info, size_t info_len,
                        uint8_t *okm_out, size_t okm_len) noexcept {
  uint8_t prk[HKDF_HASH_SIZE];

  auto result = hkdf_extract(salt, salt_len, ikm, ikm_len, prk);
  if (result.is_error()) {
    return result;
  }

  result = hkdf_expand(prk, HKDF_HASH_SIZE, info, info_len, okm_out, okm_len);

  // Zeroize PRK
  volatile uint8_t *vptr = prk;
  for (size_t i = 0; i < HKDF_HASH_SIZE; ++i) {
    vptr[i] = 0;
  }

  return result;
}

} // namespace gridshield::security
