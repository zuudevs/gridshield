/**
 * @file crypto.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief PRODUCTION crypto implementation with micro-ecc and OpenSSL
 * @version 1.0
 * @date 2026-02-11
 * 
 * Libraries:
 * - Arduino: micro-ecc (uECC) - install in libs/micro-ecc/
 * - Native: OpenSSL 3.6.1
 * 
 * @copyright Copyright (c) 2026
 */

#include "security/crypto.hpp"

#if GS_PLATFORM_NATIVE
    #include <cstring>
#else
    #include <string.h>
#endif

// ============================================================================
// PRODUCTION LIBRARIES (Standardized on embedded libs for portability)
// ============================================================================
#if defined(GS_PLATFORM_ARDUINO) || defined(GS_PLATFORM_NATIVE)
    // micro-ecc for ECDSA
    #include <uECC.h>
    // Crypto library for AES-256-GCM
    #include <AES.h>
    #include <GCM.h>
    #define USE_EMBEDDED_CRYPTO 1

    static gridshield::platform::IPlatformCrypto* g_crypto_ptr = nullptr;
    static int uecc_rng_adapter(uint8_t *dest, unsigned size) {
        if (g_crypto_ptr) {
            return g_crypto_ptr->random_bytes(dest, size).is_ok() ? 1 : 0;
        }
        return 0;
    }
#endif

