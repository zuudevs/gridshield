/**
 * @file tflite_runner.hpp
 * @brief TensorFlow Lite Micro inference runner
 *
 * @note Header-only, zero heap allocation.
 */

#pragma once

#include "core/error.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>

namespace gridshield::analytics {

// ============================================================================
// Constants
// ============================================================================
static constexpr size_t TFLITE_DEFAULT_ARENA_SIZE = 8192;
static constexpr size_t TFLITE_MAX_INPUT_SIZE = 64;
static constexpr size_t TFLITE_MAX_OUTPUT_SIZE = 16;
static constexpr size_t TFLITE_MAX_MODEL_SIZE = 16384;

// ============================================================================
// Types
// ============================================================================

enum class QuantizationType : uint8_t
{
    Float32 = 0,
    Int8 = 1,
    UInt8 = 2
};

struct ModelInfo
{
    uint16_t input_size{0};
    uint16_t output_size{0};
    QuantizationType input_quant{QuantizationType::Float32};
    QuantizationType output_quant{QuantizationType::Float32};
    uint32_t model_size{0};
    uint32_t arena_size{0};
    bool loaded{false};
};

struct InferenceResult
{
    std::array<int32_t, TFLITE_MAX_OUTPUT_SIZE> output{};
    uint16_t output_count{0};
    uint32_t inference_time_us{0};
    bool valid{false};
};

// ============================================================================
// Interface
// ============================================================================

class ITfliteRunner
{
public:
    virtual ~ITfliteRunner() = default;

    virtual core::Result<void> load_model(const uint8_t* model_data,
                                          size_t model_size) noexcept = 0;
    virtual core::Result<void> set_input(const int32_t* input_data,
                                         size_t input_count) noexcept = 0;
    virtual core::Result<InferenceResult> invoke() noexcept = 0;
    virtual ModelInfo get_model_info() const noexcept = 0;
    virtual bool is_loaded() const noexcept = 0;
    virtual void unload() noexcept = 0;
};

// ============================================================================
// Mock Implementation
// ============================================================================

class TfliteRunner final : public ITfliteRunner
{
public:
    core::Result<void> load_model(const uint8_t* model_data, size_t model_size) noexcept override
    {
        if (model_data == nullptr || model_size == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (model_size > TFLITE_MAX_MODEL_SIZE) {
            return GS_MAKE_ERROR(core::ErrorCode::ModelLoadFailed);
        }

        model_info_.model_size = static_cast<uint32_t>(model_size);
        model_info_.input_size = mock_input_size_;
        model_info_.output_size = mock_output_size_;
        model_info_.arena_size = TFLITE_DEFAULT_ARENA_SIZE;
        model_info_.loaded = true;
        return core::Result<void>{};
    }

    core::Result<void> set_input(const int32_t* input_data, size_t input_count) noexcept override
    {
        if (!model_info_.loaded) {
            return GS_MAKE_ERROR(core::ErrorCode::ModelLoadFailed);
        }
        if (input_data == nullptr || input_count == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        if (input_count > TFLITE_MAX_INPUT_SIZE) {
            return GS_MAKE_ERROR(core::ErrorCode::TensorMismatch);
        }

        std::memcpy(input_buffer_.data(), input_data, input_count * sizeof(int32_t));
        input_count_ = static_cast<uint16_t>(input_count);
        return core::Result<void>{};
    }

    core::Result<InferenceResult> invoke() noexcept override
    {
        if (!model_info_.loaded) {
            return core::Result<InferenceResult>(GS_MAKE_ERROR(core::ErrorCode::ModelLoadFailed));
        }
        if (simulate_failure_) {
            return core::Result<InferenceResult>(GS_MAKE_ERROR(core::ErrorCode::InferenceFailed));
        }

        InferenceResult result{};
        result.output = mock_output_;
        result.output_count = mock_output_size_;
        result.inference_time_us = mock_inference_time_us_;
        result.valid = true;
        invoke_count_++;
        return core::Result<InferenceResult>(GS_MOVE(result));
    }

    ModelInfo get_model_info() const noexcept override
    {
        return model_info_;
    }
    bool is_loaded() const noexcept override
    {
        return model_info_.loaded;
    }
    void unload() noexcept override
    {
        model_info_ = ModelInfo{};
    }

    // === Test helpers ===
    void set_mock_output(const int32_t* data, uint16_t count) noexcept
    {
        size_t copy_count = std::min(static_cast<size_t>(count), TFLITE_MAX_OUTPUT_SIZE);
        std::memcpy(mock_output_.data(), data, copy_count * sizeof(int32_t));
        mock_output_size_ = static_cast<uint16_t>(copy_count);
    }

    void set_mock_dimensions(uint16_t input_size, uint16_t output_size) noexcept
    {
        mock_input_size_ = input_size;
        mock_output_size_ = output_size;
    }

    void set_simulate_failure(bool fail) noexcept
    {
        simulate_failure_ = fail;
    }
    void set_mock_inference_time(uint32_t time_us) noexcept
    {
        mock_inference_time_us_ = time_us;
    }
    uint32_t invoke_count() const noexcept
    {
        return invoke_count_;
    }

private:
    ModelInfo model_info_{};
    std::array<int32_t, TFLITE_MAX_INPUT_SIZE> input_buffer_{};
    std::array<int32_t, TFLITE_MAX_OUTPUT_SIZE> mock_output_{};
    uint16_t input_count_{0};
    uint16_t mock_input_size_{6};
    uint16_t mock_output_size_{1};
    uint32_t mock_inference_time_us_{150};
    uint32_t invoke_count_{0};
    bool simulate_failure_{false};
};

} // namespace gridshield::analytics
