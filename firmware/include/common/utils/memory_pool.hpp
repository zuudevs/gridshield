/**
 * @file memory_pool.hpp
 * @brief Fixed-block memory pool allocator
 *
 * @note Header-only, zero heap allocation. Template parameters
 *       control block size and pool capacity at compile time.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::utils {

static constexpr size_t BITMAP_BITS_PER_BYTE = 8;

/**
 * @brief Fixed-block memory pool with compile-time sizing.
 * @tparam BlockSize Size of each allocation block in bytes
 * @tparam NumBlocks Number of blocks in the pool
 */
template <size_t BlockSize, size_t NumBlocks> class MemoryPool
{
    static_assert(BlockSize > 0, "BlockSize must be > 0");
    static_assert(NumBlocks > 0, "NumBlocks must be > 0");

    static constexpr size_t BITMAP_SIZE =
        (NumBlocks + BITMAP_BITS_PER_BYTE - 1) / BITMAP_BITS_PER_BYTE;

public:
    core::Result<void> init() noexcept
    {
        bitmap_.fill(0);
        used_count_ = 0;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void*> allocate() noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<void*>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        for (size_t idx = 0; idx < NumBlocks; ++idx) {
            const size_t byte_idx = idx / BITMAP_BITS_PER_BYTE;
            const size_t bit_idx = idx % BITMAP_BITS_PER_BYTE;
            const uint8_t mask = static_cast<uint8_t>(1U << bit_idx);

            if ((bitmap_[byte_idx] & mask) == 0) {
                bitmap_[byte_idx] |= mask;
                used_count_++;
                auto* ptr = &storage_[idx * BlockSize];
                std::memset(ptr, 0, BlockSize);
                return core::Result<void*>(static_cast<void*>(ptr));
            }
        }
        return core::Result<void*>(GS_MAKE_ERROR(core::ErrorCode::PoolExhausted));
    }

    core::Result<void> deallocate(void* ptr) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }
        if (ptr == nullptr) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        auto* byte_ptr = static_cast<uint8_t*>(ptr);
        if (byte_ptr < storage_.data() || byte_ptr >= storage_.data() + storage_.size()) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        const size_t offset = static_cast<size_t>(byte_ptr - storage_.data());
        if (offset % BlockSize != 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        const size_t idx = offset / BlockSize;
        const size_t byte_idx = idx / BITMAP_BITS_PER_BYTE;
        const size_t bit_idx = idx % BITMAP_BITS_PER_BYTE;
        const uint8_t mask = static_cast<uint8_t>(1U << bit_idx);

        if ((bitmap_[byte_idx] & mask) == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::PoolDoubleFree);
        }

        bitmap_[byte_idx] &= static_cast<uint8_t>(~mask);
        used_count_--;
        return core::Result<void>{};
    }

    size_t used() const noexcept
    {
        return used_count_;
    }
    size_t available() const noexcept
    {
        return NumBlocks - used_count_;
    }
    static constexpr size_t capacity() noexcept
    {
        return NumBlocks;
    }
    static constexpr size_t block_size() noexcept
    {
        return BlockSize;
    }
    static constexpr size_t total_bytes() noexcept
    {
        return BlockSize * NumBlocks;
    }
    bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    std::array<uint8_t, BlockSize * NumBlocks> storage_{};
    std::array<uint8_t, BITMAP_SIZE> bitmap_{};
    size_t used_count_{0};
    bool initialized_{false};
};

} // namespace gridshield::utils