namespace gridshield {
namespace security {

// ============================================================================
// ECC KEY PAIR
// ============================================================================
ECCKeyPair::ECCKeyPair() noexcept 
    : has_private_(false), has_public_(false) {
    clear();
}

ECCKeyPair::~ECCKeyPair() noexcept {
    clear();
}

ECCKeyPair::ECCKeyPair(ECCKeyPair&& other) noexcept
    : has_private_(other.has_private_), has_public_(other.has_public_) {
#if GS_PLATFORM_NATIVE
    std::memcpy(private_key_, other.private_key_, ECC_KEY_SIZE);
    std::memcpy(public_key_, other.public_key_, ECC_PUBLIC_KEY_SIZE);
#else
    memcpy(private_key_, other.private_key_, ECC_KEY_SIZE);
    memcpy(public_key_, other.public_key_, ECC_PUBLIC_KEY_SIZE);
#endif
    other.clear();
}

ECCKeyPair& ECCKeyPair::operator=(ECCKeyPair&& other) noexcept {
    if (this != &other) {
        clear();
#if GS_PLATFORM_NATIVE
        std::memcpy(private_key_, other.private_key_, ECC_KEY_SIZE);
        std::memcpy(public_key_, other.public_key_, ECC_PUBLIC_KEY_SIZE);
#else
        memcpy(private_key_, other.private_key_, ECC_KEY_SIZE);
        memcpy(public_key_, other.public_key_, ECC_PUBLIC_KEY_SIZE);
#endif
        has_private_ = other.has_private_;
        has_public_ = other.has_public_;
        other.clear();
    }
    return *this;
}

core::Result<void> ECCKeyPair::generate() noexcept {
#if defined(USE_EMBEDDED_CRYPTO)
    // micro-ecc secp256r1
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_make_key(public_key_, private_key_, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    has_private_ = true;
    has_public_ = true;
    return core::Result<void>();
    
#else
    return GS_MAKE_ERROR(core::ErrorCode::NotImplemented);
#endif
}

core::Result<void> ECCKeyPair::load_private_key(const uint8_t* key, size_t length) noexcept {
    if (GS_UNLIKELY(key == nullptr || length != ECC_KEY_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
#if GS_PLATFORM_NATIVE
    std::memcpy(private_key_, key, ECC_KEY_SIZE);
#else
    memcpy(private_key_, key, ECC_KEY_SIZE);
#endif
    has_private_ = true;
    return core::Result<void>();
}

core::Result<void> ECCKeyPair::load_public_key(const uint8_t* key, size_t length) noexcept {
    if (GS_UNLIKELY(key == nullptr || length != ECC_PUBLIC_KEY_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
#if GS_PLATFORM_NATIVE
    std::memcpy(public_key_, key, ECC_PUBLIC_KEY_SIZE);
#else
    memcpy(public_key_, key, ECC_PUBLIC_KEY_SIZE);
#endif
    has_public_ = true;
    return core::Result<void>();
}

const uint8_t* ECCKeyPair::get_private_key() const noexcept {
    return has_private_ ? private_key_ : nullptr;
}

const uint8_t* ECCKeyPair::get_public_key() const noexcept {
    return has_public_ ? public_key_ : nullptr;
}

bool ECCKeyPair::has_private_key() const noexcept {
    return has_private_;
}

bool ECCKeyPair::has_public_key() const noexcept {
    return has_public_;
}

void ECCKeyPair::clear() noexcept {
    volatile uint8_t* vptr = private_key_;
    for (size_t i = 0; i < ECC_KEY_SIZE; ++i) {
        vptr[i] = 0;
    }
    
    vptr = public_key_;
    for (size_t i = 0; i < ECC_PUBLIC_KEY_SIZE; ++i) {
        vptr[i] = 0;
    }
    
    has_private_ = false;
    has_public_ = false;
}

// ============================================================================
// CRYPTO ENGINE
// ============================================================================
CryptoEngine::CryptoEngine(platform::IPlatformCrypto& platform_crypto) noexcept
    : platform_crypto_(platform_crypto) {
#if defined(USE_EMBEDDED_CRYPTO)
    g_crypto_ptr = &platform_crypto_;
    uECC_set_rng(uecc_rng_adapter);
#endif
	}

core::Result<void> CryptoEngine::generate_keypair(ECCKeyPair& keypair) noexcept {
    return keypair.generate();
}

core::Result<void> CryptoEngine::sign(
    const ECCKeyPair& keypair,
    const uint8_t* message, size_t msg_len,
    uint8_t* signature_out) noexcept {
    
    if (GS_UNLIKELY(!keypair.has_private_key() || message == nullptr || 
                     signature_out == nullptr)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    // Hash message
    uint8_t hash[SHA256_HASH_SIZE];
    GS_TRY(hash_sha256(message, msg_len, hash));
    
#if defined(USE_EMBEDDED_CRYPTO)
    // micro-ecc ECDSA
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_sign(keypair.get_private_key(), hash, SHA256_HASH_SIZE, 
                   signature_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    
    return core::Result<void>();

#else
    return GS_MAKE_ERROR(core::ErrorCode::NotImplemented);
#endif
}

core::Result<bool> CryptoEngine::verify(
    const ECCKeyPair& keypair,
    const uint8_t* message, size_t msg_len,
    const uint8_t* signature) noexcept {
    
    if (GS_UNLIKELY(!keypair.has_public_key() || message == nullptr || 
                     signature == nullptr)) {
        return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }
    
    uint8_t hash[SHA256_HASH_SIZE];
    auto result = hash_sha256(message, msg_len, hash);
    if (result.is_error()) {
        return core::Result<bool>(result.error());
    }
    
#if defined(USE_EMBEDDED_CRYPTO)
    // micro-ecc verify
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    int valid = uECC_verify(keypair.get_public_key(), hash, SHA256_HASH_SIZE,
                            signature, curve);
    
    return core::Result<bool>(valid != 0);

#else
    return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::NotImplemented));
#endif
}

core::Result<void> CryptoEngine::derive_shared_secret(
    const ECCKeyPair& our_keypair,
    const uint8_t* their_public_key,
    uint8_t* shared_secret_out) noexcept {
    
    if (GS_UNLIKELY(!our_keypair.has_private_key() || their_public_key == nullptr || 
                     shared_secret_out == nullptr)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
#if defined(USE_EMBEDDED_CRYPTO)
    // micro-ecc ECDH
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_shared_secret(their_public_key, our_keypair.get_private_key(),
                            shared_secret_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    return core::Result<void>();

#else
    return GS_MAKE_ERROR(core::ErrorCode::NotImplemented);
#endif
}

core::Result<size_t> CryptoEngine::encrypt_aes_gcm(
    const uint8_t* key,
    const uint8_t* nonce,
    const uint8_t* plaintext, size_t pt_len,
    uint8_t* ciphertext_out,
    uint8_t* tag_out) noexcept {
    
    if (GS_UNLIKELY(key == nullptr || nonce == nullptr || 
                    plaintext == nullptr || ciphertext_out == nullptr ||
                    tag_out == nullptr || pt_len == 0)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }
    
#if defined(USE_EMBEDDED_CRYPTO)
    // Use Crypto library GCM<AES256>
    GCM<AES256> gcm;
    gcm.setKey(key, AES_KEY_SIZE);
    gcm.setIV(nonce, NONCE_SIZE);
    gcm.encrypt(ciphertext_out, plaintext, pt_len);
    gcm.computeTag(tag_out, AES_GCM_TAG_SIZE);
    
    return core::Result<size_t>(pt_len);

#else
    return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NotImplemented));
#endif
}

core::Result<size_t> CryptoEngine::decrypt_aes_gcm(
    const uint8_t* key,
    const uint8_t* nonce,
    const uint8_t* ciphertext, size_t ct_len,
    const uint8_t* tag,
    uint8_t* plaintext_out) noexcept {
    
    if (GS_UNLIKELY(key == nullptr || nonce == nullptr || 
                    ciphertext == nullptr || tag == nullptr ||
                    plaintext_out == nullptr || ct_len == 0)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }
    
#if defined(USE_EMBEDDED_CRYPTO)
    // Use Crypto library GCM<AES256>
    GCM<AES256> gcm;
    gcm.setKey(key, AES_KEY_SIZE);
    gcm.setIV(nonce, NONCE_SIZE);
    gcm.decrypt(plaintext_out, ciphertext, ct_len);
    
    // Verify authentication tag
    if (!gcm.checkTag(tag, AES_GCM_TAG_SIZE)) {
        // Tag verification failed — data may be tampered
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation));
    }
    
    return core::Result<size_t>(ct_len);

#else
    return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NotImplemented));
#endif
}

core::Result<void> CryptoEngine::hash_sha256(
    const uint8_t* data, size_t length,
    uint8_t* hash_out) noexcept {
    
    if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    return platform_crypto_.sha256(data, length, hash_out);
}

core::Result<void> CryptoEngine::random_bytes(uint8_t* buffer, size_t length) noexcept {
    if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    return platform_crypto_.random_bytes(buffer, length);
}

} // namespace security
} // namespace gridshield