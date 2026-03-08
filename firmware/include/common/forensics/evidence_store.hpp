/**
 * @file evidence_store.hpp
 * @author Rafi Indra Pramudhito Zuhayr
 * @brief Evidence preservation for forensic analysis
 * @version 3.2.0
 * @date 2026-03-07
 *
 * Stores immutable evidence snapshots with a tamper-evident hash chain.
 * Each snapshot captures security event context with raw sensor readings
 * and is linked to the previous snapshot via a chained hash,
 * enabling integrity verification of the entire evidence log.
 */

#pragma once

#include "core/error.hpp"
#include "core/types.hpp"
#include "forensics/event_logger.hpp"
#include "utils/gs_macros.hpp"

#include <array>
#include <cstdint>
#include <cstring>


namespace gridshield::forensics {

// ============================================================================
// CONSTANTS
// ============================================================================
static constexpr size_t EVIDENCE_STORE_CAPACITY = 32;
static constexpr size_t EVIDENCE_HASH_SIZE = 32; // 256-bit hash
static constexpr size_t EVIDENCE_NOTES_MAX = 48;

// ============================================================================
// SENSOR SNAPSHOT — raw sensor state at time of evidence capture
// ============================================================================
struct SensorSnapshot
{
    uint32_t energy_wh{0};
    uint32_t voltage_mv{0};
    uint32_t current_ma{0};
    int32_t temperature_c10{0}; // temperature × 10
    uint16_t power_factor{0};
    uint16_t accelerometer_mg{0}; // peak g-force in milli-g

    GS_CONSTEXPR SensorSnapshot() noexcept = default;
};

// ============================================================================
// EVIDENCE SNAPSHOT — immutable forensic record
// ============================================================================
struct EvidenceSnapshot
{
    core::timestamp_t timestamp{0};
    SecurityEventType event_type{SecurityEventType::None};
    SecurityEventSeverity severity{SecurityEventSeverity::Info};
    SourceLayer source_layer{SourceLayer::System};
    uint8_t sequence{0}; // monotonic counter (mod 256)
    SensorSnapshot sensors{};
    char notes[EVIDENCE_NOTES_MAX]{};
    uint8_t hash[EVIDENCE_HASH_SIZE]{};      // hash of this snapshot
    uint8_t prev_hash[EVIDENCE_HASH_SIZE]{}; // hash chain link

    GS_CONSTEXPR bool is_valid() const noexcept
    {
        return event_type != SecurityEventType::None && timestamp > 0;
    }
};

// ============================================================================
// SOFTWARE SHA-256 — lightweight (header-only, no heap, no mbedTLS dep)
// ============================================================================
namespace detail {

/**
 * Simple deterministic hash for evidence chain.
 * Uses a Fowler–Noll–Vo-1a variant (FNV-1a 256-bit) for lightweight
 * tamper detection. This is NOT a cryptographic hash — it provides
 * change detection, not security against a motivated attacker.
 */
inline void evidence_hash(const uint8_t* data, size_t len, uint8_t out[EVIDENCE_HASH_SIZE]) noexcept
{
    // FNV-1a offset basis spread across 32 bytes
    uint8_t state[EVIDENCE_HASH_SIZE] = {
        0xcb, 0xf2, 0x9c, 0xe4, 0x84, 0x22, 0x23, 0x25, 0x14, 0x07, 0x3d,
        0xb5, 0xbf, 0x5b, 0x1e, 0x73, 0x3b, 0x0e, 0x77, 0x0a, 0xda, 0xe6,
        0x37, 0x39, 0x42, 0x11, 0xa5, 0xb3, 0x59, 0x8c, 0x2f, 0x45,
    };

    for (size_t i = 0; i < len; ++i) {
        // XOR-fold the byte across the state
        const uint8_t b = data[i];
        for (size_t j = 0; j < EVIDENCE_HASH_SIZE; ++j) {
            state[j] ^= b;
            // FNV multiply: state[j] = state[j] * 0x01000193
            // Simplified: shift-add for embedded
            uint16_t v = static_cast<uint16_t>(state[j]);
            v = static_cast<uint16_t>(v + (v << 1) + (v << 4) + (v << 7));
            state[j] = static_cast<uint8_t>(v & 0xFF);
            // Cascade carry
            state[(j + 1) % EVIDENCE_HASH_SIZE] ^= static_cast<uint8_t>(v >> 8);
        }
    }

    std::memcpy(out, state, EVIDENCE_HASH_SIZE);
}

} // namespace detail

// ============================================================================
// EVIDENCE STORE — Circular buffer with hash chain
// ============================================================================
class EvidenceStore
{
public:
    EvidenceStore() noexcept = default;

