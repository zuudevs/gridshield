/**
 * @file example_detector.cpp
 * @brief GridShield AnomalyDetector — Usage Example
 *
 * Demonstrates:
 *   - Detector initialization with baseline consumption profile
 *   - Analyzing meter readings for anomalies
 *   - Updating the consumption profile
 *   - Interpreting anomaly reports (severity, confidence, type)
 */

#include "analytics/detector.hpp"
#include "core/types.hpp"

using namespace gridshield;
using namespace gridshield::analytics;

void example_detector() {
  // ---------------------------------------------------------------
  // 1. Create Baseline Consumption Profile
  // ---------------------------------------------------------------
  // The baseline represents "normal" meter behavior for this
  // installation. It adapts over time via update_profile().
  ConsumptionProfile baseline;
  baseline.avg_energy_wh = 1000;    // Average hourly consumption
  baseline.avg_voltage_mv = 220000; // 220V nominal
  baseline.avg_current_ma = 4545;   // ~1kW at 220V
  baseline.avg_power_factor = 950;  // 0.95 PF (×1000)
  baseline.sample_count = 100;      // Profile maturity

  // ---------------------------------------------------------------
  // 2. Initialize Detector
  // ---------------------------------------------------------------
  AnomalyDetector detector;

  auto init_result = detector.initialize(baseline);
  if (init_result.is_error()) {
    return;
  }

  // ---------------------------------------------------------------
  // 3. Analyze Normal Meter Reading
  // ---------------------------------------------------------------
  core::MeterReading normal_reading;
  normal_reading.timestamp = 1000;
  normal_reading.energy_wh = 1050;    // Slightly above average — normal
  normal_reading.voltage_mv = 219500; // Within tolerance
  normal_reading.current_ma = 4780;
  normal_reading.power_factor = 945;

  auto normal_result = detector.analyze(normal_reading);
  if (normal_result.is_ok()) {
    const AnomalyReport &report = normal_result.value();
    // report.severity    → AnomalySeverity::None or Low
    // report.confidence  → 0-100 (confidence percentage)
    // report.type        → AnomalyType::None
    (void)report;
  }

  // ---------------------------------------------------------------
  // 4. Analyze Suspicious Reading (voltage anomaly)
  // ---------------------------------------------------------------
  core::MeterReading suspicious_reading;
  suspicious_reading.timestamp = 2000;
  suspicious_reading.energy_wh = 0;      // Zero consumption — meter bypass?
  suspicious_reading.voltage_mv = 50000; // Abnormally low voltage
  suspicious_reading.current_ma = 0;
  suspicious_reading.power_factor = 0;

  auto suspicious_result = detector.analyze(suspicious_reading);
  if (suspicious_result.is_ok()) {
    const AnomalyReport &report = suspicious_result.value();

    // Check severity levels:
    //   AnomalySeverity::None
    //   AnomalySeverity::Low
    //   AnomalySeverity::Medium
    //   AnomalySeverity::High       ← Requires investigation
    //   AnomalySeverity::Critical   ← Immediate action needed

    if (report.severity >= AnomalySeverity::High) {
      // Trigger alert — possible meter tampering
    }

    // Check anomaly type:
    //   AnomalyType::None
    //   AnomalyType::ConsumptionAnomaly
    //   AnomalyType::VoltageAnomaly
    //   AnomalyType::CurrentAnomaly
    //   AnomalyType::PowerFactorAnomaly

    // Confidence tells how certain the detection is (0-100)
    if (report.confidence >= CONFIDENCE_HIGH) {
      // High confidence — proceed with automated response
    }
  }

  // ---------------------------------------------------------------
  // 5. Update Profile (adaptive learning)
  // ---------------------------------------------------------------
  // Feed normal readings to improve baseline accuracy over time
  auto update_result = detector.update_profile(normal_reading);
  (void)update_result;

  // ---------------------------------------------------------------
  // 6. Get Current Profile
  // ---------------------------------------------------------------
  const ConsumptionProfile &current = detector.get_profile();
  // current.avg_energy_wh   → updated running average
  // current.sample_count    → incremented
  (void)current;
}
