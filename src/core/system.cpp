/**
 * @file system.cpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2026-02-03
 * 
 * @copyright Copyright (c) 2026
 * 
 */

#include "core/system.hpp"
#include <new>

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

core::Result<void> GridShieldSystem::initialize(const SystemConfig& config,
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
    auto result = tamper_detector_.initialize(config_.tamper_config, platform);
    if (result.is_error()) {
        transition_state(core::SystemState::Error);
        return result.error();
    }
    
    // Initialize security layer
    result = initialize_crypto();
    if (result.is_error()) {
        transition_state(core::SystemState::Error);
        return result.error();
    }
    
    // Initialize network layer
    if (platform_->comm != nullptr) {
        packet_transport_ = new (std::nothrow) network::PacketTransport(*platform_->comm);
        if (packet_transport_ == nullptr) {
            transition_state(core::SystemState::Error);
            return MAKE_ERROR(core::ErrorCode::ResourceExhausted);
        }
        
        result = platform_->comm->init();
        if (result.is_error()) {
            transition_state(core::SystemState::Error);
            return result.error();
        }
    }
    
    // Initialize analytics layer
    result = anomaly_detector_.initialize(config_.baseline_profile);
    if (result.is_error()) {
        transition_state(core::SystemState::Error);
        return result.error();
    }
    
    initialized_ = true;
    transition_state(core::SystemState::Ready);
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::start() noexcept {
    if (!initialized_ || state_ != core::SystemState::Ready) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    // Start tamper monitoring
    auto result = tamper_detector_.start();
    if (result.is_error()) {
        return result.error();
    }
    
    transition_state(core::SystemState::Operating);
    last_heartbeat_ = platform_->time->get_timestamp_ms();
    last_reading_ = last_heartbeat_;
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::stop() noexcept {
    if (state_ != core::SystemState::Operating) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    auto result = tamper_detector_.stop();
    if (result.is_error()) {
        return result.error();
    }
    
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
        auto result = platform_->comm->shutdown();
        if (result.is_error()) {
            return result.error();
        }
    }
    
    device_keypair_.clear();
    server_public_key_.clear();
    
    transition_state(core::SystemState::Shutdown);
    initialized_ = false;
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::process_cycle() noexcept {
    if (state_ != core::SystemState::Operating && state_ != core::SystemState::Tampered) {
        return MAKE_ERROR(core::ErrorCode::InvalidState);
    }
    
    core::timestamp_t current_time = platform_->time->get_timestamp_ms();
    
    // Check for tamper events
    if (tamper_detector_.is_tampered() && state_ != core::SystemState::Tampered) {
        auto result = handle_tamper_event();
        if (result.is_error()) {
            return result.error();
        }
    }
    
    // Send heartbeat if interval elapsed
    if (current_time - last_heartbeat_ >= config_.heartbeat_interval_ms) {
        auto result = send_heartbeat();
        if (result.is_error()) {
            // Non-critical error - log but continue
        }
        last_heartbeat_ = current_time;
    }
    
    // Process periodic reading (simulated here)
    if (current_time - last_reading_ >= config_.reading_interval_ms) {
        // In production: Read actual meter data
        core::MeterReading reading;
        reading.timestamp = current_time;
        reading.energy_wh = 1000; // Placeholder
        reading.voltage_mv = 220000; // 220V
        reading.current_ma = 4545; // ~1kW at 220V
        reading.power_factor = 95; // 0.95
        
        auto result = send_meter_reading(reading);
        if (result.is_error()) {
            // Non-critical error
        }
        last_reading_ = current_time;
    }
    
    // Perform cross-layer validation periodically
    auto validation_result = perform_cross_layer_validation();
    if (validation_result.is_error()) {
        // Log error but continue operation
    }
    
    return core::Result<void>();
}

core::Result<void> GridShieldSystem::send_meter_reading(const core::MeterReading& reading) noexcept {
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
    auto update_result = anomaly_detector_.update_profile(reading);
    if (update_result.is_error()) {
        return update_result.error();
    }
    
    // Build packet
    network::SecurePacket packet;
    auto build_result = packet.build(
        network::PacketType::MeterData,
        config_.meter_id,
        core::Priority::Normal,
        reinterpret_cast<const uint8_t*>(&reading),
        sizeof(core::MeterReading),
        *crypto_engine_,
        device_keypair_
    );
    
    if (build_result.is_error()) {
        return build_result.error();
    }
    
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
    auto build_result = packet.build(
        network::PacketType::TamperAlert,
        config_.meter_id,
        core::Priority::Emergency,
        reinterpret_cast<const uint8_t*>(&event),
        sizeof(core::TamperEvent),
        *crypto_engine_,
        device_keypair_
    );
    
    if (build_result.is_error()) {
        return build_result.error();
    }
    
    return packet_transport_->send_packet(packet, *crypto_engine_, device_keypair_);
}

core::Result<void> GridShieldSystem::send_heartbeat() noexcept {
    if (!initialized_ || crypto_engine_ == nullptr || packet_transport_ == nullptr) {
        return MAKE_ERROR(core::ErrorCode::SystemNotInitialized);
    }
    
    uint8_t heartbeat_data[8];
    core::timestamp_t timestamp = platform_->time->get_timestamp_ms();
    for (size_t i = 0; i < sizeof(timestamp); ++i) {
        heartbeat_data[i] = static_cast<uint8_t>((timestamp >> (i * 8)) & 0xFF);
    }
    
    network::SecurePacket packet;
    auto build_result = packet.build(
        network::PacketType::Heartbeat,
        config_.meter_id,
        core::Priority::Low,
        heartbeat_data,
        sizeof(heartbeat_data),
        *crypto_engine_,
        device_keypair_
    );
    
    if (build_result.is_error()) {
        return build_result.error();
    }
    
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
    auto result = crypto_engine_->generate_keypair(device_keypair_);
    if (result.is_error()) {
        return result.error();
    }
    
    // In production: Load server public key from secure storage
    // For now: Generate placeholder
    result = crypto_engine_->generate_keypair(server_public_key_);
    if (result.is_error()) {
        return result.error();
    }
    
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
    
    // Network anomaly detection would go here (not implemented in this version)
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