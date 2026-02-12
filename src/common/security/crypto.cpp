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
// PRODUCTION LIBRARIES
// ============================================================================
#if GS_PLATFORM_ARDUINO
    // micro-ecc for ECDSA on Arduino
    #include <uECC.h>
    #define CRYPTO_ENABLED 1

	static gridshield::platform::IPlatformCrypto* g_crypto_ptr = nullptr;
    static int uecc_rng_adapter(uint8_t *dest, unsigned size) {
        if (g_crypto_ptr) {
            return g_crypto_ptr->random_bytes(dest, size).is_ok() ? 1 : 0;
        }
        return 0;
    }
    
#elif GS_PLATFORM_NATIVE
    // OpenSSL for full crypto on Native
    #include <openssl/ec.h>
    #include <openssl/ecdsa.h>
    #include <openssl/ecdh.h>
    #include <openssl/obj_mac.h>
    #include <openssl/evp.h>
    #define CRYPTO_ENABLED 1
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
#if GS_PLATFORM_ARDUINO && defined(CRYPTO_ENABLED)
    // Arduino: micro-ecc secp256r1
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_make_key(public_key_, private_key_, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    has_private_ = true;
    has_public_ = true;
    return core::Result<void>();
    
#elif GS_PLATFORM_NATIVE && defined(CRYPTO_ENABLED)
    // Native: OpenSSL EC_KEY
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) {
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    if (!EC_KEY_generate_key(key)) {
        EC_KEY_free(key);
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    // Extract private key
    const BIGNUM* priv_bn = EC_KEY_get0_private_key(key);
    int len = BN_bn2bin(priv_bn, private_key_);
    if (len != ECC_KEY_SIZE) {
        // Pad with zeros if needed
        if (len < static_cast<int>(ECC_KEY_SIZE)) {
            std::memmove(private_key_ + (ECC_KEY_SIZE - len), private_key_, len);
            std::memset(private_key_, 0, ECC_KEY_SIZE - len);
        }
    }
    
    // Extract public key (uncompressed: 0x04 || x || y)
    const EC_POINT* pub_point = EC_KEY_get0_public_key(key);
    const EC_GROUP* group = EC_KEY_get0_group(key);
    
    uint8_t pub_buf[65]; // 1 (prefix) + 64 (x+y)
    size_t pub_len = EC_POINT_point2oct(group, pub_point, 
        POINT_CONVERSION_UNCOMPRESSED, pub_buf, sizeof(pub_buf), nullptr);
    
    if (pub_len != 65) {
        EC_KEY_free(key);
        return GS_MAKE_ERROR(core::ErrorCode::KeyGenerationFailed);
    }
    
    // Skip 0x04 prefix, copy x and y
    std::memcpy(public_key_, pub_buf + 1, ECC_PUBLIC_KEY_SIZE);
    
    EC_KEY_free(key);
    
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
#if GS_PLATFORM_ARDUINO && defined(CRYPTO_ENABLED)
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
    
#if GS_PLATFORM_ARDUINO && defined(CRYPTO_ENABLED)
    // Arduino: micro-ecc ECDSA
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_sign(keypair.get_private_key(), hash, SHA256_HASH_SIZE, 
                   signature_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    
    return core::Result<void>();
    
#elif GS_PLATFORM_NATIVE && defined(CRYPTO_ENABLED)
    // Native: OpenSSL ECDSA
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    BIGNUM* priv_bn = BN_bin2bn(keypair.get_private_key(), ECC_KEY_SIZE, nullptr);
    if (!EC_KEY_set_private_key(key, priv_bn)) {
        BN_free(priv_bn);
        EC_KEY_free(key);
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    unsigned int sig_len = ECC_SIGNATURE_SIZE;
    int result = ECDSA_sign(0, hash, SHA256_HASH_SIZE, 
                            signature_out, &sig_len, key);
    
    BN_free(priv_bn);
    EC_KEY_free(key);
    
    if (result != 1) {
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
    
#if GS_PLATFORM_ARDUINO && defined(CRYPTO_ENABLED)
    // Arduino: micro-ecc verify
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    int valid = uECC_verify(keypair.get_public_key(), hash, SHA256_HASH_SIZE,
                            signature, curve);
    
    return core::Result<bool>(valid != 0);
    
#elif GS_PLATFORM_NATIVE && defined(CRYPTO_ENABLED)
    // Native: OpenSSL verify
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!key) {
        return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::CryptoFailure));
    }
    
    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* pub_point = EC_POINT_new(group);
    
    // Convert public key (x || y) to EC_POINT (with 0x04 prefix)
    uint8_t pub_buf[65];
    pub_buf[0] = 0x04;
    std::memcpy(pub_buf + 1, keypair.get_public_key(), ECC_PUBLIC_KEY_SIZE);
    
    if (!EC_POINT_oct2point(group, pub_point, pub_buf, 65, nullptr)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(key);
        return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::CryptoFailure));
    }
    
    if (!EC_KEY_set_public_key(key, pub_point)) {
        EC_POINT_free(pub_point);
        EC_KEY_free(key);
        return core::Result<bool>(GS_MAKE_ERROR(core::ErrorCode::CryptoFailure));
    }
    
    int valid = ECDSA_verify(0, hash, SHA256_HASH_SIZE, 
                            signature, ECC_SIGNATURE_SIZE, key);
    
    EC_POINT_free(pub_point);
    EC_KEY_free(key);
    
    return core::Result<bool>(valid == 1);
    
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
    
