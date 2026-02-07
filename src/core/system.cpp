/**
 * @file system.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief System orchestrator implementation
 * @version 0.2
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "core/system.hpp"

#if PLATFORM_NATIVE
    #include <new>
#else
    // NEED ADOPTION: Ensure placement new is available
    #include <new.h>
#endif

namespace gridshield {

GridShieldSystem::GridShieldSystem() noexcept
    : platform_(nullptr),
      crypto_engine_(nullptr),
      packet_transport_(nullptr),
      state_(core::SystemState::Uninitialized),
      mode_(OperationMode::Normal),
      initialized_(false),
      last_heartbeat_(0),
      last_reading_(0) {}

GridShieldSystem::~GridShieldSystem() {
    if (crypto_engine_ != nullptr) {
        delete crypto_engine_;
        crypto_engine_ = nullptr;
    }
    
    if (packet_transport_ != nullptr) {
        delete packet_transport_;
        packet_transport_ = nullptr;
    }
}

core::Result<void> GridShieldSystem::initialize(
    const SystemConfig& config,
    platform::PlatformServices& platform) noexcept {
    
    if (initialized_) {
        return MAKE_ERROR(core::ErrorCode::SystemAlreadyInitialized);
    }
    
    if (!platform.is_valid()) {
        return MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    config_ = config;
    platform_ = &platform;
    
    transition_state(core::SystemState::Initializing);
    
    // Initialize hardware layer
    TRY(tamper_detector_.initialize(config_.tamper_config, platform));
    
    // Initialize security layer
    TRY(initialize_crypto());
    
    // Initialize network layer
    if (platform_->comm != nullptr) {
        packet_transport_ = new (std::nothrow) network::PacketTransport(*platform_->comm);
        if (packet_transport_ == nullptr) {
            transition_state(core::SystemState::Error);
            return MAKE_ERROR(core::ErrorCode::ResourceExhausted);
        }
        
        TRY(platform_->comm->init());
    }
    
    // Initialize analytics layer
    TRY(anomaly_detector_.initialize(config_.baseline_profile));
    
    initialized_ = true;
    transition_state(core::SystemState::Ready);
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::start() noexcept {
    if (!initialized_ || state_ != core::SystemState::Ready) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    // Start tamper monitoring
    TRY(tamper_detector_.start());
    
    transition_state(core::SystemState::Operating);
    last_heartbeat_ = platform_->time->get_timestamp_ms();
    last_reading_ = last_heartbeat_;
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::stop() noexcept {
    if (state_ != core::SystemState::Operating) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    TRY(tamper_detector_.stop());
    
    transition_state(core::SystemState::Ready);
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::shutdown() noexcept {
    if (state_ == core::SystemState::Operating) {
        auto result = stop();
        if (result.is_error()) {
            return result.error();
        }
    }
    
    if (platform_ != nullptr && platform_->comm != nullptr) {
        TRY(platform_->comm->shutdown());
    }
    
    device_keypair_.clear();
    server_public_key_.clear();
    
    transition_state(core::SystemState::Shutdown);
    initialized_ = false;
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::process_cycle() noexcept {
    if (state_ != core::SystemState::Operating && 
        state_ != core::SystemState::Tampered) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    core::timestamp_t current_time = platform_->time->get_timestamp_ms();
    
    // Check for tamper events (highest priority)
    if (tamper_detector_.is_tampered() && state_ != core::SystemState::Tampered) {
        auto result = handle_tamper_event();
        if (result.is_error()) {
            return result.error();
        }
    }
    
    // Send heartbeat if interval elapsed
    if (current_time - last_heartbeat_ >= config_.heartbeat_interval_ms) {
        auto result = send_heartbeat();
        // Non-critical error - log but continue
        (void)result;
        last_heartbeat_ = current_time;
    }
    
    // Process periodic reading
    if (current_time - last_reading_ >= config_.reading_interval_ms) {
        // In production: Read actual meter hardware
        core::MeterReading reading;
        reading.timestamp = current_time;
        reading.energy_wh = 1000; // Placeholder
        reading.voltage_mv = 220000; // 220V
        reading.current_ma = 4545; // ~1kW at 220V
        reading.power_factor = 95; // 0.95
        
        auto result = send_meter_reading(reading);
        // Non-critical error
        (void)result;
        last_reading_ = current_time;
    }
    
    // Perform cross-layer validation periodically
    auto validation_result = perform_cross_layer_validation();
    // Log error but continue operation
    (void)validation_result;
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::send_meter_reading(
    const core::MeterReading& reading) noexcept {
    
    if (!initialized_ || crypto_engine_ == nullptr || packet_transport_ == nullptr) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    // Analyze for anomalies first
    auto analysis_result = anomaly_detector_.analyze(reading);
    if (analysis_result.is_ok()) {
        const auto& report = analysis_result.value();
        if (report.severity >= analytics::AnomalySeverity::High) {
            validation_state_.consumption_anomaly_detected = true;
        }
    }
    
    // Update consumption profile
    TRY(anomaly_detector_.update_profile(reading));
    
    // Build packet
    network::SecurePacket packet;
    TRY(packet.build(
        network::PacketType::MeterData,
        config_.meter_id,
        core::Priority::Normal,
        reinterpret_cast<const uint8_t*>(&reading),
        sizeof(core::MeterReading),
        *crypto_engine_,
        device_keypair_
    ));
    
    // Send packet
    return packet_transport_->send_packet(packet, *crypto_engine_, device_keypair_);
}

core::Result<void> GridShieldSystem::send_tamper_alert() noexcept {
    if (!initialized_ || crypto_engine_ == nullptr || packet_transport_ == nullptr) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    core::TamperEvent event;
    event.timestamp = tamper_detector_.get_tamper_timestamp();
    event.event_type = static_cast<uint8_t>(tamper_detector_.get_tamper_type());
    event.severity = static_cast<uint8_t>(core::Priority::Emergency);
    event.sensor_id = config_.tamper_config.sensor_pin;
    
    network::SecurePacket packet;
    TRY(packet.build(
        network::PacketType::TamperAlert,
        config_.meter_id,
        core::Priority::Emergency,
        reinterpret_cast<const uint8_t*>(&event),
        sizeof(core::TamperEvent),
        *crypto_engine_,
        device_keypair_
    ));
    
    return packet_transport_->send_packet(packet, *crypto_engine_, device_keypair_);
}

core::Result<void> GridShieldSystem::send_heartbeat() noexcept {
    if (!initialized_ || crypto_engine_ == nullptr || packet_transport_ == nullptr) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    uint8_t heartbeat_data[8];
    core::timestamp_t timestamp = platform_->time->get_timestamp_ms();
    
    // Serialize timestamp to bytes
    for (size_t i = 0; i < sizeof(timestamp); ++i) {
        heartbeat_data[i] = static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF);
    }
    
    network::SecurePacket packet;
    TRY(packet.build(
        network::PacketType::Heartbeat,
        config_.meter_id,
        core::Priority::Low,
        heartbeat_data,
        sizeof(heartbeat_data),
        *crypto_engine_,
        device_keypair_
    ));
    
    return packet_transport_->send_packet(packet, *crypto_engine_, device_keypair_);
}

core::Result<void> GridShieldSystem::initialize_crypto() noexcept {
    if (platform_->crypto == nullptr) {
        return MAKE_ERROR(core::ErrorCode::InvalidParameter);
    }
    
    crypto_engine_ = new (std::nothrow) security::CryptoEngine(*platform_->crypto);
    if (crypto_engine_ == nullptr) {
        return MAKE_ERROR(core::ErrorCode::ResourceExhausted);
    }
    
    // Generate device keypair
    TRY(crypto_engine_->generate_keypair(device_keypair_));
    
    // In production: Load server public key from secure storage
    // For now: Generate placeholder
    TRY(crypto_engine_->generate_keypair(server_public_key_));
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::handle_tamper_event() noexcept {
    transition_state(core::SystemState::Tampered);
    set_mode(OperationMode::TamperResponse);
    
    validation_state_.physical_tamper_detected = true;
    validation_state_.validation_timestamp = platform_->time->get_timestamp_ms();
    
    // Send immediate tamper alert
    auto result = send_tamper_alert();
    if (result.is_error()) {
        // Even if send fails, record the tamper locally
        return result.error();
    }
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::perform_cross_layer_validation() noexcept {
    validation_state_.validation_timestamp = platform_->time->get_timestamp_ms();
    
    // Check physical layer
    validation_state_.physical_tamper_detected = tamper_detector_.is_tampered();
    
    // Network anomaly detection (placeholder - implement in production)
    validation_state_.network_anomaly_detected = false;
    
    // If multiple layers indicate attack, escalate priority
    if (validation_state_.requires_investigation()) {
        // Log high-priority security event
        // In production: Trigger additional security measures
    }
    
    return core::Result<void>();
}

void GridShieldSystem::transition_state(core::SystemState new_state) noexcept {
    state_ = new_state;
}

void GridShieldSystem::set_mode(OperationMode new_mode) noexcept {
    mode_ = new_mode;
}

} // namespace gridshield