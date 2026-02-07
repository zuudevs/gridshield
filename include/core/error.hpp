/**
 * @file error.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Type-safe error handling without exceptions (C++17)
 * @version 0.3
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "utils/gs_macros.hpp"

#if PLATFORM_NATIVE
    #include <cstdint>
    #include <new>
#else
    #include <stdint.h>
    // NEED ADOPTION: Ensure <new.h> or placement new is available
    #include <new.h>
#endif

namespace gridshield {
namespace core {

// ============================================================================
// ERROR CODES
// ============================================================================
enum class ErrorCode : uint16_t {
    Success = 0,
    
    // System errors (100-199)
    SystemNotInitialized = 100,
    SystemAlreadyInitialized = 101,
    SystemShutdown = 102,
    InvalidState = 103,
    ResourceExhausted = 104,
    
    // Hardware errors (200-299)
    HardwareFailure = 200,
    SensorReadFailure = 201,
    SensorNotCalibrated = 202,
    TamperDetected = 203,
    PowerLossDetected = 204,
    
    // Security errors (300-399)
    CryptoFailure = 300,
    AuthenticationFailed = 301,
    IntegrityViolation = 302,
    KeyGenerationFailed = 303,
    SignatureInvalid = 304,
    EncryptionFailed = 305,
    DecryptionFailed = 306,
    
    // Network errors (400-499)
    NetworkTimeout = 400,
    NetworkDisconnected = 401,
    TransmissionFailed = 402,
    InvalidPacket = 403,
    BufferOverflow = 404,
    
    // Analytics errors (500-599)
    AnomalyDetected = 500,
    ProfileMismatch = 501,
    ThresholdExceeded = 502,
    DataInvalid = 503,
    
    // Configuration errors (600-699)
    InvalidParameter = 600,
    ConfigurationError = 601,
    CalibrationRequired = 602,
    
    // Generic errors (900-999)
    Unknown = 900,
    NotImplemented = 901,
    NotSupported = 902
};

// ============================================================================
// ERROR CONTEXT
// ============================================================================
struct ErrorContext {
    ErrorCode code;
    uint32_t line;
    const char* file;
    uint32_t timestamp;
    
    constexpr ErrorContext(ErrorCode c, uint32_t ln = 0, const char* f = nullptr) 
        : code(c), line(ln), file(f), timestamp(0) {}
    
    constexpr bool is_critical() const {
        uint16_t code_val = static_cast<uint16_t>(code);
        return code_val >= 200 && code_val < 400;
    }
};

// ============================================================================
// RESULT<T> MONAD
// ============================================================================
template<typename T>
class Result {
public:
    // Success constructors
    explicit Result(const T& val) : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(val);
    }
    
    explicit Result(T&& val) : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(ZMOVE(val));
    }
    
    // Error constructor
    explicit Result(ErrorContext err) : has_value_(false), error_(err) {}
    
    // Destructor
    ~Result() {
        if (has_value_) {
            storage_.value.~T();
        }
    }
    
    // Delete copy (force explicit handling)
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    
    // Move constructor
    Result(Result&& other) noexcept 
        : has_value_(other.has_value_), error_(other.error_) {
        if (has_value_) {
            new (&storage_.value) T(ZMOVE(other.storage_.value));
        }
    }
    
    // Move assignment
    Result& operator=(Result&& other) noexcept {
        if (this != &other) {
            if (has_value_) {
                storage_.value.~T();
            }
            has_value_ = other.has_value_;
            error_ = other.error_;
            if (has_value_) {
                new (&storage_.value) T(ZMOVE(other.storage_.value));
            }
        }
        return *this;
    }
    
    // Status check
    constexpr bool is_ok() const { return has_value_; }
    constexpr bool is_error() const { return !has_value_; }
    
    // Value access (unsafe - must check is_ok first)
    T& value() { return storage_.value; }
    const T& value() const { return storage_.value; }
    
    // Safe value access with default
    T value_or(const T& default_val) const {
        return has_value_ ? storage_.value : default_val;
    }
    
    // Error access
    ErrorContext error() const { return error_; }
    
private:
    union Storage {
        T value;
        Storage() {} // Empty constructor
        ~Storage() {} // Empty destructor
    } storage_;
    
    bool has_value_;
    ErrorContext error_;
};

// ============================================================================
// RESULT<void> SPECIALIZATION
// ============================================================================
template<>
class Result<void> {
public:
    Result() : error_(ErrorCode::Success) {}
    explicit Result(ErrorContext err) : error_(err) {}
    
    constexpr bool is_ok() const { 
        return error_.code == ErrorCode::Success; 
    }
    constexpr bool is_error() const { return !is_ok(); }
    ErrorContext error() const { return error_; }
    
private:
    ErrorContext error_;
};

// ============================================================================
// ERROR HANDLING MACROS
// ============================================================================
#define MAKE_ERROR(code) \
    ::gridshield::core::ErrorContext((code), __LINE__, __FILE__)

// Early return on error
#define TRY(expr) \
    do { \
        auto _gs_result = (expr); \
        if (GS_UNLIKELY(_gs_result.is_error())) { \
            return _gs_result.error(); \
        } \
    } while(0)

// Assign and early return on error
#define TRY_ASSIGN(var, expr) \
    do { \
        auto _gs_result = (expr); \
        if (GS_UNLIKELY(_gs_result.is_error())) { \
            return _gs_result.error(); \
        } \
        var = ZMOVE(_gs_result.value()); \
    } while(0)

} // namespace core
} // namespace gridshield