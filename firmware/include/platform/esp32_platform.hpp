/**
 * @file esp32_platform.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Real ESP32 platform implementations using ESP-IDF APIs
 * @version 1.0
 * @date 2026-02-23
 *
 * Provides hardware-backed crypto (esp_random, mbedTLS), NVS storage,
 * and Task Watchdog Timer for production and QEMU builds.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "platform/platform.hpp"
#include "utils/gs_macros.hpp"

#include <cstring>

// ESP-IDF APIs
#include "esp_random.h"
#include "esp_crc.h"
#include "esp_task_wdt.h"
#include "nvs_flash.h"
#include "nvs.h"

// mbedTLS (built into ESP-IDF)
#include "mbedtls/sha256.h"

namespace gridshield {
namespace platform {
namespace esp32 {

// ============================================================================
// ESP32 CRYPTO — Hardware RNG + mbedTLS SHA-256 + CRC32
// ============================================================================
class Esp32Crypto : public IPlatformCrypto {
public:
  Esp32Crypto() noexcept = default;

  core::Result<void> random_bytes(uint8_t *buffer,
                                  size_t length) noexcept override {
    if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
      return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }

    // esp_fill_random uses the hardware RNG on real ESP32.
    // On QEMU it uses a PRNG seed — same API, same code path.
    esp_fill_random(buffer, length);
    return core::Result<void>();
  }

  core::Result<uint32_t> crc32(const uint8_t *data,
                               size_t length) noexcept override {
    if (GS_UNLIKELY(data == nullptr)) {
      return core::Result<uint32_t>(
          GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }

    // ESP-IDF hardware-accelerated CRC32
    uint32_t result = esp_crc32_le(0, data, length);
    return core::Result<uint32_t>(result);
  }

  core::Result<void> sha256(const uint8_t *data, size_t length,
                            uint8_t *hash_out) noexcept override {
    if (GS_UNLIKELY(data == nullptr || hash_out == nullptr)) {
      return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }

    // mbedTLS SHA-256 (hardware-accelerated on ESP32)
    int ret = mbedtls_sha256(data, length, hash_out, 0 /* not SHA-224 */);
    if (ret != 0) {
      return GS_MAKE_ERROR(core::ErrorCode::CryptoFailure);
    }

    return core::Result<void>();
  }
};

// ============================================================================
// ESP32 NVS STORAGE — Non-Volatile Storage backed key/config store
// ============================================================================
class Esp32Storage : public IPlatformStorage {
public:
  static constexpr size_t MAX_BLOB_SIZE = 4096;
  static constexpr const char *NVS_NAMESPACE = "gridshield";

  Esp32Storage() noexcept : initialized_(false) {}

  /**
   * @brief Initialize NVS flash. Must be called once before use.
   */
  core::Result<void> init() noexcept {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
        err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      // NVS partition was truncated — erase and re-init
      nvs_flash_erase();
      err = nvs_flash_init();
    }

    if (err != ESP_OK) {
      return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
    }

    initialized_ = true;
    return core::Result<void>();
  }

  core::Result<size_t> read(uint32_t address, uint8_t *buffer,
                            size_t length) noexcept override {
    if (GS_UNLIKELY(buffer == nullptr || length == 0)) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
      // No data stored yet — return zeroed buffer
      memset(buffer, 0xFF, length);
      return core::Result<size_t>(length);
    }

    // Use address as key name: "blk_0000", "blk_0001", etc.
    char key[16];
    snprintf(key, sizeof(key), "blk_%04X", static_cast<unsigned>(address));

    size_t required_size = length;
    err = nvs_get_blob(handle, key, buffer, &required_size);
    nvs_close(handle);

    if (err == ESP_ERR_NVS_NOT_FOUND) {
      // Key doesn't exist — simulate empty flash (0xFF)
      memset(buffer, 0xFF, length);
      return core::Result<size_t>(length);
    }

    if (err != ESP_OK) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::HardwareFailure));
    }

    return core::Result<size_t>(required_size);
  }

  core::Result<size_t> write(uint32_t address, const uint8_t *data,
                             size_t length) noexcept override {
    if (GS_UNLIKELY(data == nullptr || length == 0)) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    }

    if (length > MAX_BLOB_SIZE) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::HardwareFailure));
    }

    char key[16];
    snprintf(key, sizeof(key), "blk_%04X", static_cast<unsigned>(address));

    err = nvs_set_blob(handle, key, data, length);
    if (err == ESP_OK) {
      err = nvs_commit(handle);
    }

    nvs_close(handle);

    if (err != ESP_OK) {
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::HardwareFailure));
    }

    return core::Result<size_t>(length);
  }

  core::Result<void> erase(uint32_t address, size_t length) noexcept override {
    (void)length; // NVS erases by key, not by range

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
      return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
    }

    char key[16];
    snprintf(key, sizeof(key), "blk_%04X", static_cast<unsigned>(address));

    err = nvs_erase_key(handle, key);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) {
      nvs_commit(handle);
    }

    nvs_close(handle);
    return core::Result<void>();
  }

private:
  bool initialized_;
};

// ============================================================================
// ESP32 WATCHDOG — Task Watchdog Timer
// ============================================================================
class Esp32Watchdog {
public:
  static constexpr uint32_t DEFAULT_TIMEOUT_S = 30;

  /**
   * @brief Initialize Task WDT and subscribe the current task.
   */
  static core::Result<void> init(uint32_t timeout_s = DEFAULT_TIMEOUT_S) noexcept {
    // Configure Task WDT
    esp_task_wdt_config_t config = {
        .timeout_ms = timeout_s * 1000,
        .idle_core_mask = 0, // Don't watch idle tasks
        .trigger_panic = true,
    };

    esp_err_t err = esp_task_wdt_reconfigure(&config);
    if (err != ESP_OK) {
      // First time — init instead of reconfigure
      err = esp_task_wdt_init(&config);
      if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
      }
    }

    // Subscribe current task to WDT
    err = esp_task_wdt_add(nullptr);
    if (err != ESP_OK && err != ESP_ERR_INVALID_ARG) {
      return GS_MAKE_ERROR(core::ErrorCode::HardwareFailure);
    }

    return core::Result<void>();
  }

  /**
   * @brief Feed the watchdog — call periodically in the main loop.
   */
  static void feed() noexcept {
    esp_task_wdt_reset();
  }
};

} // namespace esp32
} // namespace platform
} // namespace gridshield