    /**
     * @brief Preserve a new evidence snapshot.
     * Links to previous snapshot via hash chain. Circular — oldest
     * evidence is overwritten when capacity is reached.
     */
    core::Result<void> preserve(SecurityEventType type,
                                SecurityEventSeverity severity,
                                SourceLayer layer,
                                core::timestamp_t timestamp,
                                const SensorSnapshot& sensors,
                                const char* notes = nullptr) noexcept
    {
        auto& slot = snapshots_[write_index_];
        slot.timestamp = timestamp;
        slot.event_type = type;
        slot.severity = severity;
        slot.source_layer = layer;
        slot.sequence = sequence_++;
        slot.sensors = sensors;

        if (notes != nullptr) {
            std::strncpy(slot.notes, notes, EVIDENCE_NOTES_MAX - 1);
            slot.notes[EVIDENCE_NOTES_MAX - 1] = '\0';
        } else {
            slot.notes[0] = '\0';
        }

        // Copy previous hash into chain link
        if (count_ > 0) {
            size_t prev_idx = (write_index_ == 0) ? EVIDENCE_STORE_CAPACITY - 1 : write_index_ - 1;
            std::memcpy(slot.prev_hash, snapshots_[prev_idx].hash, EVIDENCE_HASH_SIZE);
        } else {
            std::memset(slot.prev_hash, 0, EVIDENCE_HASH_SIZE);
        }

        // Compute hash of this snapshot (over all fields except hash itself)
        compute_hash(slot);

        write_index_ = (write_index_ + 1) % EVIDENCE_STORE_CAPACITY;
        if (count_ < EVIDENCE_STORE_CAPACITY) {
            ++count_;
        }

        return core::Result<void>::ok();
    }

    /**
     * @brief Get the number of stored evidence snapshots.
     */
    GS_NODISCARD size_t evidence_count() const noexcept
    {
        return count_;
    }

    /**
     * @brief Get an evidence snapshot by index (0 = oldest).
     */
    GS_NODISCARD core::Result<EvidenceSnapshot> get_evidence(size_t index) const noexcept
    {
        if (index >= count_) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }

        size_t actual = 0;
        if (count_ < EVIDENCE_STORE_CAPACITY) {
            actual = index;
        } else {
            actual = (write_index_ + index) % EVIDENCE_STORE_CAPACITY;
        }
        return core::Result<EvidenceSnapshot>(snapshots_[actual]);
    }

    /**
     * @brief Get the latest evidence snapshot.
     */
    GS_NODISCARD core::Result<EvidenceSnapshot> latest() const noexcept
    {
        if (count_ == 0) {
            return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
        }
        return get_evidence(count_ - 1);
    }

    /**
     * @brief Verify the hash chain integrity of all stored evidence.
     * @return true if chain is intact, false if tampered.
     */
    GS_NODISCARD bool verify_chain() const noexcept
    {
        if (count_ == 0) {
            return true;
        }

        for (size_t i = 0; i < count_; ++i) {
            auto result = get_evidence(i);
            if (!result.is_ok()) {
                return false;
            }
            const auto& snap = result.value();

            // Verify this snapshot's own hash
            EvidenceSnapshot temp = snap;
            std::memset(temp.hash, 0, EVIDENCE_HASH_SIZE);
            uint8_t recomputed[EVIDENCE_HASH_SIZE]{};
            compute_hash_raw(temp, recomputed);
            if (std::memcmp(recomputed, snap.hash, EVIDENCE_HASH_SIZE) != 0) {
                return false;
            }

            // Verify chain link (skip first entry which has all-zero prev_hash)
            if (i > 0) {
                auto prev_result = get_evidence(i - 1);
                if (!prev_result.is_ok()) {
                    return false;
                }
                if (std::memcmp(snap.prev_hash, prev_result.value().hash, EVIDENCE_HASH_SIZE) !=
                    0) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * @brief Clear all evidence.
     */
    void clear() noexcept
    {
        count_ = 0;
        write_index_ = 0;
        sequence_ = 0;
        for (auto& snap : snapshots_) {
            snap = EvidenceSnapshot{};
        }
    }

private:
    void compute_hash(EvidenceSnapshot& snap) const noexcept
    {
        std::memset(snap.hash, 0, EVIDENCE_HASH_SIZE);
        compute_hash_raw(snap, snap.hash);
    }

    static void compute_hash_raw(const EvidenceSnapshot& snap,
                                 uint8_t out[EVIDENCE_HASH_SIZE]) noexcept
    {
        // Hash over the entire snapshot struct (hash field is zeroed)
        const auto* raw = reinterpret_cast<const uint8_t*>(&snap);
        detail::evidence_hash(raw, sizeof(EvidenceSnapshot), out);
    }

    EvidenceSnapshot snapshots_[EVIDENCE_STORE_CAPACITY]{};
    size_t write_index_{0};
    size_t count_{0};
    uint8_t sequence_{0};
};

} // namespace gridshield::forensics
