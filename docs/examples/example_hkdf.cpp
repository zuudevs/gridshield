/**
 * @file example_hkdf.cpp
 * @brief GridShield HKDF-SHA256 — Usage Example
 *
 * Demonstrates:
 *   - HKDF Extract: derive a pseudorandom key (PRK) from input keying material
 *   - HKDF Expand: derive one or more output keys from the PRK
 *   - Full HKDF pipeline for key derivation
 *
 * HKDF (HMAC-based Key Derivation Function) is used to derive
 * cryptographically strong keys from shared secrets (e.g., ECDH output)
 * or weak input keying material.
 *
 * Reference: RFC 5869
 */

#include "platform/mock_platform.hpp"
#include "security/crypto.hpp"
#include "security/hkdf.hpp"


#include <cstring>

using namespace gridshield;
using namespace gridshield::security;

void example_hkdf() {
  // ---------------------------------------------------------------
  // 1. Setup Crypto Engine (needed for SHA-256 primitive)
  // ---------------------------------------------------------------
  platform::mock::MockCrypto mock_crypto;
  CryptoEngine crypto(mock_crypto);

  // ---------------------------------------------------------------
  // 2. Input Keying Material (IKM)
  // ---------------------------------------------------------------
  // In production, this comes from an ECDH shared secret or
  // a pre-shared key exchange.
  const uint8_t ikm[] = "shared_secret_from_ecdh_key_exchange";
  const size_t ikm_len = sizeof(ikm) - 1;

  // Optional salt (recommended: random, at least HashLen bytes)
  const uint8_t salt[] = "gridshield_v2_salt_value";
  const size_t salt_len = sizeof(salt) - 1;

  // ---------------------------------------------------------------
  // 3. HKDF Extract — Derive PRK from IKM + salt
  // ---------------------------------------------------------------
  uint8_t prk[SHA256_HASH_SIZE]; // 32-byte pseudorandom key

  auto extract_result = hkdf_extract(salt, salt_len, ikm, ikm_len, prk);
  if (extract_result.is_error()) {
    return;
  }

  // ---------------------------------------------------------------
  // 4. HKDF Expand — Derive Output Key Material (OKM) from PRK
  // ---------------------------------------------------------------
  // "info" context binds the derived key to a specific purpose
  // (prevents key reuse across different protocols/contexts)
  const uint8_t info_encrypt[] = "gridshield_aes_encryption_key";
  const uint8_t info_auth[] = "gridshield_hmac_auth_key";

  // Derive a 32-byte AES encryption key
  uint8_t aes_key[AES_KEY_SIZE];
  auto expand_enc =
      hkdf_expand(prk, sizeof(prk), info_encrypt, sizeof(info_encrypt) - 1,
                  aes_key, sizeof(aes_key));
  if (expand_enc.is_error()) {
    return;
  }

  // Derive a separate 32-byte HMAC authentication key
  uint8_t hmac_key[32];
  auto expand_auth =
      hkdf_expand(prk, sizeof(prk), info_auth, sizeof(info_auth) - 1, hmac_key,
                  sizeof(hmac_key));
  if (expand_auth.is_error()) {
    return;
  }

  // NOTE: aes_key and hmac_key are cryptographically independent —
  // compromising one does not reveal the other.

  // ---------------------------------------------------------------
  // 5. Use Derived Keys
  // ---------------------------------------------------------------
  // aes_key → CryptoEngine::encrypt_aes_gcm(aes_key, ...)
  // hmac_key → use for message authentication

  // ---------------------------------------------------------------
  // 6. Cleanup — zero sensitive key material
  // ---------------------------------------------------------------
  std::memset(prk, 0, sizeof(prk));
  std::memset(aes_key, 0, sizeof(aes_key));
  std::memset(hmac_key, 0, sizeof(hmac_key));
}
