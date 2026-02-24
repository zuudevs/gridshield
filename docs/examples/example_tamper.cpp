/**
 * @file example_tamper.cpp
 * @brief GridShield TamperDetector — Usage Example
 *
 * Demonstrates:
 *   - Tamper detector initialization
 *   - Starting tamper monitoring
 *   - Polling for deferred tamper events (ISR → poll pattern)
 *   - Querying tamper state and type
 *   - Acknowledging tamper events
 */

#include "hardware/tamper.hpp"
#include "platform/mock_platform.hpp"

using namespace gridshield;
using namespace gridshield::hardware;

void example_tamper() {
  // ---------------------------------------------------------------
  // 1. Setup Platform Services
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
  // 2. Configure Tamper Detection
  // ---------------------------------------------------------------
  TamperConfig config;
  config.sensor_pin = 4;   // GPIO pin connected to tamper switch
  config.debounce_ms = 50; // Debounce window (ms)
  config.confirm_ms = 200; // Confirmation delay after ISR trigger

  // ---------------------------------------------------------------
  // 3. Initialize & Start
  // ---------------------------------------------------------------
  TamperDetector detector;

  auto init_result = detector.initialize(config, services);
  if (init_result.is_error()) {
    return;
  }

  auto start_result = detector.start();
  if (start_result.is_error()) {
    return;
  }

  // ---------------------------------------------------------------
  // 4. Simulate ISR Trigger (test environment only)
  // ---------------------------------------------------------------
  // In production, a physical tamper switch triggers the ISR via
  // hardware interrupt. For testing, use MockInterrupt:
  mock_interrupt.simulate_interrupt(config.sensor_pin);

  // ---------------------------------------------------------------
  // 5. Poll for Deferred Tamper Events
  // ---------------------------------------------------------------
  // NOTE: The ISR only sets a flag. poll() performs the actual
  // debounce confirmation in a non-ISR context (safe for logging).
  auto poll_result = detector.poll();
  (void)poll_result;

  // ---------------------------------------------------------------
  // 6. Check Tamper State
  // ---------------------------------------------------------------
  if (detector.is_tampered()) {
    // Tamper detected and confirmed!

    // Get tamper type
    TamperType type = detector.get_tamper_type();
    // Possible values:
    //   TamperType::None
    //   TamperType::CasingOpened
    //   TamperType::MagneticInterference
    //   TamperType::TemperatureAnomaly
    //   TamperType::VibrationDetected
    //   TamperType::PowerCutAttempt
    //   TamperType::PhysicalShock
    (void)type;

    // Get timestamp of tamper event
    core::timestamp_t when = detector.get_tamper_timestamp();
    (void)when;

    // Acknowledge the tamper event
    auto ack_result = detector.acknowledge_tamper();
    (void)ack_result;
  }

  // ---------------------------------------------------------------
  // 7. Stop Monitoring
  // ---------------------------------------------------------------
  auto stop_result = detector.stop();
  (void)stop_result;
}
