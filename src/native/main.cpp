/**
 * @file main.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Production native application
 * @version 1.0
 * @date 2026-02-11
 * 
 * @copyright Copyright (c) 2026
 */

#include "core/system.hpp"
#include "platform_native.hpp"

#if !GS_PLATFORM_NATIVE
#error "This file is for NATIVE builds only"
#endif

#include <iostream>
#include <csignal>
#include <atomic>

using namespace gridshield;

static std::atomic<bool> running{true};

void signal_handler(int) {
    running = false;
}

static SystemConfig create_config() {
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = 5000;
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1200;
    }
    config.baseline_profile.daily_avg_wh = 1200;
    config.baseline_profile.variance_threshold = 30;
    
    return config;
}

int main() {
    std::signal(SIGINT, signal_handler);
    
    std::cout << "GridShield v1.0 - Production Mode\n";
    
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
    
    auto init_result = comm.init();
    if (init_result.is_error()) {
        std::cerr << "FATAL: Communication init failed\n";
        return 1;
    }
    
    GridShieldSystem system;
    SystemConfig config = create_config();
    
    auto result = system.initialize(config, services);
    if (result.is_error()) {
        std::cerr << "FATAL: System init failed\n";
        return 1;
    }
    
    result = system.start();
    if (result.is_error()) {
        std::cerr << "FATAL: System start failed\n";
        return 1;
    }
    
    std::cout << "System running. Press Ctrl+C to stop.\n";
    
    while (running) {
        result = system.process_cycle();
        if (result.is_error()) {
            std::cerr << "ERROR: Cycle failed\n";
        }
        
        time.delay_ms(100);
    }
    
    system.shutdown();
    std::cout << "System stopped.\n";
    
    return 0;
}