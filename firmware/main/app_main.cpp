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
#include "platform/esp32_platform.hpp"
#include "platform/mock_platform.hpp"

#include <cstdio>
#include <cstring>

// ESP-IDF includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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
// HELPER: printf-based logging (captured by QEMU UART)
// ============================================================================
static void log_info(const char *msg) { printf("[GridShield] %s\n", msg); }

static void log_error(const char *msg, int code) {
  printf("[GridShield] ERROR: %s (code=%d)\n", msg, code);
}

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
// ESP-IDF ENTRY POINT
// ============================================================================
void app_main(void) {
  log_info("==============================================");
  log_info("GridShield v2.1 [ESP32 - QEMU Simulation]");
  log_info("Platform: ESP-IDF + QEMU + mbedTLS");
  log_info("==============================================");

  // Initialize NVS storage
  auto nvs_result = esp32_storage.init();
  if (nvs_result.is_error()) {
    log_error("NVS init failed", static_cast<int>(nvs_result.error().code));
    return;
  }
  log_info("NVS storage initialized");

  // Initialize Watchdog Timer (30s timeout)
  auto wdt_result = platform::esp32::Esp32Watchdog::init(30);
  if (wdt_result.is_error()) {
    log_error("Watchdog init failed", static_cast<int>(wdt_result.error().code));
    // Non-fatal — continue without watchdog
  } else {
    log_info("Watchdog timer initialized (30s)");
  }

  // Assemble platform services (real crypto + NVS, mock GPIO/Interrupt/Comm)
  services.time = &mock_time;
  services.gpio = &mock_gpio;
  services.interrupt = &mock_interrupt;
  services.crypto = &esp32_crypto;
  services.storage = &esp32_storage;
  services.comm = &mock_comm;

  // Create system
  system_ptr = new GridShieldSystem();
  if (system_ptr == nullptr) {
    log_info("FATAL: Out of memory");
    return;
  }

  // Initialize
  SystemConfig config = create_config();
  auto result = system_ptr->initialize(config, services);
  if (result.is_error()) {
    log_error("Init failed", static_cast<int>(result.error().code));
    return;
  }

  // Start
  result = system_ptr->start();
  if (result.is_error()) {
    log_error("Start failed", static_cast<int>(result.error().code));
    return;
  }

  log_info("System started successfully");
  log_info("Entering main processing loop...");

  // Main loop with cycle counter
  int cycle = 0;
  const int max_cycles = 20; // Run 20 cycles then exit for simulation

  while (cycle < max_cycles) {
    result = system_ptr->process_cycle();
    if (result.is_error()) {
      log_error("Process cycle error", static_cast<int>(result.error().code));
    } else {
      char buf[64];
      snprintf(buf, sizeof(buf), "Cycle %d/%d OK", cycle + 1, max_cycles);
      log_info(buf);
    }

    // Feed watchdog
    platform::esp32::Esp32Watchdog::feed();

    // Use FreeRTOS delay for proper QEMU time advancement
    vTaskDelay(pdMS_TO_TICKS(100));

    ++cycle;
  }

  log_info("==============================================");
  log_info("Simulation complete — all cycles finished");
  log_info("==============================================");

  // Cleanup
  system_ptr->shutdown();
  delete system_ptr;
  system_ptr = nullptr;
}

#endif // GS_QEMU_BUILD
