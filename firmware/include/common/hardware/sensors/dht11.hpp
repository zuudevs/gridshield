/**
 * @file dht11.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief DHT11 temperature & humidity sensor driver (single-wire protocol)
 * @version 1.0
 * @date 2026-04-15
 *
 * Digital sensor using proprietary single-wire protocol (NOT Dallas OneWire).
 * Range: Temperature 0-50°C (±2°C), Humidity 20-80% (±5%).
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "platform/platform.hpp"

#include <cstdint>

// ESP-IDF timing (microsecond precision required)
#if GS_PLATFORM_ESP32
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

namespace gridshield::hardware::sensors {

// ============================================================================
// DHT11 CONSTANTS
// ============================================================================
static constexpr uint32_t DHT11_START_SIGNAL_MS = 20;     // Pull LOW for 20ms
static constexpr uint32_t DHT11_RESPONSE_TIMEOUT_US = 100;
static constexpr uint32_t DHT11_BIT_THRESHOLD_US = 40;    // >40μs HIGH = bit 1
static constexpr size_t   DHT11_DATA_BITS = 40;           // 5 bytes × 8 bits
static constexpr size_t   DHT11_DATA_BYTES = 5;
static constexpr uint32_t DHT11_MIN_INTERVAL_MS = 1000;   // Min 1s between reads

// ============================================================================
// DHT11 READING
// ============================================================================
struct DHT11Reading
{
    int16_t temperature_c10{};   // Temperature in 0.1°C (e.g., 253 = 25.3°C)
    uint16_t humidity_x10{};     // Humidity in 0.1% (e.g., 650 = 65.0%)
};

// ============================================================================
// DHT11 CONFIGURATION
// ============================================================================
struct DHT11Config
{
    uint8_t pin{};

    GS_CONSTEXPR DHT11Config() noexcept = default;
};

// ============================================================================
// DHT11 DRIVER
// ============================================================================
class DHT11Driver
{
public:
    DHT11Driver() noexcept = default;

    /**
     * @brief Initialize the GPIO pin for DHT11.
     */
    core::Result<void> init(const DHT11Config& config) noexcept
    {
        config_ = config;

#if GS_PLATFORM_ESP32
        // Configure as open-drain output (can switch to input for reading)
        gpio_config_t io_conf = {};
        io_conf.pin_bit_mask = (1ULL << config_.pin);
        io_conf.mode = GPIO_MODE_INPUT_OUTPUT_OD;
        io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
        io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
        io_conf.intr_type = GPIO_INTR_DISABLE;

        esp_err_t err = gpio_config(&io_conf);
        if (err != ESP_OK) {
            return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
        }

        // Set HIGH (idle state)
        gpio_set_level(static_cast<gpio_num_t>(config_.pin), 1);
#endif

        initialized_ = true;
        return core::Result<void>{};
    }

    /**
     * @brief Read temperature and humidity from DHT11.
     *
     * Blocking call (~25ms). Must wait at least 1 second between reads.
     */
    core::Result<DHT11Reading> read() noexcept
    {
        if (GS_UNLIKELY(!initialized_)) {
            return core::Result<DHT11Reading>{GS_MAKE_ERROR(core::ErrorCode::SystemNotInitialized)};
        }

#if GS_PLATFORM_ESP32
        uint8_t data[DHT11_DATA_BYTES] = {};
        auto gpio_pin = static_cast<gpio_num_t>(config_.pin);

        // === Start Signal: pull LOW for 25ms ===
        gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);
        int idle_level = gpio_get_level(gpio_pin);

        gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT_OD);
        gpio_pullup_en(gpio_pin);
        gpio_set_level(gpio_pin, 0);
        ets_delay_us(25000);  // 25ms — some modules need > 18ms

        // --- CRITICAL SECTION: must cover release + response + read ---
        portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
        taskENTER_CRITICAL(&mux);

        // Release bus: let pull-up bring it HIGH
        gpio_set_level(gpio_pin, 1);
        ets_delay_us(30);
        gpio_set_direction(gpio_pin, GPIO_MODE_INPUT);

        int after_release = gpio_get_level(gpio_pin);

        int fail_phase = 0;  // 0=ok, 1=wait_response, 2=low_hold, 3=high_hold, 4=bit_read

        // Wait for DHT11 to pull LOW (response ~80μs)
        if (!wait_for_level(gpio_pin, 0, 500)) { fail_phase = 1; }

        // DHT11 holds LOW ~80μs then goes HIGH
        if (!fail_phase && !wait_for_level(gpio_pin, 1, 500)) { fail_phase = 2; }

        // DHT11 holds HIGH ~80μs then goes LOW (start of first bit)
        if (!fail_phase && !wait_for_level(gpio_pin, 0, 500)) { fail_phase = 3; }

        // === Read 40 bits of data ===
        size_t bits_read = 0;
        if (!fail_phase) {
            for (size_t i = 0; i < DHT11_DATA_BITS; ++i) {
                if (!wait_for_level(gpio_pin, 1, 500)) { fail_phase = 4; break; }

                int64_t t_start = esp_timer_get_time();
                if (!wait_for_level(gpio_pin, 0, 500)) { fail_phase = 4; break; }
                int64_t duration = esp_timer_get_time() - t_start;

                size_t byte_idx = i / 8;
                size_t bit_idx = 7 - (i % 8);
                if (duration > DHT11_BIT_THRESHOLD_US) {
                    data[byte_idx] |= (1 << bit_idx);
                }
                bits_read = i + 1;
            }
        }

        taskEXIT_CRITICAL(&mux);
        // --- END CRITICAL SECTION ---

        // Restore output mode (idle HIGH)
        gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT_OD);
        gpio_set_level(gpio_pin, 1);

        if (fail_phase) {
            ESP_LOGW("DHT11", "DIAG: idle=%d after_release=%d fail_phase=%d bits=%d",
                     idle_level, after_release, fail_phase, (int)bits_read);
            return core::Result<DHT11Reading>{GS_MAKE_ERROR(core::ErrorCode::SensorReadFailure)};
        }

        // Restore output mode (idle HIGH)
        gpio_set_direction(gpio_pin, GPIO_MODE_OUTPUT);
        gpio_set_level(gpio_pin, 1);

        // === Verify checksum ===
        uint8_t checksum = data[0] + data[1] + data[2] + data[3];
        if (checksum != data[4]) {
            return core::Result<DHT11Reading>{GS_MAKE_ERROR(core::ErrorCode::IntegrityViolation)};
        }

        // === Parse reading ===
        DHT11Reading reading{};
        // data[0] = humidity integer, data[1] = humidity decimal
        reading.humidity_x10 = static_cast<uint16_t>(data[0] * 10 + data[1]);
        // data[2] = temperature integer, data[3] = temperature decimal
        reading.temperature_c10 = static_cast<int16_t>(data[2] * 10 + data[3]);

        return core::Result<DHT11Reading>{reading};

#else
        // Non-ESP32 stub
        DHT11Reading reading{};
        reading.temperature_c10 = 250; // 25.0°C
        reading.humidity_x10 = 600;    // 60.0%
        return core::Result<DHT11Reading>{reading};
#endif
    }

    GS_NODISCARD bool is_initialized() const noexcept
    {
        return initialized_;
    }

private:
#if GS_PLATFORM_ESP32
    /**
     * @brief Wait for a specific GPIO level with timeout.
     */
    static bool wait_for_level(gpio_num_t pin, int level, uint32_t timeout_us) noexcept
    {
        int64_t deadline = esp_timer_get_time() + timeout_us;
        while (gpio_get_level(pin) != level) {
            if (esp_timer_get_time() > deadline) {
                return false;
            }
        }
        return true;
    }
#endif

    DHT11Config config_{};
    bool initialized_{false};
};

} // namespace gridshield::hardware::sensors
