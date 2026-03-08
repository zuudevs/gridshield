/**
 * @file example_crypto.cpp
 * @brief GridShield CryptoEngine — Usage Example
 *
 * Demonstrates:
 *   - ECC keypair generation (secp256r1)
 *   - ECDSA sign and verify
 *   - AES-256-GCM encrypt and decrypt
 *   - SHA-256 hashing
 *   - Secure random byte generation
 */

#include "platform/mock_platform.hpp"
#include "security/crypto.hpp"


#include <cstring>

using namespace gridshield;
using namespace gridshield::security;

void example_crypto() {
  // ---------------------------------------------------------------
  // 1. Create CryptoEngine with platform crypto provider
  // ---------------------------------------------------------------
  platform::mock::MockCrypto mock_crypto;
  CryptoEngine crypto(mock_crypto);

  // ---------------------------------------------------------------
  // 2. Generate ECC Keypair
  // ---------------------------------------------------------------
  ECCKeyPair device_keypair;
  auto gen_result = crypto.generate_keypair(device_keypair);
  if (gen_result.is_error()) {
    // Handle: KeyGenerationFailed
    return;
  }

  // Access key material
  const uint8_t *pub_key = device_keypair.public_key();   // 64 bytes
  const uint8_t *priv_key = device_keypair.private_key(); // 32 bytes
  (void)pub_key;
  (void)priv_key;

  // ---------------------------------------------------------------
  // 3. ECDSA Sign & Verify
  // ---------------------------------------------------------------
  const uint8_t message[] = "GridShield meter reading data";
  uint8_t signature[ECCKeyPair::SIGNATURE_SIZE]; // 64 bytes

  // Sign with private key
  auto sign_result =
      crypto.sign(device_keypair, message, sizeof(message), signature);
  if (sign_result.is_error()) {
    return;
  }

  // Verify with public key
  auto verify_result =
      crypto.verify(device_keypair, message, sizeof(message), signature);
  if (verify_result.is_error()) {
    // Signature verification failed — possible tampering
    return;
  }

  // ---------------------------------------------------------------
  // 4. SHA-256 Hashing
  // ---------------------------------------------------------------
  uint8_t hash_output[SHA256_HASH_SIZE]; // 32 bytes
  auto hash_result = crypto.sha256(message, sizeof(message), hash_output);
  if (hash_result.is_error()) {
    return;
  }

  // ---------------------------------------------------------------
  // 5. AES-256-GCM Encrypt & Decrypt
  // ---------------------------------------------------------------
  uint8_t aes_key[AES_KEY_SIZE]; // 32 bytes
  uint8_t iv[AES_IV_SIZE];       // 12 bytes
  uint8_t tag[AES_TAG_SIZE];     // 16 bytes

  // Generate random key and IV
  crypto.random_bytes(aes_key, sizeof(aes_key));
  crypto.random_bytes(iv, sizeof(iv));

  const uint8_t plaintext[] = "Sensitive meter data payload";
  uint8_t ciphertext[128];
  uint8_t decrypted[128];

  // Encrypt
  auto enc_result =
      crypto.encrypt_aes_gcm(aes_key, iv, plaintext, sizeof(plaintext),
                             ciphertext, sizeof(ciphertext), tag);
  if (enc_result.is_error()) {
    return;
  }
  size_t cipher_len = enc_result.value();

  // Decrypt
  auto dec_result = crypto.decrypt_aes_gcm(aes_key, iv, ciphertext, cipher_len,
                                           decrypted, sizeof(decrypted), tag);
  if (dec_result.is_error()) {
    // Decryption failed — data integrity compromised
    return;
  }
  // decrypted now contains original plaintext

  // ---------------------------------------------------------------
  // 6. Secure Random Bytes
  // ---------------------------------------------------------------
  uint8_t nonce[16];
  auto rand_result = crypto.random_bytes(nonce, sizeof(nonce));
  (void)rand_result;

  // ---------------------------------------------------------------
  // 7. Cleanup — securely erase key material
  // ---------------------------------------------------------------
  device_keypair.clear(); // Zeroes private + public key memory
}
