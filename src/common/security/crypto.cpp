/**
 * @file crypto.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Crypto engine implementation (C++17)
 * @version 0.6
 * @date 2026-02-09
 * 
 * Production Libraries:
 * - Arduino: micro-ecc (uECC) for ECDSA
 * - Native: OpenSSL for full crypto suite
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
// PRODUCTION LIBRARY INTEGRATION
// ============================================================================

#if GS_PLATFORM_ARDUINO
    // Uncomment when micro-ecc is installed
    // #include <uECC.h>
    // #define USE_UECC 1
#elif GS_PLATFORM_NATIVE
    // Uncomment when OpenSSL is installed
    // #include <openssl/ec.h>
    // #include <openssl/sha.h>
    // #include <openssl/rand.h>
    // #define USE_OPENSSL 1
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
#ifdef USE_UECC
    // Arduino: micro-ecc implementation
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    if (!uECC_make_key(public_key_, private_key_, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    has_private_ = true;
    has_public_ = true;
    return core::Result<void>();
    
#elif defined(USE_OPENSSL)
    // Native: OpenSSL implementation
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key || !EC_KEY_generate_key(key)) {
        EC_KEY_free(key);
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    // Extract private key
    const BIGNUM* priv_bn = EC_KEY_get0_private_key(key);
    BN_bn2bin(priv_bn, private_key_);
    
    // Extract public key
    const EC_POINT* pub_point = EC_KEY_get0_public_key(key);
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    EC_POINT_point2oct(group, pub_point, POINT_CONVERSION_UNCOMPRESSED,
                       public_key_, ECC_PUBLIC_KEY_SIZE, nullptr);
    
    EC_GROUP_free(group);
    EC_KEY_free(key);
    
    has_private_ = true;
    has_public_ = true;
    return core::Result<void>();
    
#else
    // Placeholder for testing only
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
    // Secure erase
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
    : platform_crypto_(platform_crypto) {}

core::Result<void> CryptoEngine::generate_keypair(ECCKeyPair& keypair) noexcept {
#if defined(USE_UECC) || defined(USE_OPENSSL)
    // Use production library
    return keypair.generate();
    
#else
    // Placeholder: Generate random keys for testing
    uint8_t private_key[ECC_KEY_SIZE];
    GS_TRY(platform_crypto_.random_bytes(private_key, ECC_KEY_SIZE));
    
    uint8_t public_key[ECC_PUBLIC_KEY_SIZE];
    GS_TRY(platform_crypto_.random_bytes(public_key, ECC_PUBLIC_KEY_SIZE));
    
    GS_TRY(keypair.load_private_key(private_key, ECC_KEY_SIZE));
    return keypair.load_public_key(public_key, ECC_PUBLIC_KEY_SIZE);
#endif
}

core::Result<void> CryptoEngine::sign(
    const ECCKeyPair& keypair,
    const uint8_t* message, size_t msg_len,
    uint8_t* signature_out) noexcept {
    
    if (GS_UNLIKELY(!keypair.has_private_key() || message == nullptr || 
                     signature_out == nullptr)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    // Hash message first
    uint8_t hash[SHA256_HASH_SIZE];
    GS_TRY(hash_sha256(message, msg_len, hash));
    
#ifdef USE_UECC
    // Arduino: micro-ecc ECDSA signing
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    if (!uECC_sign(keypair.get_private_key(), hash, SHA256_HASH_SIZE, 
                   signature_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    return core::Result<void>();
    
#elif defined(USE_OPENSSL)
    // Native: OpenSSL ECDSA signing
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    BIGNUM* priv_bn = BN_bin2bn(keypair.get_private_key(), ECC_KEY_SIZE, nullptr);
    EC_KEY_set_private_key(key, priv_bn);
    
    unsigned int sig_len = ECC_SIGNATURE_SIZE;
    int result = ECDSA_sign(0, hash, SHA256_HASH_SIZE, signature_out, &sig_len, key);
    
    BN_free(priv_bn);
    EC_KEY_free(key);
    
    if (result != 1) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    return core::Result<void>();
    
#else
    // Placeholder: Random signature for testing
    GS_TRY(platform_crypto_.random_bytes(signature_out, ECC_SIGNATURE_SIZE));
    return core::Result<void>();
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
    
#ifdef USE_UECC
    // Arduino: micro-ecc ECDSA verification
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    int valid = uECC_verify(keypair.get_public_key(), hash, SHA256_HASH_SIZE, 
                            signature, curve);
    return core::Result<bool>(valid != 0);
    
#elif defined(USE_OPENSSL)
    // Native: OpenSSL ECDSA verification
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    EC_POINT* pub_point = EC_POINT_new(group);
    
    EC_POINT_oct2point(group, pub_point, keypair.get_public_key(), 
                       ECC_PUBLIC_KEY_SIZE, nullptr);
    EC_KEY_set_public_key(key, pub_point);
    
    int valid = ECDSA_verify(0, hash, SHA256_HASH_SIZE, signature, 
                            ECC_SIGNATURE_SIZE, key);
    
    EC_POINT_free(pub_point);
    EC_GROUP_free(group);
    EC_KEY_free(key);
    
    return core::Result<bool>(valid == 1);
    
#else
    // Placeholder: Accept all for testing
    return core::Result<bool>(true);
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
    
#ifdef USE_UECC
    // Arduino: micro-ecc ECDH
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    if (!uECC_shared_secret(their_public_key, our_keypair.get_private_key(), 
                            shared_secret_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    return core::Result<void>();
    
#elif defined(USE_OPENSSL)
    // Native: OpenSSL ECDH
    EC_KEY* our_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    BIGNUM* priv_bn = BN_bin2bn(our_keypair.get_private_key(), ECC_KEY_SIZE, nullptr);
    EC_KEY_set_private_key(our_key, priv_bn);
    
    EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
    EC_POINT* their_point = EC_POINT_new(group);
    EC_POINT_oct2point(group, their_point, their_public_key, 
                       ECC_PUBLIC_KEY_SIZE, nullptr);
    
    int len = ECDH_compute_key(shared_secret_out, ECC_KEY_SIZE, their_point, 
                               our_key, nullptr);
    
    EC_POINT_free(their_point);
    EC_GROUP_free(group);
    BN_free(priv_bn);
    EC_KEY_free(our_key);
    
    if (len != static_cast<int>(ECC_KEY_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    return core::Result<void>();
    
#else
    // Placeholder: XOR for testing
    const uint8_t* our_private = our_keypair.get_private_key();
    for (size_t i = 0; i < ECC_KEY_SIZE; ++i) {
        shared_secret_out[i] = our_private[i] ^ their_public_key[i];
    }
    return core::Result<void>();
#endif
}

core::Result<size_t> CryptoEngine::encrypt_aes_gcm(
    const uint8_t* key,
    const uint8_t* nonce,
    const uint8_t* plaintext, size_t pt_len,
    uint8_t* ciphertext_out,
    uint8_t* tag_out) noexcept {
    
    if (GS_UNLIKELY(key == nullptr || nonce == nullptr || plaintext == nullptr || 
                     ciphertext_out == nullptr || tag_out == nullptr)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }
    
    // TODO: Implement AES-GCM with production library
    // Arduino: Use Crypto library AES
    // Native: Use OpenSSL EVP_aes_256_gcm
    
    // Placeholder: XOR cipher for testing
    for (size_t i = 0; i < pt_len; ++i) {
        ciphertext_out[i] = plaintext[i] ^ key[i % AES_KEY_SIZE];
    }
    
    auto result = platform_crypto_.random_bytes(tag_out, AES_GCM_TAG_SIZE);
    if (result.is_error()) {
        return core::Result<size_t>(result.error());
    }
    
    return core::Result<size_t>(pt_len);
}

core::Result<size_t> CryptoEngine::decrypt_aes_gcm(
    const uint8_t* key,
    const uint8_t* nonce,
    const uint8_t* ciphertext, size_t ct_len,
    const uint8_t* tag,
    uint8_t* plaintext_out) noexcept {
    
    if (GS_UNLIKELY(key == nullptr || nonce == nullptr || ciphertext == nullptr || 
                     tag == nullptr || plaintext_out == nullptr)) {
        return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }
    
    // TODO: Implement AES-GCM decryption with production library
    
    // Placeholder: XOR cipher for testing
    for (size_t i = 0; i < ct_len; ++i) {
        plaintext_out[i] = ciphertext[i] ^ key[i % AES_KEY_SIZE];
    }
    
    return core::Result<size_t>(ct_len);
}

core::Result<void> CryptoEngine::hash_sha256(
    const uint8_t* data, size_t length,
    uint8_t* hash_out) noexcept {
    
    if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
        return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    // Delegate to platform-specific SHA-256
    // Arduino: Uses Crypto library (already implemented in platform_arduino.hpp)
    // Native: Should use OpenSSL SHA256() function
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