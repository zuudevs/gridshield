/**
 * @file zero_copy_buffer.hpp
 * @brief Zero-copy ring buffer with span-based views
 *
 * @note Header-only, zero heap allocation.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::utils {

template <typename T> struct Span
{
    T* ptr{nullptr};
    size_t length{0};

    T& operator[](size_t idx) noexcept
    {
        return ptr[idx];
    }
    const T& operator[](size_t idx) const noexcept
    {
        return ptr[idx];
    }
    T* data() noexcept
    {
        return ptr;
    }
    const T* data() const noexcept
    {
        return ptr;
    }
    size_t size() const noexcept
    {
        return length;
    }
    bool empty() const noexcept
    {
        return length == 0;
    }
};

/**
 * @brief Zero-copy circular buffer.
 * @tparam Capacity Total buffer size in bytes
 */
template <size_t Capacity> class ZeroCopyBuffer
{
    static_assert(Capacity > 0, "Capacity must be > 0");

public:
    core::Result<void> init() noexcept
    {
        head_ = 0;
        tail_ = 0;
        count_ = 0;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<Span<uint8_t>> write_span(size_t requested) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<Span<uint8_t>>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (requested == 0) {
            return core::Result<Span<uint8_t>>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        size_t free_space = Capacity - count_;
        if (requested > free_space) {
            return core::Result<Span<uint8_t>>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }

        size_t contig = Capacity - head_;
        size_t actual = std::min(requested, contig);

        Span<uint8_t> span{};
        span.ptr = &buffer_[head_];
        span.length = actual;
        return core::Result<Span<uint8_t>>(GS_MOVE(span));
    }

    core::Result<void> commit_write(size_t len) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        size_t free_space = Capacity - count_;
        if (len > free_space) {
            return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
        }
        head_ = (head_ + len) % Capacity;
        count_ += len;
        return core::Result<void>{};
    }

    core::Result<Span<const uint8_t>> read_span(size_t requested) const noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<Span<const uint8_t>>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (requested == 0 || count_ == 0) {
            return core::Result<Span<const uint8_t>>(
                GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        size_t contig = Capacity - tail_;
        size_t actual = std::min({requested, count_, contig});

        Span<const uint8_t> span{};
        span.ptr = &buffer_[tail_];
        span.length = actual;
        return core::Result<Span<const uint8_t>>(GS_MOVE(span));
    }

    core::Result<void> commit_read(size_t len) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (len > count_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        tail_ = (tail_ + len) % Capacity;
        count_ -= len;
        return core::Result<void>{};
    }

    core::Result<size_t> write(const uint8_t* data, size_t len) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (data == nullptr || len == 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        size_t free_space = Capacity - count_;
        size_t to_write = std::min(len, free_space);
        if (to_write == 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
        }

        size_t first = std::min(to_write, Capacity - head_);
        std::memcpy(&buffer_[head_], data, first);
        if (to_write > first) {
            std::memcpy(&buffer_[0], data + first, to_write - first);
        }

        head_ = (head_ + to_write) % Capacity;
        count_ += to_write;
        return core::Result<size_t>(to_write);
    }

    core::Result<size_t> read(uint8_t* data, size_t max_len) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (data == nullptr || max_len == 0) {
            return core::Result<size_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }
        size_t to_read = std::min(max_len, count_);
        if (to_read == 0) {
            return core::Result<size_t>(static_cast<size_t>(0));
        }

        size_t first = std::min(to_read, Capacity - tail_);
        std::memcpy(data, &buffer_[tail_], first);
        if (to_read > first) {
            std::memcpy(data + first, &buffer_[0], to_read - first);
        }

        tail_ = (tail_ + to_read) % Capacity;
        count_ -= to_read;
        return core::Result<size_t>(to_read);
    }

    size_t used() const noexcept
    {
        return count_;
    }
    size_t available() const noexcept
    {
        return Capacity - count_;
    }
    static constexpr size_t capacity() noexcept
    {
        return Capacity;
    }
    bool empty() const noexcept
    {
        return count_ == 0;
    }
    bool full() const noexcept
    {
        return count_ == Capacity;
    }
    bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    std::array<uint8_t, Capacity> buffer_{};
    size_t head_{0};
    size_t tail_{0};
    size_t count_{0};
    bool initialized_{false};
};

} // namespace gridshield::utils