#if GS_PLATFORM_ARDUINO && defined(CRYPTO_ENABLED)
    // Arduino: micro-ecc ECDH
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    
    if (!uECC_shared_secret(their_public_key, our_keypair.get_private_key(),
                            shared_secret_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    return core::Result<void>();
    
#elif GS_PLATFORM_NATIVE && defined(CRYPTO_ENABLED)
    // Native: OpenSSL ECDH
    EC_KEY* our_key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    if (!our_key) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    BIGNUM* priv_bn = BN_bin2bn(our_keypair.get_private_key(), ECC_KEY_SIZE, nullptr);
    EC_KEY_set_private_key(our_key, priv_bn);
    
    const EC_GROUP* group = EC_KEY_get0_group(our_key);
    EC_POINT* their_point = EC_POINT_new(group);
    
    uint8_t pub_buf[65];
    pub_buf[0] = 0x04;
    std::memcpy(pub_buf + 1, their_public_key, ECC_PUBLIC_KEY_SIZE);
    
    EC_POINT_oct2point(group, their_point, pub_buf, 65, nullptr);
    
    int len = ECDH_compute_key(shared_secret_out, ECC_KEY_SIZE, 
                               their_point, our_key, nullptr);
    
    EC_POINT_free(their_point);
    BN_free(priv_bn);
    EC_KEY_free(our_key);
    
    if (len != static_cast<int>(ECC_KEY_SIZE)) {
        return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }
    
    return core::Result<void>();
    
#else
    return GS_MAKE_ERROR(core::ErrorCode::NotImplemented);
#endif
}

core::Result<size_t> CryptoEngine::encrypt_aes_gcm(
    const uint8_t* /*key*/,
    const uint8_t* /*nonce*/,
    const uint8_t* /*plaintext*/, size_t /*pt_len*/,
    uint8_t* /*ciphertext_out*/,
    uint8_t* /*tag_out*/) noexcept {
    
    // TODO: Implement AES-GCM
    // Arduino: Use Crypto library GCM
    // Native: Use OpenSSL EVP_aes_256_gcm
    
    return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NotImplemented));
}

core::Result<size_t> CryptoEngine::decrypt_aes_gcm(
    const uint8_t* /*key*/,
    const uint8_t* /*nonce*/,
    const uint8_t* /*ciphertext*/, size_t /*ct_len*/,
    const uint8_t* /*tag*/,
    uint8_t* /*plaintext_out*/) noexcept {
    
    return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::NotImplemented));
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