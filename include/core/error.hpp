/**
 * @file error.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.2
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <cstdint>
#include <utility>
#include <new>

namespace gridshield::core {

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

class ErrorContext {
public:
    constexpr ErrorContext(ErrorCode code, uint32_t line = 0, const char* file = nullptr) noexcept
        : code_(code), line_(line), file_(file), timestamp_(0) {}
    
    constexpr ErrorCode code() const noexcept { return code_; }
    constexpr uint32_t line() const noexcept { return line_; }
    constexpr const char* file() const noexcept { return file_; }
    constexpr uint32_t timestamp() const noexcept { return timestamp_; }
    
    void set_timestamp(uint32_t ts) noexcept { timestamp_ = ts; }
    
    constexpr bool is_critical() const noexcept {
        return static_cast<uint16_t>(code_) >= 200 && 
               static_cast<uint16_t>(code_) < 400;
    }
    
private:
    ErrorCode code_;
    uint32_t line_;
    const char* file_;
    uint32_t timestamp_;
};

template<typename T>
class Result {
public:
    // Removed constexpr because placement new is not constexpr-friendly in C++11/14
    Result(T&& value) noexcept 
        : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(std::move(value));
    }
    
    Result(const T& value) noexcept 
        : has_value_(true), error_(ErrorCode::Success) {
        new (&storage_.value) T(value);
    }
    
    Result(ErrorContext error) noexcept 
        : has_value_(false), error_(error) {}
    
    ~Result() {
        if (has_value_) {
            storage_.value.~T();
        }
    }
    
    Result(const Result&) = delete;
    
    Result& operator=(const Result& other) {
        if (this != &other) {
            if (has_value_) {
                storage_.value.~T();
            }
            has_value_ = other.has_value_;
            error_ = other.error_;
            if (has_value_) {
                new (&storage_.value) T(other.storage_.value);
            }
        }
        return *this;
    }
    
    Result(Result&& other) noexcept : has_value_(other.has_value_), error_(other.error_) {
        if (has_value_) {
            new (&storage_.value) T(std::move(other.storage_.value));
        }
    }
    
    bool is_ok() const noexcept { return has_value_; }
    bool is_error() const noexcept { return !has_value_; }
    
    // Removed constexpr and overloads causing ambiguity in older compilers
    T& value() noexcept { return storage_.value; }
    const T& value() const noexcept { return storage_.value; }
    
    ErrorContext error() const noexcept { return error_; }
    
    T value_or(T&& default_value) const noexcept {
        return has_value_ ? storage_.value : std::move(default_value);
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

template<>
class Result<void> {
public:
    constexpr Result() noexcept : error_(ErrorCode::Success) {}
    constexpr Result(ErrorContext error) noexcept : error_(error) {}
    
    constexpr bool is_ok() const noexcept { 
        return error_.code() == ErrorCode::Success; 
    }
    constexpr bool is_error() const noexcept { return !is_ok(); }
    constexpr ErrorContext error() const noexcept { return error_; }
    
private:
    ErrorContext error_;
};

#define MAKE_ERROR(code) \
    ::gridshield::core::ErrorContext((code), __LINE__, __FILE__)

#define TRY(expr) \
    ({ \
        auto _result = (expr); \
        if (_result.is_error()) { \
            return _result.error(); \
        } \
        std::move(_result.value()); \
    })

} // namespace gridshield::core