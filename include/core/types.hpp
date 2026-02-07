/**
 * @file types.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Core domain types with optimized memory layout
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
    #include <cstring>
#else
    #include <stdint.h>
    #include <string.h>
#endif

namespace gridshield {
namespace core {

// ============================================================================
// TYPE ALIASES
// ============================================================================
using timestamp_t = uint64_t;
using meter_id_t = uint64_t;
using sequence_t = uint32_t;

// ============================================================================
// ENUMERATIONS
// ============================================================================
enum class SecurityLevel : uint8_t {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};

enum class SystemState : uint8_t {
    Uninitialized = 0,
    Initializing = 1,
    Ready = 2,
    Operating = 3,
    Tampered = 4,
    PowerLoss = 5,
    Error = 6,
    Shutdown = 7
};

enum class Priority : uint8_t {
    Lowest = 0,
    Low = 1,
    Normal = 2,
    High = 3,
    Critical = 4,
    Emergency = 5
};

// ============================================================================
// METER READING (24 bytes - cache aligned)
// ============================================================================
struct GS_ALIGN(8) MeterReading {
    timestamp_t timestamp;    // 8 bytes
    uint32_t energy_wh;       // 4 bytes
    uint32_t voltage_mv;      // 4 bytes (220V = 220000mV)
    uint16_t current_ma;      // 2 bytes
    uint16_t power_factor;    // 2 bytes (0-100 scaled)
    uint8_t phase;            // 1 byte
    uint8_t reserved[3];      // 3 bytes padding
    
    MeterReading() 
        : timestamp(0), energy_wh(0), voltage_mv(0),
          current_ma(0), power_factor(0), phase(0), reserved{0} {}
};
static_assert(sizeof(MeterReading) == 24, "MeterReading size must be 24 bytes");

// ============================================================================
// TAMPER EVENT (16 bytes - optimized)
// ============================================================================
struct GS_ALIGN(8) TamperEvent {
    timestamp_t timestamp;  // 8 bytes
    uint32_t metadata;      // 4 bytes
    uint16_t sensor_id;     // 2 bytes
    uint8_t event_type;     // 1 byte
    uint8_t severity;       // 1 byte
    
    TamperEvent() 
        : timestamp(0), metadata(0), sensor_id(0),
          event_type(0), severity(0) {}
};
static_assert(sizeof(TamperEvent) == 16, "TamperEvent size must be 16 bytes");

// ============================================================================
// STATIC BUFFER (fixed-size, no heap allocation)
// ============================================================================
template<typename T, size_t N>
class StaticBuffer {
public:
    StaticBuffer() : size_(0) {}
    
    GS_INLINE bool push(const T& item) {
        if (GS_UNLIKELY(size_ >= N)) return false;
        data_[size_++] = item;
        return true;
    }
    
    GS_INLINE bool pop(T& item) {
        if (GS_UNLIKELY(size_ == 0)) return false;
        item = ZMOVE(data_[--size_]);
        return true;
    }
    
    void clear() { size_ = 0; }
    
    constexpr size_t size() const { return size_; }
    constexpr size_t capacity() const { return N; }
    constexpr bool empty() const { return size_ == 0; }
    constexpr bool full() const { return size_ == N; }
    
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }
    
    T* data() { return data_; }
    const T* data() const { return data_; }
    
private:
    T data_[N];
    size_t size_;
};

// ============================================================================
// BYTE ARRAY (specialized for raw bytes)
// ============================================================================
template<size_t N>
class ByteArray {
public:
    ByteArray() : size_(0) {
        memset(data_, 0, N);
    }
    
    void clear() {
        size_ = 0;
        memset(data_, 0, N);
    }
    
    GS_INLINE bool append(const uint8_t* data, size_t len) {
        if (GS_UNLIKELY(size_ + len > N)) return false;
        memcpy(data_ + size_, data, len);
        size_ += len;
        return true;
    }
    
    constexpr size_t size() const { return size_; }
    constexpr size_t capacity() const { return N; }
    
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    
    uint8_t& operator[](size_t idx) { return data_[idx]; }
    const uint8_t& operator[](size_t idx) const { return data_[idx]; }
    
private:
    uint8_t data_[N];
    size_t size_;
};

} // namespace core
} // namespace gridshield