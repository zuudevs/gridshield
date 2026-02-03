# GridShield - Production-Grade Firmware Architecture

## Directory Structure

```
gridshield/
├── CMakeLists.txt                      # Build configuration
├── build.sh                            # Quick build script
├── README.md                           # Project documentation
│
├── include/                            # Header files (interface definitions)
│   ├── core/
│   │   ├── error.hpp                   # Result<T> error handling, ErrorCode enum
│   │   ├── types.hpp                   # Core types: MeterReading, TamperEvent, etc.
│   │   └── system.hpp                  # Main system orchestrator interface
│   │
│   ├── platform/
│   │   ├── platform.hpp                # Platform abstraction layer (HAL)
│   │   └── mock_platform.hpp           # Mock implementation for PC testing
│   │
│   ├── hardware/
│   │   └── tamper.hpp                  # Physical tamper detection (Layer 1)
│   │
│   ├── security/
│   │   └── crypto.hpp                  # ECC crypto engine (Layer 2)
│   │
│   ├── network/
│   │   └── packet.hpp                  # Secure packet protocol (Layer 2)
│   │
│   └── analytics/
│       └── detector.hpp                # Anomaly detection (Layer 3)
│
└── src/                                # Implementation files
    ├── main.cpp                        # Entry point + demonstration
    │
    ├── core/
    │   └── system.cpp                  # System orchestrator implementation
    │
    ├── hardware/
    │   └── tamper.cpp                  # Tamper detector implementation
    │
    ├── security/
    │   └── crypto.cpp                  # Crypto engine implementation
    │
    ├── network/
    │   └── packet.cpp                  # Packet protocol implementation
    │
    └── analytics/
        └── detector.cpp                # Anomaly detector implementation
```

## File Roles & Responsibilities

### Core Module

**include/core/error.hpp**
- `ErrorCode` enum: Categorized error codes (100-999)
- `ErrorContext`: Error metadata with source location
- `Result<T>`: Type-safe error handling without exceptions
- `MAKE_ERROR` macro: Automatic error context capture
- `TRY` macro: Early return on error

**include/core/types.hpp**
- `MeterReading`: Energy consumption data structure (24 bytes)
- `TamperEvent`: Physical security event (16 bytes)
- `StaticBuffer<T, N>`: Fixed-size container for embedded
- `ByteArray<N>`: Byte buffer with append operations
- Type aliases: `timestamp_t`, `meter_id_t`, `sequence_t`

**include/core/system.hpp + src/core/system.cpp**
- `GridShieldSystem`: Main orchestrator class
- Lifecycle management: initialize → start → process_cycle → stop → shutdown
- Layer coordination: Physical + Network + Analytics
- Cross-layer validation logic
- Automatic tamper response handling

### Platform Module

**include/platform/platform.hpp**
- `IPlatformTime`: Timestamp and delay abstraction
- `IPlatformGPIO`: Digital I/O abstraction
- `IPlatformInterrupt`: ISR attachment interface
- `IPlatformCrypto`: Hardware crypto (RNG, hash) abstraction
- `IPlatformStorage`: Non-volatile memory abstraction
- `IPlatformComm`: Network communication abstraction
- `PlatformServices`: Aggregates all platform services

**include/platform/mock_platform.hpp**
- `MockTime`, `MockGPIO`, `MockInterrupt`: Testing implementations
- `MockCrypto`: Software fallback for cryptographic operations
- `MockComm`: In-memory packet transmission simulator
- Allows full system testing without hardware

### Hardware Module (Physical Security Layer)

**include/hardware/tamper.hpp + src/hardware/tamper.cpp**
- `ITamperDetector`: Abstract tamper detection interface
- `TamperDetector`: Concrete implementation
- ISR-safe tamper event handling
- Debouncing logic (configurable)
- Power-loss detection support
- Priority flag mechanism for emergency transmission

Key Features:
- Interrupt-driven (non-blocking)
- Volatile state for ISR safety
- Configurable sensitivity and debounce timing

### Security Module (Network Security Layer)

**include/security/crypto.hpp + src/security/crypto.cpp**
- `ECCKeyPair`: 256-bit ECC key management
- `ICryptoEngine`: Abstract crypto operations
- `CryptoEngine`: Implementation (placeholder for production ECC library)

Operations:
- `generate_keypair()`: Create device identity
- `sign()`: ECDSA signature generation
- `verify()`: ECDSA signature verification
- `derive_shared_secret()`: ECDH key agreement
- `encrypt_aes_gcm()` / `decrypt_aes_gcm()`: Authenticated encryption
- `hash_sha256()`: Message digest
- `random_bytes()`: CSPRNG

### Network Module (Communication Security Layer)

**include/network/packet.hpp + src/network/packet.cpp**
- `SecurePacket`: Authenticated and integrity-protected packet
- `PacketHeader`: Metadata (type, priority, sequence, meter_id)
- `PacketFooter`: ECC signature + magic byte
- `IPacketTransport`: Abstract send/receive interface
- `PacketTransport`: Concrete implementation over platform comm

Packet Format (wire protocol):
```
[HEADER] [PAYLOAD] [SIGNATURE] [FOOTER]
   ^         ^          ^         ^
  24B     0-512B       64B        1B
```

Security Features:
- SHA256 checksum for integrity
- ECDSA signature for authentication
- Sequence numbers for replay protection
- Magic bytes for frame validation

### Analytics Module (Application Security Layer)

