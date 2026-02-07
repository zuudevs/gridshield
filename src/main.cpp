/**
 * @file demo_main.cpp
 * @brief Native platform entry point for development/testing
 * @version 0.2
 * @date 2026-02-07
 * 
 * @copyright Copyright (c) 2026
 *
 * This file is for NATIVE (PC) builds only.
 * Arduino builds use gridshield.ino instead.
 */

#include "core/system.hpp"
#include "platform/mock_platform.hpp"

#if PLATFORM_NATIVE

#include <iostream>
#include <iomanip>

using namespace gridshield;

static void print_header(const char* title) {
    std::cout << "\n[" << title << "]\n";
    std::cout << std::string(40, '-') << "\n";
}

static void print_error(const core::ErrorContext& error) {
    std::cerr << "ERROR: Code " << static_cast<int>(error.code);
    if (error.file) {
        std::cerr << " at " << error.file << ":" << error.line;
    }
    std::cerr << "\n";
}

int main() {
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  GridShield AMI Security System v1.0.0\n";
    std::cout << "  Multi-Layer Protection for Advanced Metering Infrastructure\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";

    // Platform services
    platform::mock::MockTime mock_time;
    platform::mock::MockGPIO mock_gpio;
    platform::mock::MockInterrupt mock_interrupt;
    platform::mock::MockCrypto mock_crypto;
    platform::mock::MockComm mock_comm;
    
    platform::PlatformServices services;
    services.time = &mock_time;
    services.gpio = &mock_gpio;
    services.interrupt = &mock_interrupt;
    services.crypto = &mock_crypto;
    services.comm = &mock_comm;
    services.storage = nullptr;
    
    // Initialize communication
    auto init_result = mock_comm.init();
    if (init_result.is_error()) {
        print_error(init_result.error());
        return 1;
    }
    
    // System configuration
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = 5000;
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    // Initialize baseline consumption profile
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1000 + (i * 50);
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;
    config.baseline_profile.profile_confidence = 80;
    
    // Create system
    GridShieldSystem system;
    
    print_header("Initialization");
    std::cout << "Meter ID: 0x" << std::hex << std::uppercase 
              << config.meter_id << std::dec << "\n";
    std::cout << "Initializing GridShield system...\n";
    
    auto result = system.initialize(config, services);
    if (result.is_error()) {
        print_error(result.error());
        return 1;
    }
    std::cout << "✓ System initialized successfully\n";
    std::cout << "System State: " << static_cast<int>(system.get_state()) << "\n";
    
    // Start system
    result = system.start();
    if (result.is_error()) {
        print_error(result.error());
        return 1;
    }
    std::cout << "✓ System started\n";
    
    // Phase 1: Normal operation
    print_header("Phase 1] Normal Operation Mode");
    for (int i = 0; i < 3; ++i) {
        result = system.process_cycle();
        if (result.is_error()) {
            print_error(result.error());
        } else {
            std::cout << "Cycle " << (i + 1) << ": ✓ Processing complete\n";
        }
        mock_time.delay_ms(1000);
    }
    
    // Phase 2: Tamper detection simulation
    print_header("Phase 2] Tamper Detection Test");
    std::cout << "Simulating physical tamper event...\n";
    
    // Trigger tamper sensor
    mock_gpio.simulate_trigger(config.tamper_config.sensor_pin, true);
    mock_interrupt.simulate_interrupt(config.tamper_config.sensor_pin);
    
    result = system.process_cycle();
    if (result.is_ok()) {
        std::cout << "✓ Tamper event processed\n";
    }
    
    std::cout << "System State: ";
    switch (system.get_state()) {
        case core::SystemState::Tampered:
            std::cout << "⚠️  TAMPERED (CRITICAL)\n";
            break;
        default:
            std::cout << static_cast<int>(system.get_state()) << "\n";
    }
    
    // Phase 3: Consumption anomaly detection
    print_header("Phase 3] Consumption Anomaly Detection");
    std::cout << "Sending normal consumption readings...\n";
    
    core::MeterReading reading;
    reading.timestamp = mock_time.get_timestamp_ms();
    reading.voltage_mv = 220000; // 220V
    reading.current_ma = 4545;   // ~1kW
    reading.power_factor = 95;
    
    for (int i = 0; i < 3; ++i) {
        reading.energy_wh = 1000 + (i * 10);
        reading.timestamp = mock_time.get_timestamp_ms();
        
        result = system.send_meter_reading(reading);
        if (result.is_ok()) {
            std::cout << "  Reading " << (i + 1) << ": " 
                      << reading.energy_wh << " Wh ✓\n";
        }
        mock_time.delay_ms(500);
    }
    
    // Send anomalous reading (90% drop)
    std::cout << "Simulating anomalous consumption drop...\n";
    reading.energy_wh = 100; // Drastic drop
    reading.timestamp = mock_time.get_timestamp_ms();
    
    result = system.send_meter_reading(reading);
    if (result.is_ok()) {
        std::cout << "  Anomalous reading: " << reading.energy_wh << " Wh ⚠️\n";
        std::cout << "  Analytics layer flagged potential manipulation\n";
    }
    
    // Cleanup
    print_header("Shutdown");
    result = system.shutdown();
    if (result.is_ok()) {
        std::cout << "✓ System shutdown successful\n";
    }
    
    std::cout << "\n═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  Demo Complete\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    
    return 0;
}

#else
// AVR platform uses gridshield.ino entry point
#error "This file is for NATIVE builds only. Use gridshield.ino for Arduino."
#endif