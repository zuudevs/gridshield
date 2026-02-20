/**
 * @file types.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Core domain types with optimized memory layout (C++17)
 * @version 0.6
 * @date 2026-02-09
 * 
 * @copyright Copyright (c) 2026
 *
 */

#pragma once

#include "utils/gs_macros.hpp"

#if GS_PLATFORM_NATIVE
    #include <cstdint>
    #include <cstring>
    #include <type_traits>
    #include <new> // Required for placement new
#else
    #include <stdint.h>
    #include <string.h>
    #include <new.h> // Arduino placement new
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
    uint32_t voltage_mv;      // 4 bytes
    uint16_t current_ma;      // 2 bytes
    uint16_t power_factor;    // 2 bytes (0-1000 scaled by 10)
    uint8_t phase;            // 1 byte
    uint8_t reserved[3];      // 3 bytes padding
    
    constexpr MeterReading() noexcept
        : timestamp(0), energy_wh(0), voltage_mv(0),
          current_ma(0), power_factor(0), phase(0), reserved{0} {}
};
GS_STATIC_ASSERT(sizeof(MeterReading) == 24, "MeterReading must be 24 bytes");

// ============================================================================
// TAMPER EVENT (16 bytes)
// ============================================================================
struct GS_ALIGN(8) TamperEvent {
    timestamp_t timestamp;  // 8 bytes
    uint32_t metadata;      // 4 bytes
    uint16_t sensor_id;     // 2 bytes
    uint8_t event_type;     // 1 byte
    uint8_t severity;       // 1 byte
    
    constexpr TamperEvent() noexcept
        : timestamp(0), metadata(0), sensor_id(0),
          event_type(0), severity(0) {}
};
GS_STATIC_ASSERT(sizeof(TamperEvent) == 16, "TamperEvent must be 16 bytes");

// ============================================================================
// STATIC BUFFER (no heap allocation)
// ============================================================================
template<typename T, size_t N>
class StaticBuffer {
public:
    StaticBuffer() : size_(0) {
        // Zero-initialize storage (optional, untuk keamanan)
        for (size_t i = 0; i < N * sizeof(T); ++i) {
            storage_[i] = 0;
        }
    }
    
    ~StaticBuffer() {
        clear();
    }
    
    StaticBuffer(const StaticBuffer&) = delete;
    StaticBuffer& operator=(const StaticBuffer&) = delete;
    
    StaticBuffer(StaticBuffer&& other) : size_(other.size_) {
        for (size_t i = 0; i < size_; ++i) {
            T* src = reinterpret_cast<T*>(&other.storage_[i * sizeof(T)]);
            T* dst = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
            new (dst) T(GS_MOVE(*src));
        }
        other.size_ = 0;
    }
    
    StaticBuffer& operator=(StaticBuffer&& other) {
        if (this != &other) {
            clear();
            size_ = other.size_;
            for (size_t i = 0; i < size_; ++i) {
                T* src = reinterpret_cast<T*>(&other.storage_[i * sizeof(T)]);
                T* dst = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
                new (dst) T(GS_MOVE(*src));
            }
            other.size_ = 0;
        }
        return *this;
    }
    
    bool push(const T& item) {
        if (size_ >= N) return false;
        T* ptr = reinterpret_cast<T*>(&storage_[size_ * sizeof(T)]);
        new (ptr) T(item);
        ++size_;
        return true;
    }
    
    bool push(T&& item) {
        if (size_ >= N) return false;
        T* ptr = reinterpret_cast<T*>(&storage_[size_ * sizeof(T)]);
        new (ptr) T(GS_MOVE(item));
        ++size_;
        return true;
    }
    
    bool pop(T& item) {
        if (size_ == 0) return false;
        T* ptr = reinterpret_cast<T*>(&storage_[(size_ - 1) * sizeof(T)]);
        item = GS_MOVE(*ptr);
        ptr->~T();
        --size_;
        return true;
    }
    
    // FIFO removal: remove oldest element (front) and shift remaining
    bool pop_front(T& item) {
        if (size_ == 0) return false;
        T* front = reinterpret_cast<T*>(&storage_[0]);
        item = GS_MOVE(*front);
        front->~T();
        
        // Shift all elements left by one position
        for (size_t i = 1; i < size_; ++i) {
            T* src = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
            T* dst = reinterpret_cast<T*>(&storage_[(i - 1) * sizeof(T)]);
            new (dst) T(GS_MOVE(*src));
            src->~T();
        }
        --size_;
        return true;
    }
    
    void clear() {
        for (size_t i = 0; i < size_; ++i) {
            T* ptr = reinterpret_cast<T*>(&storage_[i * sizeof(T)]);
            ptr->~T();
        }
        size_ = 0;
    }
    
    size_t size() const { return size_; }
    size_t capacity() const { return N; }
    bool empty() const { return size_ == 0; }
    bool full() const { return size_ == N; }
    
    T& operator[](size_t idx) {
        return *reinterpret_cast<T*>(&storage_[idx * sizeof(T)]);
    }
    
    const T& operator[](size_t idx) const {
        return *reinterpret_cast<const T*>(&storage_[idx * sizeof(T)]);
    }
    
private:
    // Raw storage (alignment optional on AVR to avoid 'over-aligned' errors)
#if GS_PLATFORM_NATIVE
    alignas(T) char storage_[N * sizeof(T)];
#else
    char storage_[N * sizeof(T)];
#endif
    size_t size_;
};

// ============================================================================
// BYTE ARRAY (specialized for raw bytes)
// ============================================================================
template<size_t N>
class ByteArray {
public:
    ByteArray() noexcept : size_(0) {
#if GS_PLATFORM_NATIVE
        std::memset(data_, 0, N);
#else
        memset(data_, 0, N);
#endif
    }
    
    void clear() noexcept {
        size_ = 0;
#if GS_PLATFORM_NATIVE
        std::memset(data_, 0, N);
#else
        memset(data_, 0, N);
#endif
    }
    
    GS_NODISCARD inline bool append(const uint8_t* data, size_t len) noexcept {
        if (GS_UNLIKELY(size_ + len > N)) return false;
#if GS_PLATFORM_NATIVE
        std::memcpy(data_ + size_, data, len);
#else
        memcpy(data_ + size_, data, len);
#endif
        size_ += len;
        return true;
    }
    
    GS_NODISCARD constexpr size_t size() const noexcept { return size_; }
    GS_NODISCARD constexpr size_t capacity() const noexcept { return N; }
    
    GS_NODISCARD uint8_t* data() noexcept { return data_; }
    GS_NODISCARD const uint8_t* data() const noexcept { return data_; }
    
    GS_NODISCARD uint8_t& operator[](size_t idx) noexcept { 
        GS_ASSERT(idx < size_);
        return data_[idx]; 
    }
    
    GS_NODISCARD const uint8_t& operator[](size_t idx) const noexcept { 
        GS_ASSERT(idx < size_);
        return data_[idx]; 
    }
    
private:
    uint8_t data_[N];
    size_t size_;
};

} // namespace core
} // namespace gridshield