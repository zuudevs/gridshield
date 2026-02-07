/**
 * @file types.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Domain types for GridShield
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

namespace gridshield::core {

using timestamp_t = uint64_t;
using meter_id_t = uint64_t;
using sequence_t = uint32_t;

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

struct MeterReading {
    timestamp_t timestamp;
    uint32_t energy_wh;
    uint32_t voltage_mv;  // 220V = 220000 mV
    uint16_t current_ma;
    uint16_t power_factor; // 0-100 (scaled by 100)
    uint8_t phase;
    uint8_t reserved;
    
    MeterReading() : timestamp(0), energy_wh(0), voltage_mv(0),
                     current_ma(0), power_factor(0), phase(0), reserved(0) {}
};

struct TamperEvent {
    timestamp_t timestamp;
    uint8_t event_type;
    uint8_t severity;
    uint16_t sensor_id;
    uint32_t metadata;
    
    TamperEvent() : timestamp(0), event_type(0), severity(0),
                    sensor_id(0), metadata(0) {}
};

static_assert(sizeof(TamperEvent) == 16, "TamperEvent size mismatch");

// Fixed-size buffer for embedded systems
template<typename T, size_t N>
class StaticBuffer {
public:
    StaticBuffer() : size_(0) {}
    
    bool push(const T& item) {
        if (UNLIKELY(size_ >= N)) return false;
        data_[size_++] = item;
        return true;
    }
    
    bool pop(T& item) {
        if (UNLIKELY(size_ == 0)) return false;
        item = data_[--size_];
        return true;
    }
    
    void clear() { size_ = 0; }
    
    size_t size() const { return size_; }
    size_t capacity() const { return N; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    
    T& operator[](size_t idx) { return data_[idx]; }
    const T& operator[](size_t idx) const { return data_[idx]; }
    
    T* data() { return data_; }
    const T* data() const { return data_; }
    
private:
    T data_[N];
    size_t size_;
};

// Byte array with append operations
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
    
    bool append(const uint8_t* data, size_t len) {
        if (UNLIKELY(size_ + len > N)) return false;
        memcpy(data_ + size_, data, len);
        size_ += len;
        return true;
    }
    
    size_t size() const { return size_; }
    size_t capacity() const { return N; }
    
    uint8_t* data() { return data_; }
    const uint8_t* data() const { return data_; }
    
    uint8_t& operator[](size_t idx) { return data_[idx]; }
    const uint8_t& operator[](size_t idx) const { return data_[idx]; }
    
private:
    uint8_t data_[N];
    size_t size_;
};

} // namespace gridshield::core