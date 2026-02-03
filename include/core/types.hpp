/**
 * @file types.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include <cstdint>
#include <cstring>

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
    uint16_t voltage_mv;
    uint16_t current_ma;
    uint16_t power_factor;
    uint8_t phase;
    uint8_t reserved;
    
    constexpr MeterReading() noexcept 
        : timestamp(0), energy_wh(0), voltage_mv(0), 
          current_ma(0), power_factor(0), phase(0), reserved(0) {}
};

static_assert(sizeof(MeterReading) == 24, "MeterReading must be 24 bytes");

struct TamperEvent {
    timestamp_t timestamp;
    uint8_t event_type;
    uint8_t severity;
    uint16_t sensor_id;
    uint32_t metadata;
    
    constexpr TamperEvent() noexcept 
        : timestamp(0), event_type(0), severity(0), 
          sensor_id(0), metadata(0) {}
};

static_assert(sizeof(TamperEvent) == 16, "TamperEvent must be 16 bytes");

template<typename T, size_t N>
class StaticBuffer {
public:
    constexpr StaticBuffer() noexcept : size_(0) {}
    
    constexpr bool push(const T& item) noexcept {
        if (size_ >= N) return false;
        data_[size_++] = item;
        return true;
    }
    
    constexpr bool pop(T& item) noexcept {
        if (size_ == 0) return false;
        item = data_[--size_];
        return true;
    }
    
    constexpr void clear() noexcept { size_ = 0; }
    constexpr size_t size() const noexcept { return size_; }
    constexpr size_t capacity() const noexcept { return N; }
    constexpr bool empty() const noexcept { return size_ == 0; }
    constexpr bool full() const noexcept { return size_ == N; }
    
    constexpr T& operator[](size_t idx) noexcept { return data_[idx]; }
    constexpr const T& operator[](size_t idx) const noexcept { return data_[idx]; }
    
    constexpr T* data() noexcept { return data_; }
    constexpr const T* data() const noexcept { return data_; }
    
private:
    T data_[N];
    size_t size_;
};

template<size_t N>
class ByteArray {
public:
    constexpr ByteArray() noexcept : size_(0) {
        for (size_t i = 0; i < N; ++i) data_[i] = 0;
    }
    
    constexpr void clear() noexcept { 
        size_ = 0;
        for (size_t i = 0; i < N; ++i) data_[i] = 0;
    }
    
    constexpr bool append(const uint8_t* data, size_t len) noexcept {
        if (size_ + len > N) return false;
        for (size_t i = 0; i < len; ++i) {
            data_[size_++] = data[i];
        }
        return true;
    }
    
    constexpr size_t size() const noexcept { return size_; }
    constexpr size_t capacity() const noexcept { return N; }
    
    constexpr uint8_t* data() noexcept { return data_; }
    constexpr const uint8_t* data() const noexcept { return data_; }
    
    constexpr uint8_t& operator[](size_t idx) noexcept { return data_[idx]; }
    constexpr const uint8_t& operator[](size_t idx) const noexcept { return data_[idx]; }
    
private:
    uint8_t data_[N];
    size_t size_;
};

} // namespace gridshield::core