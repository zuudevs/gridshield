/**
 * @file ml_anomaly.hpp
 * @brief ML-based anomaly detection
 *
 * @note Header-only, zero heap allocation.
 */

#pragma once

#include "analytics/tflite_runner.hpp"
#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>

namespace gridshield::analytics {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t ML_FEATURE_COUNT = 6;
static constexpr int32_t ML_DEFAULT_THRESHOLD_X1000 = 700;
static constexpr int32_t ML_THRESHOLD_SCALE = 1000;
static constexpr int32_t ML_MAX_THRESHOLD_X1000 = 999;
static constexpr int32_t ML_MIN_THRESHOLD_X1000 = 100;
static constexpr int32_t ML_ADAPTIVE_STEP_X1000 = 10;
static constexpr size_t ML_SCORE_HISTORY_SIZE = 16;

// ============================================================================
// Types
// ============================================================================

struct FeatureVector
{
    std::array<int32_t, ML_FEATURE_COUNT> features{};
    uint64_t timestamp{0};
    bool valid{false};
};

struct AnomalyScore
{
    int32_t score{0};
    int32_t threshold{ML_DEFAULT_THRESHOLD_X1000};
    bool is_anomaly{false};
    int32_t confidence{0};
    uint8_t top_feature_idx{0};
    bool valid{false};
};

struct SensorSnapshot
{
    uint32_t voltage_mv{0};
    uint32_t current_ma{0};
    uint32_t energy_wh{0};
    int16_t temperature_c10{0};
    int32_t accel_mg{0};
    uint64_t timestamp{0};
};

// ============================================================================
// ML Anomaly Detector
// ============================================================================

class MlAnomalyDetector
{
public:
    core::Result<void> init(ITfliteRunner* runner) noexcept
    {
        if (runner == nullptr) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        runner_ = runner;
        threshold_x1000_ = ML_DEFAULT_THRESHOLD_X1000;
        score_count_ = 0;
        initialized_ = true;
        return core::Result<void>{};
    }

    FeatureVector extract_features(const SensorSnapshot& snapshot) const noexcept
    {
        FeatureVector fv{};
        static constexpr int32_t VOLTAGE_NORM_DIVISOR = 240;
        static constexpr int32_t CURRENT_NORM_DIVISOR = 100;
        static constexpr int32_t ENERGY_NORM_DIVISOR = 1000;
        static constexpr int32_t TEMP_NORM_DIVISOR = 500;
        static constexpr int32_t ACCEL_NORM_DIVISOR = 1000;
        static constexpr int32_t SECONDS_PER_DAY = 86400;

        fv.features[0] =
            static_cast<int32_t>((static_cast<int64_t>(snapshot.voltage_mv) * ML_THRESHOLD_SCALE) /
                                 VOLTAGE_NORM_DIVISOR);
        fv.features[1] =
            static_cast<int32_t>((static_cast<int64_t>(snapshot.current_ma) * ML_THRESHOLD_SCALE) /
                                 CURRENT_NORM_DIVISOR);
        fv.features[2] = static_cast<int32_t>(
            (static_cast<int64_t>(snapshot.energy_wh) * ML_THRESHOLD_SCALE) / ENERGY_NORM_DIVISOR);
        fv.features[3] = static_cast<int32_t>(
            (static_cast<int64_t>(snapshot.temperature_c10) * ML_THRESHOLD_SCALE) /
            TEMP_NORM_DIVISOR);
        fv.features[4] = static_cast<int32_t>(
            (static_cast<int64_t>(snapshot.accel_mg) * ML_THRESHOLD_SCALE) / ACCEL_NORM_DIVISOR);
        fv.features[5] =
            static_cast<int32_t>(snapshot.timestamp % static_cast<uint64_t>(SECONDS_PER_DAY));

        fv.timestamp = snapshot.timestamp;
        fv.valid = true;
        return fv;
    }

    core::Result<AnomalyScore> score(const SensorSnapshot& snapshot) noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<AnomalyScore>(GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized));
        }
        if (runner_ == nullptr || !runner_->is_loaded()) {
            return core::Result<AnomalyScore>(GS_MAKE_ERROR(core::ErrorCode::ModelLoadFailed));
        }

        auto fv = extract_features(snapshot);

        auto input_result = runner_->set_input(fv.features.data(), ML_FEATURE_COUNT);
        if (input_result.is_error()) {
            return core::Result<AnomalyScore>(input_result.error());
        }

        auto invoke_result = runner_->invoke();
        if (invoke_result.is_error()) {
            return core::Result<AnomalyScore>(invoke_result.error());
        }

        const auto& inference = invoke_result.value();

        AnomalyScore result{};
        result.score = (inference.output_count > 0) ? inference.output[0] : 0;

        if (result.score < 0)
            result.score = 0;
        if (result.score > ML_THRESHOLD_SCALE)
            result.score = ML_THRESHOLD_SCALE;

        result.threshold = threshold_x1000_;
        result.is_anomaly = (result.score >= threshold_x1000_);
        result.confidence = (result.score > threshold_x1000_) ? result.score - threshold_x1000_
                                                              : threshold_x1000_ - result.score;

        int32_t max_abs = 0;
        for (size_t feat_idx = 0; feat_idx < ML_FEATURE_COUNT; ++feat_idx) {
            int32_t abs_val =
                (fv.features[feat_idx] >= 0) ? fv.features[feat_idx] : -fv.features[feat_idx];
            if (abs_val > max_abs) {
                max_abs = abs_val;
                result.top_feature_idx = static_cast<uint8_t>(feat_idx);
            }
        }

        result.valid = true;
        adapt_threshold(result.is_anomaly);

        if (score_count_ < ML_SCORE_HISTORY_SIZE) {
            score_history_[score_count_] = result.score;
            score_count_++;
        } else {
            for (size_t hist_idx = 1; hist_idx < ML_SCORE_HISTORY_SIZE; ++hist_idx) {
                score_history_[hist_idx - 1] = score_history_[hist_idx];
            }
            score_history_[ML_SCORE_HISTORY_SIZE - 1] = result.score;
        }

        return core::Result<AnomalyScore>(GS_MOVE(result));
    }

    int32_t threshold() const noexcept
    {
        return threshold_x1000_;
    }
    void set_threshold(int32_t threshold_x1000) noexcept
    {
        if (threshold_x1000 >= ML_MIN_THRESHOLD_X1000 &&
            threshold_x1000 <= ML_MAX_THRESHOLD_X1000) {
            threshold_x1000_ = threshold_x1000;
        }
    }
    size_t scored_count() const noexcept
    {
        return score_count_;
    }
    bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
    void adapt_threshold(bool was_anomaly) noexcept
    {
        if (was_anomaly) {
            threshold_x1000_ += ML_ADAPTIVE_STEP_X1000;
            if (threshold_x1000_ > ML_MAX_THRESHOLD_X1000) {
                threshold_x1000_ = ML_MAX_THRESHOLD_X1000;
            }
        } else {
            threshold_x1000_ -= ML_ADAPTIVE_STEP_X1000;
            if (threshold_x1000_ < ML_MIN_THRESHOLD_X1000) {
                threshold_x1000_ = ML_MIN_THRESHOLD_X1000;
            }
        }
    }

    ITfliteRunner* runner_{nullptr};
    int32_t threshold_x1000_{ML_DEFAULT_THRESHOLD_X1000};
    std::array<int32_t, ML_SCORE_HISTORY_SIZE> score_history_{};
    size_t score_count_{0};
    bool initialized_{false};
};

} // namespace gridshield::analytics
