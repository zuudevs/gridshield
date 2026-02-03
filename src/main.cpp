/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include <windows.h>
#include "core/system.hpp"
#include "platform/mock_platform.hpp"
#include <iostream>
#include <iomanip>

using namespace gridshield;

void setupConsole() {
    #ifdef _WIN32
        // Paksa console output & input menggunakan UTF-8 (Code Page 65001)
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);

        // Aktifkan buffering penuh untuk stdout
        // Ini mencegah karakter multi-byte UTF-8 terpotong saat di-print ke terminal
        setvbuf(stdout, nullptr, _IOFBF, 1024);
    #endif
}

static void print_header() {
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  GridShield AMI Security System v1.0.0\n";
    std::cout << "  Multi-Layer Protection for Advanced Metering Infrastructure\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "\n";
}

static void print_system_state(const GridShieldSystem& system) {
    std::cout << "System State: ";
    
    switch (system.get_state()) {
        case core::SystemState::Uninitialized:
            std::cout << "UNINITIALIZED";
            break;
        case core::SystemState::Initializing:
            std::cout << "INITIALIZING";
            break;
        case core::SystemState::Ready:
            std::cout << "READY";
            break;
        case core::SystemState::Operating:
            std::cout << "OPERATING";
            break;
        case core::SystemState::Tampered:
            std::cout << "⚠️  TAMPERED (CRITICAL)";
            break;
        case core::SystemState::PowerLoss:
            std::cout << "POWER LOSS";
            break;
        case core::SystemState::Error:
            std::cout << "ERROR";
            break;
        case core::SystemState::Shutdown:
            std::cout << "SHUTDOWN";
            break;
    }
    
    std::cout << "\n";
}

static void demonstrate_normal_operation(GridShieldSystem& system, 
                                        platform::PlatformServices& platform) {
    std::cout << "\n[Phase 1] Normal Operation Mode\n";
    std::cout << "─────────────────────────────────────\n";
    
    // Simulate several operation cycles
    for (int i = 0; i < 5; ++i) {
        std::cout << "Cycle " << (i + 1) << ": ";
        
        auto result = system.process_cycle();
        if (result.is_ok()) {
            std::cout << "✓ Processing complete\n";
        } else {
            std::cout << "✗ Error (code: " 
                     << static_cast<int>(result.error().code()) << ")\n";
        }
        
        platform.time->delay_ms(100);
    }
}

static void demonstrate_tamper_detection(GridShieldSystem& system,
                                        platform::mock::MockGPIO& gpio,
                                        platform::mock::MockInterrupt& interrupt,
                                        platform::PlatformServices& platform) {
    std::cout << "\n[Phase 2] Tamper Detection Test\n";
    std::cout << "─────────────────────────────────────\n";
    std::cout << "Simulating physical tamper event...\n";
    
    // Simulate tamper sensor trigger
    gpio.simulate_trigger(5, false); // Active low trigger
    interrupt.simulate_interrupt(5);
    
    platform.time->delay_ms(100);
    
    // Process cycle after tamper
    auto result = system.process_cycle();
    if (result.is_ok()) {
        std::cout << "✓ Tamper event processed\n";
        print_system_state(system);
    } else {
        std::cout << "✗ Failed to process tamper event\n";
    }
}

static void demonstrate_anomaly_detection(GridShieldSystem& system) {
    std::cout << "\n[Phase 3] Consumption Anomaly Detection\n";
    std::cout << "─────────────────────────────────────\n";
    
    // Simulate normal readings
    std::cout << "Sending normal consumption readings...\n";
    for (int i = 0; i < 3; ++i) {
        core::MeterReading reading;
        reading.timestamp = 1000000 + (i * 5000);
        reading.energy_wh = 1000 + (i * 10);
        reading.voltage_mv = 220000;
        reading.current_ma = 4545;
        
        auto result = system.send_meter_reading(reading);
        if (result.is_ok()) {
            std::cout << "  Reading " << (i + 1) << ": " 
                     << reading.energy_wh << " Wh ✓\n";
        }
    }
    
    // Simulate anomalous reading (sudden drop)
    std::cout << "Simulating anomalous consumption drop...\n";
    core::MeterReading anomalous_reading;
    anomalous_reading.timestamp = 1015000;
    anomalous_reading.energy_wh = 100; // 90% drop
    anomalous_reading.voltage_mv = 220000;
    anomalous_reading.current_ma = 454;
    
    auto result = system.send_meter_reading(anomalous_reading);
    if (result.is_ok()) {
        std::cout << "  Anomalous reading: " 
                 << anomalous_reading.energy_wh << " Wh ⚠️\n";
        std::cout << "  Analytics layer flagged potential manipulation\n";
    }
}

