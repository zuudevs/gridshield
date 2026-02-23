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
#include "platform/mock_platform.hpp"

#include <cstdio>
#include <cstring>

// ESP-IDF includes for FreeRTOS delay
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
static platform::mock::MockCrypto mock_crypto;
static platform::mock::MockComm mock_comm;

// Mock storage (simple RAM-backed storage)
class QemuStorage final : public platform::IPlatformStorage {
public:
  static constexpr size_t STORAGE_SIZE = 4096;

  QemuStorage() noexcept { memset(storage_, 0xFF, STORAGE_SIZE); }

  core::Result<size_t> read(uint32_t address, uint8_t *buffer,
                            size_t length) noexcept override {
    if (buffer == nullptr || length == 0)
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    if (address + length > STORAGE_SIZE)
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
    memcpy(buffer, &storage_[address], length);
    return core::Result<size_t>(length);
  }

  core::Result<size_t> write(uint32_t address, const uint8_t *data,
                             size_t length) noexcept override {
    if (data == nullptr || length == 0)
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::InvalidParameter));
    if (address + length > STORAGE_SIZE)
      return core::Result<size_t>(
          GS_MAKE_ERROR(core::ErrorCode::BufferOverflow));
    memcpy(&storage_[address], data, length);
    return core::Result<size_t>(length);
  }

  core::Result<void> erase(uint32_t address, size_t length) noexcept override {
    if (address + length > STORAGE_SIZE)
      return GS_MAKE_ERROR(core::ErrorCode::BufferOverflow);
    memset(&storage_[address], 0xFF, length);
    return core::Result<void>();
  }

private:
  uint8_t storage_[STORAGE_SIZE];
};

static QemuStorage qemu_storage;
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
  log_info("GridShield v1.1 [ESP32 - QEMU Simulation]");
  log_info("Platform: ESP-IDF + QEMU");
  log_info("==============================================");

  // Assemble platform services
  services.time = &mock_time;
  services.gpio = &mock_gpio;
  services.interrupt = &mock_interrupt;
  services.crypto = &mock_crypto;
  services.storage = &qemu_storage;
  services.comm = &mock_comm;

  // Initialize mock communication
  auto comm_result = mock_comm.init();
  if (comm_result.is_error()) {
    log_error("Comm init failed", static_cast<int>(comm_result.error().code));
    return;
  }

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
