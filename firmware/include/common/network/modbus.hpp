/**
 * @file modbus.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief Modbus RTU/TCP client abstraction for industrial sensor communication
 * @version 1.0
 * @date 2026-02-25
 *
 * Provides a platform-agnostic Modbus client interface supporting
 * RTU (serial/UART) and TCP modes for reading energy meter registers.
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
// MODBUS CONSTANTS
// ============================================================================
static constexpr uint32_t MODBUS_DEFAULT_BAUD_RATE = 9600;
static constexpr uint8_t MODBUS_DEFAULT_SLAVE_ADDR = 1;
static constexpr uint32_t MODBUS_DEFAULT_TIMEOUT_MS = 1000;
static constexpr size_t MODBUS_MAX_REGISTERS_PER_READ = 125;
static constexpr size_t MODBUS_RTU_FRAME_MAX_SIZE = 256;

// ============================================================================
// MODBUS MODE
// ============================================================================
enum class ModbusMode : uint8_t
{
    RTU = 0, // Serial (UART) — for PZEM-004T, energy meters
    TCP = 1  // Network (WiFi/Ethernet)
};

// ============================================================================
// MODBUS FUNCTION CODES
// ============================================================================
enum class ModbusFunction : uint8_t
{
    ReadHoldingRegisters = 0x03,
    ReadInputRegisters = 0x04,
    WriteSingleRegister = 0x06,
    WriteMultipleRegisters = 0x10
};

// ============================================================================
// MODBUS RTU CONFIGURATION
// ============================================================================
struct ModbusRtuConfig
{
    uint8_t uart_port{};
    uint32_t baud_rate{MODBUS_DEFAULT_BAUD_RATE};
    uint8_t slave_addr{MODBUS_DEFAULT_SLAVE_ADDR};
    uint8_t tx_pin{};
    uint8_t rx_pin{};
    uint32_t timeout_ms{MODBUS_DEFAULT_TIMEOUT_MS};

    GS_CONSTEXPR ModbusRtuConfig() noexcept = default;
};

static constexpr uint16_t MODBUS_TCP_DEFAULT_PORT = 502;

// ============================================================================
// MODBUS TCP CONFIGURATION
// ============================================================================
struct ModbusTcpConfig
{
    const char* host{};
    uint16_t port{MODBUS_TCP_DEFAULT_PORT};
    uint8_t slave_addr{MODBUS_DEFAULT_SLAVE_ADDR};
    uint32_t timeout_ms{MODBUS_DEFAULT_TIMEOUT_MS};

    GS_CONSTEXPR ModbusTcpConfig() noexcept = default;
};

// ============================================================================
// MODBUS CLIENT INTERFACE
// ============================================================================
class IModbusClient
{
public:
    virtual ~IModbusClient() noexcept = default;

    /**
     * @brief Initialize RTU mode (serial).
     */
    virtual core::Result<void> init_rtu(const ModbusRtuConfig& config) noexcept = 0;

    /**
     * @brief Read a single holding register (function code 0x03).
     * @param reg_addr Register address
     * @return Register value (16-bit)
     */
    virtual core::Result<uint16_t> read_holding_register(uint16_t reg_addr) noexcept = 0;

    /**
     * @brief Read multiple holding registers.
     * @param reg_addr Starting register address
     * @param count Number of registers to read
     * @param out_buf Output buffer (must hold at least count elements)
     */
    virtual core::Result<void>
    read_holding_registers(uint16_t reg_addr, uint16_t count, uint16_t* out_buf) noexcept = 0;

    /**
     * @brief Read a single input register (function code 0x04).
     */
    virtual core::Result<uint16_t> read_input_register(uint16_t reg_addr) noexcept = 0;

    /**
     * @brief Write a single register (function code 0x06).
     */
    virtual core::Result<void> write_register(uint16_t reg_addr, uint16_t value) noexcept = 0;

    /**
     * @brief Shut down the Modbus client.
     */
    virtual void shutdown() noexcept = 0;
};

} // namespace gridshield::network
