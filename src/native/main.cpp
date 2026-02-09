/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Native platform entry point for development/testing
 * @version 0.4
 * @date 2026-02-08
 * 
 * @copyright Copyright (c) 2026
 */

#include "core/system.hpp"
#include "platform_native.hpp"

#if !GS_PLATFORM_NATIVE
#error "This file is for NATIVE builds only"
#endif

#include <iostream>
#include <iomanip>

#if defined(_WIN32)
#include <windows.h>
#endif

using namespace gridshield;

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================
static void setup_utf8_console() noexcept {
#if defined(_WIN32)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    std::setvbuf(stdout, nullptr, _IOFBF, 1024);
#endif
}

static void print_header(const char* title) {
    std::cout << "\n[" << title << "]\n";
    std::cout << std::string(50, '─') << "\n";
}

static void print_error(const core::ErrorContext& error) {
    std::cerr << "ERROR: Code " << static_cast<int>(error.code);
    if (error.file) {
        std::cerr << " at " << error.file << ":" << error.line;
    }
    std::cerr << "\n";
}

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
static SystemConfig create_config() {
    SystemConfig config;
    
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;  // 1 minute
    config.reading_interval_ms = 5000;     // 5 seconds
    
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    // Initialize baseline profile
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 
            1000 + static_cast<uint32_t>(i * 50);
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;
    config.baseline_profile.profile_confidence = 80;
    
    return config;
}

// ============================================================================
// MAIN
// ============================================================================
int main() {
    setup_utf8_console();
    
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  GridShield AMI Security System v1.0.0\n";
    std::cout << "  Multi-Layer Protection for Advanced Metering Infrastructure\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";

    // Platform services
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::native::NativeInterrupt interrupt;
    platform::native::NativeCrypto crypto;
    platform::native::NativeComm comm;
    
    platform::PlatformServices services;
    services.time = &time;
    services.gpio = &gpio;
    services.interrupt = &interrupt;
    services.crypto = &crypto;
    services.comm = &comm;
    services.storage = nullptr;
    
    // Initialize communication
    auto init_result = comm.init();
    if (init_result.is_error()) {
        print_error(init_result.error());
        return 1;
    }
    
    // System configuration
    SystemConfig config = create_config();
    
    // Create system
    GridShieldSystem system;
    
    print_header("Initialization");
    std::cout << "Meter ID: 0x" << std::hex << std::uppercase 
              << config.meter_id << std::dec << "\n";
    
    auto result = system.initialize(config, services);
    if (result.is_error()) {
        print_error(result.error());
        return 1;
    }
    std::cout << "✓ System initialized successfully\n";
    
    // Start system
    result = system.start();
    if (result.is_error()) {
        print_error(result.error());
        return 1;
    }
    std::cout << "✓ System started\n";
    
    // Phase 1: Normal operation
    print_header("Phase 1: Normal Operation");
    for (int i = 0; i < 3; ++i) {
        result = system.process_cycle();
        if (result.is_error()) {
            print_error(result.error());
        } else {
            std::cout << "Cycle " << (i + 1) << ": ✓ Processing complete\n";
        }
        time.delay_ms(1000);
    }
    
    // Phase 2: Tamper detection
    print_header("Phase 2: Tamper Detection");
    std::cout << "Simulating physical tamper event...\n";
    
    gpio.simulate_trigger(config.tamper_config.sensor_pin, true);
    interrupt.simulate_interrupt(config.tamper_config.sensor_pin);
    
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
    
    // Phase 3: Consumption anomaly
    print_header("Phase 3: Consumption Anomaly Detection");
    std::cout << "Sending normal consumption readings...\n";
    
    core::MeterReading reading;
    reading.voltage_mv = 220000;
    reading.current_ma = 4545;
    reading.power_factor = 950;
    
    for (int i = 0; i < 3; ++i) {
        reading.energy_wh = 1000 + static_cast<uint32_t>(i * 10);
        reading.timestamp = time.get_timestamp_ms();
        
        result = system.send_meter_reading(reading);
        if (result.is_ok()) {
            std::cout << "  Reading " << (i + 1) << ": " 
                      << reading.energy_wh << " Wh ✓\n";
        }
        time.delay_ms(500);
    }
    
    std::cout << "Simulating anomalous consumption drop...\n";
    reading.energy_wh = 100;
    reading.timestamp = time.get_timestamp_ms();
    
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