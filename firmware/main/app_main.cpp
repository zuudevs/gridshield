/**
 * @file qemu_main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief QEMU simulation entry point using pure ESP-IDF APIs
 * @version 1.0
 * @date 2026-02-23
 *
 * This file provides a QEMU-compatible app_main() that doesn't depend
 * on the Arduino framework. It uses ESP-IDF native APIs and the
 * GridShield mock platform layer.
 *
 * Build:  idf.py build
 * Run:    idf.py qemu monitor
 *
 * @copyright Copyright (c) 2026
 */

#if defined(GS_QEMU_BUILD) // Only compile for QEMU simulation builds

#include "core/system.hpp"
#include "core/config_manager.hpp"
#include "platform/esp32_platform.hpp"
#include "platform/mock_platform.hpp"

#include <cstdio>
#include <cstring>

// ESP-IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "GridShield";

// Forward declaration for ESP-IDF entry point
extern "C" void app_main(void);

using namespace gridshield;

// ============================================================================
// PLATFORM INSTANCES (static, no heap)
// ============================================================================
static platform::mock::MockTime mock_time;
static platform::mock::MockGPIO mock_gpio;
static platform::mock::MockInterrupt mock_interrupt;
static platform::mock::MockComm mock_comm;

// Real ESP32 crypto (Hardware RNG + mbedTLS SHA-256 + CRC32)
static platform::esp32::Esp32Crypto esp32_crypto;

// Real ESP32 NVS storage (persists across reboots on real HW, RAM-backed on QEMU)
static platform::esp32::Esp32Storage esp32_storage;



static platform::PlatformServices services;
static GridShieldSystem *system_ptr = nullptr;



// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
static SystemConfig create_config() {
  SystemConfig config;
  config.meter_id = 0x1234567890ABCDEF;
  config.heartbeat_interval_ms = 60000;
  config.reading_interval_ms = 5000;
  config.tamper_config.sensor_pin = 4; // GPIO4
  config.tamper_config.debounce_ms = 50;

  for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
    config.baseline_profile.hourly_avg_wh[i] = 1200;
  }
  config.baseline_profile.daily_avg_wh = 1200;
  config.baseline_profile.variance_threshold = 30;

  return config;
}

// ============================================================================
// CONFIGURATION VALIDATION
// ============================================================================
static bool validate_config(const SystemConfig &config) {
  bool valid = true;

  if (config.meter_id == 0) {
    ESP_LOGE(TAG, "Config error: meter_id must not be zero");
    valid = false;
  }

  if (config.reading_interval_ms == 0) {
    ESP_LOGE(TAG, "Config error: reading_interval_ms must be > 0");
    valid = false;
  }

  if (config.heartbeat_interval_ms == 0) {
    ESP_LOGE(TAG, "Config error: heartbeat_interval_ms must be > 0");
    valid = false;
  }

  if (config.heartbeat_interval_ms <= config.reading_interval_ms) {
    ESP_LOGW(TAG, "Config warning: heartbeat <= reading interval");
  }

  if (config.baseline_profile.variance_threshold == 0) {
    ESP_LOGW(TAG, "Config warning: variance_threshold is 0");
  }

  if (config.tamper_config.sensor_pin == 0) {
    ESP_LOGW(TAG, "Config warning: sensor_pin is 0");
  }

  return valid;
}

// ============================================================================
// ESP-IDF ENTRY POINT
// ============================================================================
void app_main(void) {
  ESP_LOGI(TAG, "==============================================");
  ESP_LOGI(TAG, "GridShield v2.1 [ESP32 - QEMU Simulation]");
  ESP_LOGI(TAG, "Platform: ESP-IDF + QEMU + mbedTLS");
  ESP_LOGI(TAG, "==============================================");

  // Initialize NVS storage
  auto nvs_result = esp32_storage.init();
  if (nvs_result.is_error()) {
    ESP_LOGE(TAG, "NVS init failed (code=%d)", static_cast<int>(nvs_result.error().code));
    return;
  }
  ESP_LOGI(TAG, "NVS storage initialized");

  // Initialize Watchdog Timer (30s timeout)
  auto wdt_result = platform::esp32::Esp32Watchdog::init(30);
  if (wdt_result.is_error()) {
    ESP_LOGW(TAG, "Watchdog init failed (code=%d) — continuing", static_cast<int>(wdt_result.error().code));
  } else {
    ESP_LOGI(TAG, "Watchdog timer initialized (30s)");
  }

  // Assemble platform services (real crypto + NVS, mock GPIO/Interrupt/Comm)
  services.time = &mock_time;
  services.gpio = &mock_gpio;
  services.interrupt = &mock_interrupt;
  services.crypto = &esp32_crypto;
  services.storage = &esp32_storage;
  services.comm = &mock_comm;

  // Load config: try NVS first, fallback to compiled defaults
  core::ConfigManager config_mgr(services);
  SystemConfig default_config = create_config();
  SystemConfig config = config_mgr.load_or_default(default_config);
  ESP_LOGI(TAG, "Config loaded (meter_id=0x%llx)",
           static_cast<unsigned long long>(config.meter_id));

  if (!validate_config(config)) {
    ESP_LOGE(TAG, "FATAL: Configuration validation failed");
    return;
  }

  // Persist validated config for next boot
  auto save_res = config_mgr.save(config);
  if (save_res.is_error()) {
    ESP_LOGW(TAG, "Failed to save config to NVS (code=%d)",
             static_cast<int>(save_res.error().code));
  }
  ESP_LOGI(TAG, "Configuration validated and persisted");

  // Create system
  system_ptr = new GridShieldSystem();
  if (system_ptr == nullptr) {
    ESP_LOGE(TAG, "FATAL: Out of memory");
    return;
  }

  // Initialize
  auto result = system_ptr->initialize(config, services);
  if (result.is_error()) {
    ESP_LOGE(TAG, "Init failed (code=%d)", static_cast<int>(result.error().code));
    return;
  }

  // Start
  result = system_ptr->start();
  if (result.is_error()) {
    ESP_LOGE(TAG, "Start failed (code=%d)", static_cast<int>(result.error().code));
    return;
  }

  ESP_LOGI(TAG, "System started successfully");
  ESP_LOGI(TAG, "Entering main processing loop...");

  // Main loop with cycle counter
  int cycle = 0;
  const int max_cycles = 20; // Run 20 cycles then exit for simulation

  while (cycle < max_cycles) {
    result = system_ptr->process_cycle();
    if (result.is_error()) {
      ESP_LOGW(TAG, "Process cycle %d error (code=%d)", cycle + 1,
               static_cast<int>(result.error().code));
    } else {
      ESP_LOGI(TAG, "Cycle %d/%d OK", cycle + 1, max_cycles);
    }

    // Feed watchdog
    platform::esp32::Esp32Watchdog::feed();

    // Use FreeRTOS delay for proper QEMU time advancement
    vTaskDelay(pdMS_TO_TICKS(100));

    ++cycle;
  }

  ESP_LOGI(TAG, "==============================================");
  ESP_LOGI(TAG, "Simulation complete — all cycles finished");
  ESP_LOGI(TAG, "==============================================");

  // Cleanup
  system_ptr->shutdown();
  delete system_ptr;
  system_ptr = nullptr;
}

#endif // GS_QEMU_BUILD