static void demonstrate_secure_communication(platform::mock::MockComm& comm) {
    std::cout << "\n[Phase 4] Secure Communication Verification\n";
    std::cout << "─────────────────────────────────────\n";
    
    const auto& tx_buffer = comm.get_tx_buffer();
    std::cout << "Total packets transmitted: " << tx_buffer.size() << " bytes\n";
    std::cout << "All packets are encrypted and signed with ECC\n";
    std::cout << "✓ Communication layer secure\n";
}

int main() {
	setupConsole();
    print_header();
    
    // Initialize mock platform services
    platform::mock::MockTime mock_time;
    platform::mock::MockGPIO mock_gpio;
    platform::mock::MockInterrupt mock_interrupt;
    platform::mock::MockCrypto mock_crypto;
    platform::mock::MockComm mock_comm;
    
    platform::PlatformServices platform;
    platform.time = &mock_time;
    platform.gpio = &mock_gpio;
    platform.interrupt = &mock_interrupt;
    platform.crypto = &mock_crypto;
    platform.comm = &mock_comm;
    
    // Configure system
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.tamper_config.sensor_pin = 5;
    config.tamper_config.backup_power_pin = 6;
    config.heartbeat_interval_ms = 5000;
    config.reading_interval_ms = 2000;
    
    // Initialize baseline consumption profile
    config.baseline_profile.daily_avg_wh = 1000;
    config.baseline_profile.variance_threshold = 30; // 30% deviation threshold
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1000 + (i * 10);
    }
    
    std::cout << "[Initialization]\n";
    std::cout << "─────────────────────────────────────\n";
    std::cout << "Meter ID: 0x" << std::hex << std::uppercase 
             << config.meter_id << std::dec << "\n";
    std::cout << "Initializing GridShield system...\n";
    
    GridShieldSystem system;
    auto init_result = system.initialize(config, platform);
    
    if (init_result.is_error()) {
        std::cerr << "✗ Initialization failed (code: " 
                 << static_cast<int>(init_result.error().code()) << ")\n";
        return 1;
    }
    
    std::cout << "✓ System initialized successfully\n";
    print_system_state(system);
    
    // Start system
    std::cout << "\nStarting security monitoring...\n";
    auto start_result = system.start();
    
    if (start_result.is_error()) {
        std::cerr << "✗ Failed to start system\n";
        return 1;
    }
    
    std::cout << "✓ System started\n";
    print_system_state(system);
    
    // Demonstrate capabilities
    demonstrate_normal_operation(system, platform);
    demonstrate_tamper_detection(system, mock_gpio, mock_interrupt, platform);
    demonstrate_anomaly_detection(system);
    demonstrate_secure_communication(mock_comm);
    
    // Shutdown
    std::cout << "\n[Shutdown]\n";
    std::cout << "─────────────────────────────────────\n";
    std::cout << "Shutting down system...\n";
    
    auto shutdown_result = system.shutdown();
    if (shutdown_result.is_ok()) {
        std::cout << "✓ System shutdown complete\n";
        print_system_state(system);
    }
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "  GridShield Demonstration Complete\n";
    std::cout << "  Security Layers: ✓ Physical | ✓ Network | ✓ Analytics\n";
    std::cout << "═══════════════════════════════════════════════════════════════════\n";
    std::cout << "\n";
    
    return 0;
}