**include/analytics/detector.hpp + src/analytics/detector.cpp**
- `IAnomalyDetector`: Abstract anomaly detection interface
- `AnomalyDetector`: Statistical deviation analyzer
- `ConsumptionProfile`: Historical baseline (24-hour profile)
- `AnomalyReport`: Detected anomaly details
- `CrossLayerValidation`: Correlation between layers

Detection Logic:
- Profile learning from historical data
- Real-time deviation calculation
- Severity classification (Low → Critical)
- Threshold-based alerting (configurable)

### Entry Point

**src/main.cpp**
- System initialization demonstration
- Normal operation simulation
- Tamper event demonstration
- Anomaly detection demonstration
- Secure communication verification
- Graceful shutdown sequence

## Build & Execution

### Compilation
```bash
./build.sh          # Release build
./build.sh Debug    # Debug build with symbols
```

Or manually:
```bash
g++ -std=c++23 -O2 -Wall -Wextra -I./include \
    -o bin/gridshield \
    src/main.cpp \
    src/core/system.cpp \
    src/hardware/tamper.cpp \
    src/security/crypto.cpp \
    src/network/packet.cpp \
    src/analytics/detector.cpp
```

### Execution
```bash
./bin/gridshield
```

## Production Deployment Strategy

### Step 1: Hardware Platform Selection
Replace mock platform with actual MCU HAL:
- ESP32: Use ESP-IDF platform APIs
- STM32: Use HAL/LL drivers
- nRF52: Use nRF SDK

Example (ESP32):
```cpp
class ESP32GPIO : public IPlatformGPIO {
    core::Result<void> configure(uint8_t pin, PinMode mode) noexcept override {
        gpio_config_t config = {
            .pin_bit_mask = (1ULL << pin),
            .mode = (mode == PinMode::Output) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
            // ...
        };
        gpio_config(&config);
        return core::Result<void>();
    }
};
```

### Step 2: Cryptography Library Integration
Replace placeholder crypto with production library (mbedTLS, WolfSSL, TinyCrypt):

```cpp
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>

core::Result<void> CryptoEngine::sign(...) {
    mbedtls_ecdsa_context ctx;
    mbedtls_ecdsa_init(&ctx);
    // Use secp256r1 curve
    // ...
}
```

### Step 3: Secure Key Storage
Use hardware security module:
- ATECC608 for secure element
- ESP32's eFuse for key storage
- STM32's Secure Storage

### Step 4: Communication Module
Implement actual transport (LoRa, NB-IoT, WiFi):

```cpp
class LoRaComm : public IPlatformComm {
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        // SPI communication with LoRa radio
        return lora_transmit(data, length);
    }
};
```

## Memory Footprint Analysis

### Static Memory (BSS/Data)
- System state: ~200 bytes
- Crypto buffers: ~256 bytes
- Packet buffers: ~650 bytes
- Analytics profile: ~200 bytes
**Total**: ~1.3 KB

### Stack Usage (Estimated)
- Main task: ~2 KB
- Interrupt handlers: ~512 bytes
**Total**: ~2.5 KB

### Code Size (Flash)
- Core logic: ~20 KB
- Platform abstraction: ~5 KB
- Crypto (without library): ~10 KB
- With mbedTLS: +50 KB
**Total**: ~35-85 KB depending on crypto library

## Security Hardening Checklist

✓ Zero dynamic allocation (predictable memory)
✓ Bounds checking on all buffers
✓ Volatile state in ISR handlers
✓ Result-based error handling (no exceptions)
✓ Type-safe interfaces (no void*)
✓ Const correctness throughout
✓ No implicit conversions (explicit casts)
✓ Structured logging of security events

TODO for Production:
- [ ] Enable stack canaries (-fstack-protector-strong)
- [ ] Enable position-independent code (PIE)
- [ ] Implement secure erase for key material
- [ ] Add watchdog timer support
- [ ] Implement secure boot chain
- [ ] Add OTA update with signature verification
- [ ] Implement rate limiting for anti-DoS
- [ ] Add encrypted logging to flash

## Testing Strategy

### Unit Testing
Test each module independently:
```cpp
// Test tamper detector
MockGPIO gpio;
MockInterrupt interrupt;
TamperDetector detector;
// Trigger simulation...
```

### Integration Testing
Use mock platform to test full system:
```cpp
GridShieldSystem system;
MockPlatformServices platform;
system.initialize(config, platform);
// Simulate scenarios...
```

### Hardware-in-Loop Testing
Deploy to actual hardware with test harness:
- Physical tamper simulation
- RF injection for network testing
- Current source for anomaly testing

## Performance Benchmarks

Target MCU: ESP32 @ 240MHz

- Tamper detection latency: <5ms (ISR-driven)
- Packet encryption: ~50ms (with mbedTLS)
- Anomaly analysis: <10ms
- Full process cycle: <100ms
- Power consumption: <50mA active, <1mA sleep

## Extensibility Points

### Adding New Sensors
1. Extend `TamperType` enum
2. Implement new detector class
3. Register with system orchestrator

### Adding New Packet Types
1. Extend `PacketType` enum
2. Define payload structure
3. Implement handler in system

### Adding New Anomaly Detectors
1. Implement `IAnomalyDetector` interface
2. Register with analytics module
3. Configure thresholds

## License & Attribution

MIT License - See LICENSE.md

Developed by:
- Muhammad Ichwan Fauzi (System Architecture)
- Rafi Indra Pramudhito Zuhayr (Firmware Implementation)
- Cesar Ardika Bhayangkara (Hardware Integration)

Institut Teknologi PLN - 2025