/**
 * @file error.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Type-safe error handling without exceptions (C++17)
 * @version 0.7
 * @date 2026-02-09
 * 
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "utils/gs_macros.hpp"

#if GS_PLATFORM_NATIVE
    #include <cstdint>
    #include <new>
    #include <type_traits>
#else
    #include <stdint.h>
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
    
    constexpr ErrorContext(ErrorCode c, uint32_t ln = 0, const char* f = nullptr) noexcept
        : code(c), line(ln), file(f) {}
    
    constexpr bool is_critical() const noexcept {
        uint16_t code_val = static_cast<uint16_t>(code);
        return code_val >= 200 && code_val < 400;
    }
};

// ============================================================================
// RESULT<T> MONAD (C++17 compliant)
// ============================================================================
template<typename T>
class Result {
public:
    // Success constructors
    explicit Result(const T& val) noexcept
        : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(val);
    }
    
    explicit Result(T&& val) noexcept
        : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(GS_MOVE(val));
    }
    
    // Error constructor
    /*implicit*/ Result(ErrorContext err) noexcept
        : has_value_(false), error_(err) {}
    
    // Destructor
    ~Result() noexcept {
        if (has_value_) {
            storage_.value.~T();
        }
    }
    
    // Delete copy
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;
    
    // Move constructor
    Result(Result&& other) noexcept
        : has_value_(other.has_value_), error_(other.error_) {
        if (has_value_) {
            new (&storage_.value) T(GS_MOVE(other.storage_.value));
            other.has_value_ = false;
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
                new (&storage_.value) T(GS_MOVE(other.storage_.value));
                other.has_value_ = false;
            }
        }
        return *this;
    }
    
    // Status check
    GS_NODISCARD constexpr bool is_ok() const noexcept { return has_value_; }
    GS_NODISCARD constexpr bool is_error() const noexcept { return !has_value_; }
    
    // Value access (unsafe - check is_ok first)
    GS_NODISCARD T& value() noexcept { 
        GS_ASSERT(has_value_);
        return storage_.value; 
    }
    
    GS_NODISCARD const T& value() const noexcept { 
        GS_ASSERT(has_value_);
        return storage_.value; 
    }
    
    // Safe value access
    GS_NODISCARD T value_or(const T& default_val) const noexcept {
        return has_value_ ? storage_.value : default_val;
    }
    
    // Error access
    GS_NODISCARD ErrorContext error() const noexcept { return error_; }
    
    // Convert to Result<void>, discarding the value
    Result<void> as_void() const noexcept {
        if (has_value_) return Result<void>();
        return error_;
    }
    
private:
    union Storage {
        T value;
        Storage() {}
        ~Storage() {}
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
    Result() noexcept : error_(ErrorCode::Success) {}
    /*implicit*/ Result(ErrorContext err) noexcept : error_(err) {}
    
    GS_NODISCARD constexpr bool is_ok() const noexcept { 
        return error_.code == ErrorCode::Success; 
    }
    GS_NODISCARD constexpr bool is_error() const noexcept { 
        return !is_ok(); 
    }
    GS_NODISCARD ErrorContext error() const noexcept { return error_; }
    
private:
    ErrorContext error_;
};

// ============================================================================
// ERROR MACROS
// ============================================================================
#define GS_MAKE_ERROR(code) \
    ::gridshield::core::ErrorContext((code), __LINE__, __FILE__)

#define GS_TRY(expr) \
    do { \
        auto gs_result_ = (expr); \
        if (GS_UNLIKELY(gs_result_.is_error())) { \
            return gs_result_.error(); \
        } \
    } while(0)

#define GS_TRY_ASSIGN(var, expr) \
    do { \
        auto gs_result_ = (expr); \
        if (GS_UNLIKELY(gs_result_.is_error())) { \
            return gs_result_.error(); \
        } \
        var = GS_MOVE(gs_result_.value()); \
    } while(0)

} // namespace core
} // namespace gridshield