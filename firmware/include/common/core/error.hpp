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
#include <type_traits>
#else
#include <stdint.h>
#endif

#include <new>

namespace gridshield::core {

// ============================================================================
// ERROR CODES
// ============================================================================
enum class ErrorCode : uint16_t
{
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
    MqttConnectionFailed = 405,
    MqttPublishFailed = 406,
    MqttSubscribeFailed = 407,
    WiFiConnectionFailed = 408,
    WiFiNotConnected = 409,
    ModbusError = 410,
    LoRaError = 411,

    // Analytics errors (500-599)
    AnomalyDetected = 500,
    ProfileMismatch = 501,
    ThresholdExceeded = 502,
    DataInvalid = 503,

    // Configuration errors (600-699)
    InvalidParameter = 600,
    ConfigurationError = 601,
    CalibrationRequired = 602,

    // Sensor errors (700-799)
    SensorTimeout = 700,
    SensorNotFound = 701,
    SensorCalibrationError = 702,
    ADCReadError = 703,
    I2CError = 704,
    OneWireError = 705,
    UARTError = 706,

    // OTA errors (800-809)
    OtaFailed = 800,
    OtaValidationFailed = 801,
    OtaRollbackFailed = 802,

    // Cloud errors (810-819)
    CloudConnectionFailed = 810,
    CloudPublishFailed = 811,
    CloudSubscribeFailed = 812,
    CloudProvisioningFailed = 813,
    CloudAuthenticationFailed = 814,
    CloudPayloadTooLarge = 815,

    // SNTP errors (820-829)
    SntpSyncFailed = 820,
    SntpTimeout = 821,
    SntpNotSynced = 822,

    // Mesh / ESP-NOW errors (825-829)
    MeshPeerFull = 825,
    MeshSendFailed = 826,
    MeshInvalidPeer = 827,

    // Certificate errors (830-839)
    CertificateExpired = 830,
    CertificateInvalid = 831,
    CertificateNotFound = 832,

    // Memory pool errors (840-849)
    PoolExhausted = 840,
    PoolDoubleFree = 841,

    // TFLite / ML errors (850-859)
    ModelLoadFailed = 850,
    InferenceFailed = 851,
    TensorMismatch = 852,

    // Generic errors (950-999)
    Unknown = 950,
    NotImplemented = 951,
    NotSupported = 952
};

// ============================================================================
// ERROR CONTEXT
// ============================================================================
struct ErrorContext
{
    ErrorCode code;
    uint32_t line;
    const char* file;

    constexpr ErrorContext(ErrorCode errc_, uint32_t ln_ = 0, const char* file_ = nullptr) noexcept
        : code(errc_), line(ln_), file(file_)
    {}

    [[nodiscard]] constexpr bool is_critical() const noexcept
    {
        auto code_val = static_cast<uint16_t>(code);
        return code_val >= static_cast<uint16_t>(ErrorCode::HardwareFailure) &&
               code_val < static_cast<uint16_t>(ErrorCode::NetworkTimeout);
    }
};

// ============================================================================
// RESULT<T> MONAD (C++17 compliant)
// ============================================================================

// Forward declaration
template <typename T> class Result;

// ============================================================================
// RESULT<void> SPECIALIZATION (defined first so generic template can use it)
// ============================================================================
template <> class Result<void>
{
public:
    Result() noexcept : error_(ErrorCode::Success)
    {}
    /*implicit*/ Result(ErrorContext err) noexcept : error_(err)
    {}

    GS_NODISCARD constexpr bool is_ok() const noexcept
    {
        return error_.code == ErrorCode::Success;
    }
    GS_NODISCARD constexpr bool is_error() const noexcept
    {
        return !is_ok();
    }
    GS_NODISCARD ErrorContext error() const noexcept
    {
        return error_;
    }

private:
    ErrorContext error_;
};

// ============================================================================
// GENERIC RESULT<T> TEMPLATE
// ============================================================================
template <typename T> class Result
{
public:
    // Success constructors
    explicit Result(const T& val) noexcept : has_value_(true), error_(ErrorCode::Success)
    {
        new (&storage_.value) T(val);
    }

    explicit Result(T&& val) noexcept : has_value_(true), error_(ErrorCode::Success)
    {
        new (&storage_.value) T(GS_MOVE(val));
    }

    // Error constructor
    /*implicit*/ Result(ErrorContext err) noexcept : has_value_(false), error_(err)
    {}

    // Destructor
    ~Result() noexcept
    {
        if (has_value_) {
            storage_.value.~T();
        }
    }

    // Delete copy
    Result(const Result&) = delete;
    Result& operator=(const Result&) = delete;

    // Move constructor
    Result(Result&& other) noexcept : has_value_(other.has_value_), error_(other.error_)
    {
        if (has_value_) {
            new (&storage_.value) T(GS_MOVE(other.storage_.value));
            other.has_value_ = false;
        }
    }

    // Move assignment
    Result& operator=(Result&& other) noexcept
    {
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
    GS_NODISCARD constexpr bool is_ok() const noexcept
    {
        return has_value_;
    }
    GS_NODISCARD constexpr bool is_error() const noexcept
    {
        return !has_value_;
    }

    // Value access (unsafe - check is_ok first)
    GS_NODISCARD T& value() noexcept
    {
        GS_ASSERT(has_value_);
        return storage_.value;
    }

    GS_NODISCARD const T& value() const noexcept
    {
        GS_ASSERT(has_value_);
        return storage_.value;
    }

    // Safe value access
    GS_NODISCARD T value_or(const T& default_val) const noexcept
    {
        return has_value_ ? storage_.value : default_val;
    }

    // Error access
    GS_NODISCARD ErrorContext error() const noexcept
    {
        return error_;
    }

    // Convert to Result<void>, discarding the value
    Result<void> as_void() noexcept
    {
        if (has_value_) {
            return Result<void>{};
        }
        return error_;
    }

private:
    union Storage {
        T value;
        Storage()
        {}
        ~Storage()
        {}
    } storage_;

    bool has_value_;
    ErrorContext error_;
};

// ============================================================================
// ERROR MACROS
// ============================================================================
#define GS_MAKE_ERROR(code) ::gridshield::core::ErrorContext((code), __LINE__, __FILE__)

#define GS_TRY(expr)                                                                               \
    do {                                                                                           \
        auto gs_result_ = (expr);                                                                  \
        if (GS_UNLIKELY(gs_result_.is_error())) {                                                  \
            return gs_result_.error();                                                             \
        }                                                                                          \
    } while (0)

#define GS_TRY_ASSIGN(var, expr)                                                                   \
    do {                                                                                           \
        auto gs_result_ = (expr);                                                                  \
        if (GS_UNLIKELY(gs_result_.is_error())) {                                                  \
            return gs_result_.error();                                                             \
        }                                                                                          \
        (var) = GS_MOVE(gs_result_.value());                                                       \
    } while (0)

} // namespace gridshield::core