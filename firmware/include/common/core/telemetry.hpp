/**
 * @file telemetry.hpp
 * @author zuudevs (zuudevs@gmail.com)
 * @brief System telemetry and diagnostics counters
 * @version 1.0
 * @date 2026-02-23
 *
 * Tracks runtime statistics: packets sent/received, errors,
 * uptime, tamper events, crypto operations, etc.
 * All counters are lock-free (single-writer assumed on ESP32 single-core use).
 *
 * @copyright Copyright (c) 2026
 */

#pragma once

#include "core/types.hpp"

namespace gridshield::core {

// ============================================================================
// TELEMETRY COUNTERS
// ============================================================================
struct TelemetryCounters
{
    // Lifecycle
    uint32_t boot_count{};
    timestamp_t uptime_ms{};
    timestamp_t last_boot_time{};

    // Network
    uint32_t packets_sent{};
    uint32_t packets_received{};
    uint32_t packets_failed{};
    uint32_t bytes_sent{};
    uint32_t bytes_received{};
    uint32_t network_retries{};

    // Security
    uint32_t crypto_operations{};
    uint32_t key_rotations{};
    uint32_t tamper_events{};
    uint32_t tamper_acknowledged{};

    // Analytics
    uint32_t readings_processed{};
    uint32_t anomalies_detected{};

    // Errors
    uint32_t total_errors{};
    uint32_t critical_errors{};

    // Process cycles
    uint32_t cycle_count{};
    uint32_t cycle_overruns{}; // cycle took longer than reading_interval

    constexpr TelemetryCounters() noexcept = default;
};

// ============================================================================
// SYSTEM TELEMETRY
// ============================================================================
class SystemTelemetry
{
public:
    SystemTelemetry() noexcept = default;

    // --- Increment helpers ---

    void record_packet_sent(uint32_t bytes = 0) noexcept
    {
        ++counters_.packets_sent;
        counters_.bytes_sent += bytes;
    }

    void record_packet_received(uint32_t bytes = 0) noexcept
    {
        ++counters_.packets_received;
        counters_.bytes_received += bytes;
    }

    void record_packet_failed() noexcept
    {
        ++counters_.packets_failed;
    }

    void record_network_retry() noexcept
    {
        ++counters_.network_retries;
    }

    void record_crypto_op() noexcept
    {
        ++counters_.crypto_operations;
    }

    void record_key_rotation() noexcept
    {
        ++counters_.key_rotations;
    }

    void record_tamper_event() noexcept
    {
        ++counters_.tamper_events;
    }

    void record_tamper_acknowledged() noexcept
    {
        ++counters_.tamper_acknowledged;
    }

    void record_reading() noexcept
    {
        ++counters_.readings_processed;
    }

    void record_anomaly() noexcept
    {
        ++counters_.anomalies_detected;
    }

    void record_error(bool critical = false) noexcept
    {
        ++counters_.total_errors;
        if (critical) {
            ++counters_.critical_errors;
        }
    }

    void record_cycle(bool overrun = false) noexcept
    {
        ++counters_.cycle_count;
        if (overrun) {
            ++counters_.cycle_overruns;
        }
    }

    void record_boot(timestamp_t boot_time = 0) noexcept
    {
        ++counters_.boot_count;
        counters_.last_boot_time = boot_time;
    }

    void update_uptime(timestamp_t current_ms) noexcept
    {
        if (counters_.last_boot_time > 0) {
            counters_.uptime_ms = current_ms - counters_.last_boot_time;
        }
    }

    // --- Accessors ---

    GS_NODISCARD const TelemetryCounters& counters() const noexcept
    {
        return counters_;
    }

    void reset() noexcept
    {
        counters_ = TelemetryCounters{};
    }

private:
    TelemetryCounters counters_;
};

} // namespace gridshield::core
