# GridShield - Multi-Layer AMI Security System

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    GridShield System                        │
│                  (Core Orchestrator)                        │
└─────────────────────────────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
        ▼                  ▼                  ▼
┌──────────────┐  ┌──────────────┐  ┌──────────────┐
│   Physical   │  │   Network    │  │  Analytics   │
│    Layer     │  │    Layer     │  │    Layer     │
│              │  │              │  │              │
│ - Tamper     │  │ - ECC Crypto │  │ - Anomaly    │
│   Detection  │  │ - Secure     │  │   Detection  │
│ - Power Loss │  │   Packets    │  │ - Profile    │
│   Alert      │  │ - Signature  │  │   Learning   │
└──────────────┘  └──────────────┘  └──────────────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                           ▼
                ┌──────────────────┐
                │  Platform Layer  │
                │   (Abstraction)  │
                │                  │
                │ - GPIO/ISR       │
                │ - Time/Crypto    │
                │ - Communication  │
                └──────────────────┘
```

## Module Structure

### Core (`core/`)
- **error.hpp**: Result-based error handling with compile-time safety
- **types.hpp**: Domain-specific types (MeterReading, TamperEvent, etc.)
- **system.hpp**: Main orchestrator coordinating all layers

### Platform (`platform/`)
- **platform.hpp**: Hardware abstraction interfaces
- **mock_platform.hpp**: Mock implementation for testing

### Hardware (`hardware/`)
- **tamper.hpp/cpp**: Physical tamper detection with ISR support

### Security (`security/`)
- **crypto.hpp/cpp**: ECC (secp256r1) cryptography engine
  - Key generation and management
  - ECDSA signing/verification
  - ECDH key agreement
  - AES-256-GCM encryption

### Network (`network/`)
- **packet.hpp/cpp**: Secure packet protocol
  - Integrity verification (checksum)
  - Authentication (ECDSA signature)
  - Priority-based transmission

### Analytics (`analytics/`)
- **detector.hpp/cpp**: Consumption anomaly detection
  - Profile learning
  - Real-time deviation analysis
  - Cross-layer validation

## Key Features

### 1. Defense-in-Depth Architecture
Three independent security layers provide redundancy:
- Physical tampering detection
- Cryptographic authentication
- Statistical anomaly analysis

### 2. Resource-Optimized Design
- Zero-allocation error handling (Result<T>)
- Static buffers for predictable memory usage
- Lightweight cryptography suitable for MCU

### 3. ISR-Safe Implementation
- Volatile state in interrupt handlers
- Lock-free communication patterns
- Debouncing and edge detection

### 4. Platform Portability
- Clean abstraction layer
- Easy to port to ESP32, STM32, or other MCUs
- Mock platform for PC-based testing

### 5. Cross-Layer Validation
System correlates signals from multiple layers:
```
Physical Tamper + Consumption Drop + Network Anomaly
    → Emergency Priority Alert
```

## Build Instructions

### Prerequisites
- CMake ≥ 3.20
- C++23 compiler (GCC ≥ 13, Clang ≥ 17, MSVC 2022+)

### Quick Build
```bash
./build.sh          # Release build
./build.sh Debug    # Debug build
```

### Manual Build
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### Run
```bash
./bin/gridshield
```

## Example Output

```
═══════════════════════════════════════════════════════════════════
  GridShield AMI Security System v1.0.0
  Multi-Layer Protection for Advanced Metering Infrastructure
═══════════════════════════════════════════════════════════════════

[Initialization]
─────────────────────────────────────────
Meter ID: 0x1234567890ABCDEF
Initializing GridShield system...
✓ System initialized successfully
System State: READY

[Phase 1] Normal Operation Mode
─────────────────────────────────────
Cycle 1: ✓ Processing complete
Cycle 2: ✓ Processing complete
...

[Phase 2] Tamper Detection Test
─────────────────────────────────────
Simulating physical tamper event...
✓ Tamper event processed
System State: ⚠️  TAMPERED (CRITICAL)

[Phase 3] Consumption Anomaly Detection
─────────────────────────────────────
Sending normal consumption readings...
  Reading 1: 1000 Wh ✓
  Reading 2: 1010 Wh ✓
Simulating anomalous consumption drop...
  Anomalous reading: 100 Wh ⚠️
  Analytics layer flagged potential manipulation
```

## Production Deployment

### Hardware Integration
Replace mock platform with actual drivers:
```cpp
// Instead of MockGPIO:
class ESP32GPIO : public IPlatformGPIO {
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        // Call ESP32 SDK
        gpio_set_direction((gpio_num_t)pin, mode == PinMode::Output ? 
                          GPIO_MODE_OUTPUT : GPIO_MODE_INPUT);
        return core::Result<void>();
    }
    // ...
};
```

### Cryptography
Integrate real ECC library (e.g., mbedTLS):
```cpp
core::Result<void> CryptoEngine::sign(...) noexcept {
    mbedtls_ecdsa_context ctx;
    // Use actual secp256r1 implementation
    return core::Result<void>();
}
```

### Communication
Implement with LoRa/NB-IoT/WiFi:
```cpp
class LoRaComm : public IPlatformComm {
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        // Use LoRa radio
        return core::Result<size_t>(lora_send(data, length));
    }
};
```

## Security Considerations

1. **Key Storage**: Use secure element (ATECC608) for private keys
2. **Firmware Protection**: Enable read protection on MCU
3. **Secure Boot**: Verify firmware signature on startup
4. **Time Synchronization**: Use authenticated NTP for timestamps
5. **Firmware Updates**: Implement secure OTA with rollback

## Performance Metrics

### Memory Footprint (Estimated)
- Flash: ~64 KB (without crypto library)
- RAM: ~12 KB (static + stack)
- Packet overhead: ~130 bytes/packet

### Timing (ESP32 @ 240MHz)
- Tamper detection: <5ms (ISR-driven)
- Packet encryption: ~50ms
- Anomaly analysis: <10ms
- Full cycle: <100ms

## License

See LICENSE.md

## Authors

- Muhammad Ichwan Fauzi (202331227) - Architecture & Security
- Rafi Indra Pramudhito Zuhayr (202331291) - Firmware & Implementation
- Cesar Ardika Bhayangkara (202311240) - Hardware Integration