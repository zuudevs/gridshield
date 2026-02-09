/**
 * @file system.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Main system orchestrator coordinating all security layers
 * @version 0.3
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "../platform/platform.hpp"
#include "hardware/tamper.hpp"
#include "security/crypto.hpp"
#include "network/packet.hpp"
#include "analytics/detector.hpp"

namespace gridshield {

// ============================================================================
// SYSTEM CONFIGURATION
// ============================================================================
struct SystemConfig {
    core::meter_id_t meter_id;
    hardware::TamperConfig tamper_config;
    analytics::ConsumptionProfile baseline_profile;
    uint32_t heartbeat_interval_ms;
    uint32_t reading_interval_ms;
    
    GS_CONSTEXPR SystemConfig() noexcept
        : meter_id(0), heartbeat_interval_ms(60000), reading_interval_ms(5000) {}
};

// ============================================================================
// OPERATION MODE
// ============================================================================
enum class OperationMode : uint8_t {
    Normal = 0,
    TamperResponse = 1,
    LowPower = 2,
    Maintenance = 3
};

// ============================================================================
// GRIDSHIELD SYSTEM (Main Orchestrator)
// ============================================================================
class GridShieldSystem {
public:
    GridShieldSystem() noexcept;
    ~GridShieldSystem() noexcept;
    
    // Non-copyable, non-movable
    GridShieldSystem(const GridShieldSystem&) = delete;
    GridShieldSystem& operator=(const GridShieldSystem&) = delete;
    GridShieldSystem(GridShieldSystem&&) = delete;
    GridShieldSystem& operator=(GridShieldSystem&&) = delete;
    
    // Lifecycle management
    core::Result<void> initialize(const SystemConfig& config,
                                 platform::PlatformServices& platform) noexcept;
    
    core::Result<void> start() noexcept;
    core::Result<void> stop() noexcept;
    core::Result<void> shutdown() noexcept;
    
    // Main processing loop
    core::Result<void> process_cycle() noexcept;
    
    // State queries
    GS_NODISCARD core::SystemState get_state() const noexcept { return state_; }
    GS_NODISCARD OperationMode get_mode() const noexcept { return mode_; }
    
    // Operations
    core::Result<void> send_meter_reading(const core::MeterReading& reading) noexcept;
    core::Result<void> send_tamper_alert() noexcept;
    core::Result<void> send_heartbeat() noexcept;
    
private:
    core::Result<void> initialize_crypto() noexcept;
    core::Result<void> handle_tamper_event() noexcept;
    core::Result<void> perform_cross_layer_validation() noexcept;
    
    void transition_state(core::SystemState new_state) noexcept;
    void set_mode(OperationMode new_mode) noexcept;
    
    SystemConfig config_;
    platform::PlatformServices* platform_;
    
    // Layer components
    hardware::TamperDetector tamper_detector_;
    security::CryptoEngine* crypto_engine_;
    security::ECCKeyPair device_keypair_;
    security::ECCKeyPair server_public_key_;
    network::PacketTransport* packet_transport_;
    analytics::AnomalyDetector anomaly_detector_;
    
    // State management
    core::SystemState state_;
    OperationMode mode_;
    bool initialized_;
    
    // Timing
    core::timestamp_t last_heartbeat_;
    core::timestamp_t last_reading_;
    
    // Cross-layer validation
    analytics::CrossLayerValidation validation_state_;
};

} // namespace gridshield