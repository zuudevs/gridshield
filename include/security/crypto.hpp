/**
 * @file crypto.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "platform/platform.hpp"
#include <cstdint>

namespace gridshield::security {

constexpr size_t ECC_KEY_SIZE = 32;      // 256-bit
constexpr size_t ECC_SIGNATURE_SIZE = 64; // r + s components
constexpr size_t ECC_PUBLIC_KEY_SIZE = 64; // x + y coordinates
constexpr size_t AES_KEY_SIZE = 32;      // 256-bit
constexpr size_t AES_BLOCK_SIZE = 16;
constexpr size_t NONCE_SIZE = 12;

class ECCKeyPair {
public:
    ECCKeyPair() noexcept;
    ~ECCKeyPair();
    
    core::Result<void> generate() noexcept;
    core::Result<void> load_private_key(const uint8_t* key, size_t length) noexcept;
    core::Result<void> load_public_key(const uint8_t* key, size_t length) noexcept;
    
    const uint8_t* get_private_key() const noexcept;
    const uint8_t* get_public_key() const noexcept;
    
    bool has_private_key() const noexcept;
    bool has_public_key() const noexcept;
    
    void clear() noexcept;
    
private:
    uint8_t private_key_[ECC_KEY_SIZE];
    uint8_t public_key_[ECC_PUBLIC_KEY_SIZE];
    bool has_private_;
    bool has_public_;
};

class ICryptoEngine {
public:
    virtual ~ICryptoEngine() = default;
    
    // Key management
    virtual core::Result<void> generate_keypair(ECCKeyPair& keypair) noexcept = 0;
    
    // ECDSA operations
    virtual core::Result<void> sign(const ECCKeyPair& keypair,
                                   const uint8_t* message, size_t msg_len,
                                   uint8_t* signature_out) noexcept = 0;
    
    virtual core::Result<bool> verify(const ECCKeyPair& keypair,
                                     const uint8_t* message, size_t msg_len,
                                     const uint8_t* signature) noexcept = 0;
    
    // ECDH key agreement
    virtual core::Result<void> derive_shared_secret(const ECCKeyPair& our_keypair,
                                                    const uint8_t* their_public_key,
                                                    uint8_t* shared_secret_out) noexcept = 0;
    
    // AES-256-GCM operations
    virtual core::Result<size_t> encrypt_aes_gcm(const uint8_t* key,
                                                 const uint8_t* nonce,
                                                 const uint8_t* plaintext, size_t pt_len,
                                                 uint8_t* ciphertext_out,
                                                 uint8_t* tag_out) noexcept = 0;
    
    virtual core::Result<size_t> decrypt_aes_gcm(const uint8_t* key,
                                                 const uint8_t* nonce,
                                                 const uint8_t* ciphertext, size_t ct_len,
                                                 const uint8_t* tag,
                                                 uint8_t* plaintext_out) noexcept = 0;
    
    // Hash operations
    virtual core::Result<void> hash_sha256(const uint8_t* data, size_t length,
                                          uint8_t* hash_out) noexcept = 0;
    
    // Random number generation
    virtual core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept = 0;
};

class CryptoEngine : public ICryptoEngine {
public:
    explicit CryptoEngine(platform::IPlatformCrypto& platform_crypto) noexcept;
    ~CryptoEngine() override = default;
    
    core::Result<void> generate_keypair(ECCKeyPair& keypair) noexcept override;
    
    core::Result<void> sign(const ECCKeyPair& keypair,
                           const uint8_t* message, size_t msg_len,
                           uint8_t* signature_out) noexcept override;
    
    core::Result<bool> verify(const ECCKeyPair& keypair,
                             const uint8_t* message, size_t msg_len,
                             const uint8_t* signature) noexcept override;
    
    core::Result<void> derive_shared_secret(const ECCKeyPair& our_keypair,
                                           const uint8_t* their_public_key,
                                           uint8_t* shared_secret_out) noexcept override;
    
    core::Result<size_t> encrypt_aes_gcm(const uint8_t* key,
                                        const uint8_t* nonce,
                                        const uint8_t* plaintext, size_t pt_len,
                                        uint8_t* ciphertext_out,
                                        uint8_t* tag_out) noexcept override;
    
    core::Result<size_t> decrypt_aes_gcm(const uint8_t* key,
                                        const uint8_t* nonce,
                                        const uint8_t* ciphertext, size_t ct_len,
                                        const uint8_t* tag,
                                        uint8_t* plaintext_out) noexcept override;
    
    core::Result<void> hash_sha256(const uint8_t* data, size_t length,
                                  uint8_t* hash_out) noexcept override;
    
    core::Result<void> random_bytes(uint8_t* buffer, size_t length) noexcept override;
    
private:
    platform::IPlatformCrypto& platform_crypto_;
};

} // namespace gridshield::security