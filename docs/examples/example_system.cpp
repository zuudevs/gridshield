/**
 * @file example_system.cpp
 * @brief GridShield System Module — Usage Example
 *
 * Demonstrates the full lifecycle of GridShieldSystem:
 *   init → start → process_cycle → send_meter_reading → stop → shutdown
 */

#include "core/system.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;

void example_system() {
  // ---------------------------------------------------------------
  // 1. Setup Platform Services (replace with real HW on production)
  // ---------------------------------------------------------------
  platform::mock::MockTime mock_time;
  platform::mock::MockGPIO mock_gpio;
  platform::mock::MockCrypto mock_crypto;
  platform::mock::MockComm mock_comm;
  platform::mock::MockInterrupt mock_interrupt;
  platform::mock::MockStorage mock_storage;

  platform::PlatformServices services;
  services.time = &mock_time;
  services.gpio = &mock_gpio;
  services.crypto = &mock_crypto;
  services.comm = &mock_comm;
  services.interrupt = &mock_interrupt;
  services.storage = &mock_storage;

  // ---------------------------------------------------------------
  // 2. Configure System
  // ---------------------------------------------------------------
  SystemConfig config;
  config.meter_id = 0x1234567890ABCDEF;
  config.heartbeat_interval_ms = 60000; // 1 minute
  config.reading_interval_ms = 30000;   // 30 seconds

  // Tamper detection config
  config.tamper_config.sensor_pin = 4;
  config.tamper_config.debounce_ms = 50;
  config.tamper_config.confirm_ms = 200;

  // ---------------------------------------------------------------
  // 3. Initialize & Start
  // ---------------------------------------------------------------
  GridShieldSystem system;

  auto init_result = system.initialize(config, services);
  if (init_result.is_error()) {
    // Handle initialization error
    return;
  }

  auto start_result = system.start();
  if (start_result.is_error()) {
    return;
  }

  // ---------------------------------------------------------------
  // 4. Main Loop — process cycle handles heartbeat, tamper, readings
  // ---------------------------------------------------------------
  for (int i = 0; i < 100; ++i) {
    auto cycle_result = system.process_cycle();
    if (cycle_result.is_error()) {
      // Log and continue or break
    }

    // Check system state
    if (system.get_state() == core::SystemState::Tampered) {
      // System detected physical tampering
      // process_cycle already sent tamper alert
      break;
    }
  }

  // ---------------------------------------------------------------
  // 5. Send Manual Meter Reading
  // ---------------------------------------------------------------
  core::MeterReading reading;
  reading.timestamp = mock_time.get_timestamp_ms();
  reading.energy_wh = 1500;
  reading.voltage_mv = 220000;
  reading.current_ma = 6818;
  reading.power_factor = 990;

  auto read_result = system.send_meter_reading(reading);
  // NOTE: Non-critical — log error but don't crash

  // ---------------------------------------------------------------
  // 6. Shutdown
  // ---------------------------------------------------------------
  system.stop();
  system.shutdown();
}
