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
    constexpr StaticBuffer() noexcept : size_(0) {}
    
    ~StaticBuffer() noexcept {
        clear();
    }
    
    // Delete copy
    StaticBuffer(const StaticBuffer&) = delete;
    StaticBuffer& operator=(const StaticBuffer&) = delete;
    
    // Move constructor
    StaticBuffer(StaticBuffer&& other) noexcept(std::is_nothrow_move_constructible<T>::value)
        : size_(other.size_) {
        for (size_t i = 0; i < size_; ++i) {
            new (&data_[i]) T(GS_MOVE(*reinterpret_cast<T*>(&other.data_[i])));
        }
        other.size_ = 0;
    }
    
    // Move assignment
    StaticBuffer& operator=(StaticBuffer&& other) noexcept(std::is_nothrow_move_constructible<T>::value) {
        if (this != &other) {
            clear();
            size_ = other.size_;
            for (size_t i = 0; i < size_; ++i) {
                new (&data_[i]) T(GS_MOVE(*reinterpret_cast<T*>(&other.data_[i])));
            }
            other.size_ = 0;
        }
        return *this;
    }
    
    GS_NODISCARD inline bool push(const T& item) noexcept(std::is_nothrow_copy_constructible<T>::value) {
        if (GS_UNLIKELY(size_ >= N)) return false;
        new (&data_[size_++]) T(item);
        return true;
    }
    
    GS_NODISCARD inline bool push(T&& item) noexcept(std::is_nothrow_move_constructible<T>::value) {
        if (GS_UNLIKELY(size_ >= N)) return false;
        new (&data_[size_++]) T(GS_MOVE(item));
        return true;
    }
    
    GS_NODISCARD inline bool pop(T& item) noexcept(std::is_nothrow_move_assignable<T>::value) {
        if (GS_UNLIKELY(size_ == 0)) return false;
        
        // ZUU FIX: Access through pointer cast, not directly on storage
        T* ptr = reinterpret_cast<T*>(&data_[size_ - 1]);
        item = GS_MOVE(*ptr);
        ptr->~T();
        
        size_--;
        return true;
    }
    
    void clear() noexcept {
        for (size_t i = 0; i < size_; ++i) {
            // ZUU FIX: Explicit destructor call via pointer cast
            reinterpret_cast<T*>(&data_[i])->~T();
        }
        size_ = 0;
    }
    
    GS_NODISCARD constexpr size_t size() const noexcept { return size_; }
    GS_NODISCARD constexpr size_t capacity() const noexcept { return N; }
    GS_NODISCARD constexpr bool empty() const noexcept { return size_ == 0; }
    GS_NODISCARD constexpr bool full() const noexcept { return size_ == N; }
    
    GS_NODISCARD T& operator[](size_t idx) noexcept { 
        GS_ASSERT(idx < size_);
        return *reinterpret_cast<T*>(&data_[idx]); 
    }
    
    GS_NODISCARD const T& operator[](size_t idx) const noexcept { 
        GS_ASSERT(idx < size_);
        return *reinterpret_cast<const T*>(&data_[idx]); 
    }
    
    GS_NODISCARD T* data() noexcept { return reinterpret_cast<T*>(data_); }
    GS_NODISCARD const T* data() const noexcept { return reinterpret_cast<const T*>(data_); }
    
    // Iterators
    GS_NODISCARD T* begin() noexcept { return data(); }
    GS_NODISCARD T* end() noexcept { return data() + size_; }
    GS_NODISCARD const T* begin() const noexcept { return data(); }
    GS_NODISCARD const T* end() const noexcept { return data() + size_; }
    
private:
    typename std::aligned_storage<sizeof(T), alignof(T)>::type data_[N];
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