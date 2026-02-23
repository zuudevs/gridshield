/**
 * @file retry.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Network retry logic with exponential backoff
 * @version 1.0
 * @date 2026-02-23
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

namespace gridshield::network {

// ============================================================================
// RETRY POLICY
// ============================================================================
struct RetryPolicy {
    uint8_t max_retries{3};
    uint32_t base_delay_ms{100};
    uint32_t max_delay_ms{5000};
    uint8_t backoff_factor{2};

    GS_CONSTEXPR RetryPolicy() noexcept = default;

    GS_CONSTEXPR RetryPolicy(uint8_t retries, uint32_t base_ms,
                             uint32_t max_ms, uint8_t factor) noexcept
        : max_retries(retries), base_delay_ms(base_ms),
          max_delay_ms(max_ms), backoff_factor(factor) {}
};

// ============================================================================
// RETRY RESULT
// ============================================================================
struct RetryResult {
    uint8_t attempts{};
    bool succeeded{false};
    core::ErrorCode last_error{core::ErrorCode::None};
};

// ============================================================================
// RETRY EXECUTOR
// ============================================================================

/**
 * @brief Executes an operation with exponential backoff retry
 *
 * Usage:
 *   RetryExecutor retry(policy, *platform.time);
 *   auto result = retry.execute([&]() { return transport.send(...); });
 */
class RetryExecutor {
public:
    RetryExecutor(const RetryPolicy& policy,
                  platform::IPlatformTime& time) noexcept
        : policy_(policy), time_(time) {}

    /**
     * @brief Execute a callable with retry logic
     * @tparam Func Callable returning core::Result<void>
     */
    template <typename Func>
    RetryResult execute(Func&& operation) noexcept {
        RetryResult result;
        uint32_t delay = policy_.base_delay_ms;

        for (uint8_t attempt = 0; attempt <= policy_.max_retries; ++attempt) {
            result.attempts = attempt + 1;

            auto op_result = operation();
            if (op_result.is_ok()) {
                result.succeeded = true;
                result.last_error = core::ErrorCode::None;
                return result;
            }

            result.last_error = op_result.error().code;

            // Don't delay after the last attempt
            if (attempt < policy_.max_retries) {
                time_.delay_ms(delay);

                // Exponential backoff
                delay *= policy_.backoff_factor;
                if (delay > policy_.max_delay_ms) {
                    delay = policy_.max_delay_ms;
                }
            }
        }

        return result;
    }

    /**
     * @brief Execute with retry, returning the Result directly
     */
    template <typename Func>
    core::Result<void> execute_result(Func&& operation) noexcept {
        auto retry_result = execute(GS_FORWARD(Func, operation));
        if (retry_result.succeeded) {
            return core::Result<void>();
        }
        return GS_MAKE_ERROR(retry_result.last_error);
    }

private:
    RetryPolicy policy_;
    platform::IPlatformTime& time_;
};

} // namespace gridshield::network
