/**
 * @file time_series.hpp
 * @brief Fixed-size time series buffer with forecasting
 *
 * @note Header-only, zero heap allocation.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>

namespace gridshield::analytics {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t TS_DEFAULT_CAPACITY = 128;
static constexpr size_t TS_MIN_FORECAST_SAMPLES = 3;
static constexpr uint16_t TS_DEFAULT_ALPHA_X1000 = 200;
static constexpr uint32_t TS_FIXPOINT_SCALE = 1000;

// ============================================================================
// Types
// ============================================================================

struct DataPoint
{
    int32_t value{0};
    uint64_t timestamp{0};
    bool valid{false};
};

struct ForecastResult
{
    int32_t predicted_value{0};
    uint32_t horizon_s{0};
    int8_t trend{0};
    bool valid{false};
};

struct TimeSeriesStats
{
    int32_t min_value{0};
    int32_t max_value{0};
    int32_t mean_value{0};
    int32_t moving_avg{0};
    int32_t exp_smooth{0};
    size_t sample_count{0};
};

// ============================================================================
// Time Series Buffer
// ============================================================================

template <size_t N = TS_DEFAULT_CAPACITY> class TimeSeriesBuffer
{
    static_assert(N > 0, "Capacity must be > 0");

public:
    core::Result<void> init(uint16_t alpha_x1000 = TS_DEFAULT_ALPHA_X1000) noexcept
    {
        head_ = 0;
        count_ = 0;
        alpha_x1000_ = alpha_x1000;
        exp_smooth_ = 0;
        initialized_ = true;
        return core::Result<void>{};
    }

    core::Result<void> push(int32_t value, uint64_t timestamp) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
        }

        DataPoint pt{};
        pt.value = value;
        pt.timestamp = timestamp;
        pt.valid = true;

        buffer_[head_] = pt;
        head_ = (head_ + 1) % N;
        if (count_ < N) {
            count_++;
        }

        // Exponential moving average
        if (count_ == 1) {
            exp_smooth_ = value;
        } else {
            int64_t alpha = alpha_x1000_;
            int64_t one_minus_alpha = static_cast<int64_t>(TS_FIXPOINT_SCALE) - alpha;
            exp_smooth_ = static_cast<int32_t>((alpha * value + one_minus_alpha * exp_smooth_) /
                                               static_cast<int64_t>(TS_FIXPOINT_SCALE));
        }

        return core::Result<void>{};
    }

    core::Result<int32_t> moving_average(size_t window) const noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<int32_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (window == 0 || count_ == 0) {
            return core::Result<int32_t>(GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
        }

        size_t actual_window = (window > count_) ? count_ : window;
        int64_t sum = 0;
        for (size_t step = 0; step < actual_window; ++step) {
            size_t idx = (head_ + N - 1 - step) % N;
            sum += buffer_[idx].value;
        }
        return core::Result<int32_t>(
            static_cast<int32_t>(sum / static_cast<int64_t>(actual_window)));
    }

    core::Result<int32_t> exponential_smooth() const noexcept
    {
        if (!initialized_ || count_ == 0) {
            return core::Result<int32_t>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        return core::Result<int32_t>(exp_smooth_);
    }

    core::Result<ForecastResult> forecast(uint32_t horizon_s) const noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<ForecastResult>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (count_ < TS_MIN_FORECAST_SAMPLES) {
            return core::Result<ForecastResult>(GS_MAKE_ERROR(core::ErrorCode::DataInvalid));
        }

        int64_t sum_x = 0, sum_y = 0, sum_xy = 0, sum_x2 = 0;
        int64_t num = static_cast<int64_t>(count_);

        for (size_t step = 0; step < count_; ++step) {
            size_t idx = (head_ + N - count_ + step) % N;
            int64_t x_val = static_cast<int64_t>(step);
            int64_t y_val = buffer_[idx].value;
            sum_x += x_val;
            sum_y += y_val;
            sum_xy += x_val * y_val;
            sum_x2 += x_val * x_val;
        }

        int64_t denom = num * sum_x2 - sum_x * sum_x;
        if (denom == 0) {
            ForecastResult result{};
            result.predicted_value = static_cast<int32_t>(sum_y / num);
            result.horizon_s = horizon_s;
            result.trend = 0;
            result.valid = true;
            return core::Result<ForecastResult>(GS_MOVE(result));
        }

        int64_t slope_num = num * sum_xy - sum_x * sum_y;

        uint64_t first_ts = buffer_[(head_ + N - count_) % N].timestamp;
        uint64_t last_ts = buffer_[(head_ + N - 1) % N].timestamp;
        uint64_t time_span = last_ts - first_ts;

        int64_t steps_ahead = 0;
        if (time_span > 0 && count_ > 1) {
            int64_t interval = static_cast<int64_t>(time_span) / (static_cast<int64_t>(count_) - 1);
            steps_ahead = (interval > 0) ? (static_cast<int64_t>(horizon_s) / interval) : 1;
        } else {
            steps_ahead = 1;
        }

        int64_t x_pred = static_cast<int64_t>(count_) + steps_ahead;
        int64_t predicted = (sum_y * sum_x2 - sum_x * sum_xy + x_pred * slope_num) / denom;

        ForecastResult result{};
        result.predicted_value = static_cast<int32_t>(predicted);
        result.horizon_s = horizon_s;
        result.trend = (slope_num > 0) ? 1 : ((slope_num < 0) ? -1 : 0);
        result.valid = true;
        return core::Result<ForecastResult>(GS_MOVE(result));
    }

    core::Result<TimeSeriesStats> stats() const noexcept
    {
        if (!initialized_ || count_ == 0) {
            return core::Result<TimeSeriesStats>(
                GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }

        TimeSeriesStats st{};
        st.sample_count = count_;
        st.exp_smooth = exp_smooth_;

        int64_t sum = 0;
        st.min_value = buffer_[(head_ + N - count_) % N].value;
        st.max_value = st.min_value;

        for (size_t step = 0; step < count_; ++step) {
            size_t idx = (head_ + N - count_ + step) % N;
            int32_t val = buffer_[idx].value;
            sum += val;
            if (val < st.min_value)
                st.min_value = val;
            if (val > st.max_value)
                st.max_value = val;
        }

        st.mean_value = static_cast<int32_t>(sum / static_cast<int64_t>(count_));

        static constexpr size_t DEFAULT_MA_WINDOW = 10;
        auto ma_result = moving_average(DEFAULT_MA_WINDOW);
        if (ma_result.is_ok()) {
            st.moving_avg = ma_result.value();
        }

        return core::Result<TimeSeriesStats>(GS_MOVE(st));
    }

    size_t count() const noexcept
    {
        return count_;
    }
    static constexpr size_t capacity() noexcept
    {
        return N;
    }
    bool is_initialized() const noexcept
    {
        return initialized_;
    }

    core::Result<DataPoint> latest() const noexcept
    {
        if (count_ == 0) {
            return core::Result<DataPoint>(GS_MAKE_ERROR(core::ErrorCode::DataInvalid));
        }
        return core::Result<DataPoint>(buffer_[(head_ + N - 1) % N]);
    }

private:
    std::array<DataPoint, N> buffer_{};
    size_t head_{0};
    size_t count_{0};
    uint16_t alpha_x1000_{TS_DEFAULT_ALPHA_X1000};
    int32_t exp_smooth_{0};
    bool initialized_{false};
};

} // namespace gridshield::analytics
