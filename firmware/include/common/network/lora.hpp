/**
 * @file lora.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief LoRa/LoRaWAN driver interface for long-range communication
 * @version 1.0
 * @date 2026-02-25
 *
 * Interface for SX1276/SX1278 LoRa transceivers (SPI-based).
 * Implementation requires external hardware module — this file
 * provides the HAL interface and mock for testing.
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>

namespace gridshield::network {

// ============================================================================
// LORA CONSTANTS
// ============================================================================
static constexpr size_t LORA_MAX_PAYLOAD_SIZE = 255;
static constexpr uint32_t LORA_DEFAULT_FREQUENCY_HZ = 915000000; // 915 MHz (US band)
static constexpr uint8_t LORA_DEFAULT_SPREADING_FACTOR = 7;
static constexpr uint32_t LORA_DEFAULT_BANDWIDTH_HZ = 125000;
static constexpr uint8_t LORA_DEFAULT_CODING_RATE = 5; // 4/5
static constexpr int8_t LORA_DEFAULT_TX_POWER_DBM = 14;

// ============================================================================
// LORA CONFIGURATION
// ============================================================================
struct LoRaConfig
{
    uint32_t frequency_hz{LORA_DEFAULT_FREQUENCY_HZ};
    uint8_t spreading_factor{LORA_DEFAULT_SPREADING_FACTOR}; // SF7-SF12
    uint32_t bandwidth_hz{LORA_DEFAULT_BANDWIDTH_HZ};
    uint8_t coding_rate{LORA_DEFAULT_CODING_RATE}; // 4/5 to 4/8
    int8_t tx_power_dbm{LORA_DEFAULT_TX_POWER_DBM};

    // SPI pins
    uint8_t spi_cs_pin{};
    uint8_t spi_mosi_pin{};
    uint8_t spi_miso_pin{};
    uint8_t spi_sck_pin{};
    uint8_t reset_pin{};
    uint8_t dio0_pin{}; // Interrupt pin

    GS_CONSTEXPR LoRaConfig() noexcept = default;
};

// ============================================================================
// LORA DRIVER INTERFACE
// ============================================================================
class ILoRaDriver
{
public:
    virtual ~ILoRaDriver() noexcept = default;

    /**
     * @brief Initialize the LoRa transceiver.
     */
    virtual core::Result<void> init(const LoRaConfig& config) noexcept = 0;

    /**
     * @brief Send a packet over LoRa.
     * @param data Payload bytes
     * @param length Payload length (max LORA_MAX_PAYLOAD_SIZE)
     */
    virtual core::Result<void> send(const uint8_t* data, size_t length) noexcept = 0;

    /**
     * @brief Receive a packet (blocking with timeout).
     * @param buffer Output buffer
     * @param max_length Buffer capacity
     * @param timeout_ms Timeout in milliseconds
     * @return Number of bytes received
     */
    virtual core::Result<size_t>
    receive(uint8_t* buffer, size_t max_length, uint32_t timeout_ms) noexcept = 0;

    /**
     * @brief Get the RSSI of the last received packet.
     */
    GS_NODISCARD virtual core::Result<int16_t> get_rssi() noexcept = 0;

    /**
     * @brief Get the SNR of the last received packet.
     */
    GS_NODISCARD virtual core::Result<int8_t> get_snr() noexcept = 0;

    /**
     * @brief Put transceiver into sleep mode.
     */
    virtual core::Result<void> sleep() noexcept = 0;

    /**
     * @brief Shut down the transceiver.
     */
    virtual void shutdown() noexcept = 0;
};

} // namespace gridshield::network
