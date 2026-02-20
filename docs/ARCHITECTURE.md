# GridShield - System Architecture

**Version:** 1.0.0  
**Last Updated:** February 2026  
**Authors:** M. Ichwan Fauzi, Rafi Indra Pramudhito Zuhayr, Cesar Ardika Bhayangkara

---

## Table of Contents

- [Overview](#overview)
- [High-Level Architecture](#high-level-architecture)
- [System Diagrams](#system-diagrams)
- [Layer Architecture](#layer-architecture)
- [Component Details](#component-details)
- [Data Flow](#data-flow)
- [Security Model](#security-model)
- [File Organization](#file-organization)
- [Memory Architecture](#memory-architecture)
- [Production Deployment](#production-deployment)

---

## Overview

GridShield is a production-grade, multi-layer security system designed for Advanced Metering Infrastructure (AMI). It implements a **Defense-in-Depth** strategy with three distinct security layers:

1. **Physical Security Layer** - Hardware tamper detection
2. **Network Security Layer** - Cryptographic authentication and integrity
3. **Application Security Layer** - Behavioral anomaly detection

### Key Characteristics

- **Language:** C++17 (embedded-friendly, no exceptions)
- **Memory Model:** Zero heap allocation, predictable stack usage
- **Platform Support:** Native (PC testing) + Arduino AVR (production)
- **Security:** ECC-based authentication, AES-GCM encryption, SHA-256 integrity
- **Architecture:** Layered design with Hardware Abstraction Layer (HAL)
- **Dependencies:** `rweather/Crypto` & `micro-ecc` (embedded on all platforms)

---

## High-Level Architecture

### System Overview Diagram

```mermaid
graph TB
    subgraph "Physical Layer"
        A[Tamper Sensor] --> B[TamperDetector]
        C[Power Monitor] --> B
        B --> D[ISR Handler]
    end
    
    subgraph "Network Layer"
        E[CryptoEngine] --> F[PacketTransport]
        G[ECC Keys] --> E
        F --> H[Platform Comm]
    end
    
    subgraph "Application Layer"
        I[MeterReading] --> J[AnomalyDetector]
        J --> K[ConsumptionProfile]
        K --> L[Alert Engine]
    end
    
    subgraph "Orchestration"
        M[GridShieldSystem]
    end
    
    B --> M
    F --> M
    J --> M
    M --> N[Cross-Layer Validator]
    
    subgraph "Platform Abstraction"
        O[HAL: Time/GPIO/Interrupt/Crypto/Comm]
    end
    
    M --> O
    H --> O
    
    style M fill:#e1f5fe
    style N fill:#fff3e0
    style O fill:#f3e5f5
```

### Three-Layer Security Model

```mermaid
flowchart LR
    subgraph L1["Layer 1: Physical Security"]
        direction TB
        T1[Tamper Switch] --> T2[Debounce Logic]
        T2 --> T3[Priority Flag]
        T3 --> T4[Emergency Transmission]
    end
    
    subgraph L2["Layer 2: Network Security"]
        direction TB
        N1[Meter Data] --> N2[ECC Signature]
        N2 --> N3[AES-GCM Encryption]
        N3 --> N4[Packet Formation]
        N4 --> N5[Authenticated Send]
    end
    
    subgraph L3["Layer 3: Analytics"]
        direction TB
        A1[Consumption Data] --> A2[Profile Comparison]
        A2 --> A3[Deviation Analysis]
        A3 --> A4[Anomaly Classification]
        A4 --> A5[Severity Rating]
    end
    
    L1 --> V[Cross-Layer Validator]
    L2 --> V
    L3 --> V
    V --> Alert[Security Alert]
    
    style L1 fill:#ffebee
    style L2 fill:#e8f5e9
    style L3 fill:#e3f2fd
    style V fill:#fff9c4
```

---

## System Diagrams

### Complete Data Flow

```mermaid
sequenceDiagram
    participant Sensor as Physical Sensors
    participant Tamper as TamperDetector
    participant System as GridShieldSystem
    participant Crypto as CryptoEngine
    participant Network as PacketTransport
    participant Analytics as AnomalyDetector
    participant Server as Head-End Server
    
    Note over Sensor,Server: Normal Operation Flow
    
    Sensor->>System: MeterReading (1000 Wh)
    System->>Analytics: analyze(reading)
    Analytics-->>System: AnomalyReport (OK)
    System->>Analytics: update_profile(reading)
    
    System->>Crypto: sign(reading_data, device_key)
    Crypto-->>System: signature[64 bytes]
    
    System->>Network: build_packet(reading, signature)
    Network-->>System: SecurePacket
    
    System->>Network: send_packet()
    Network->>Server: [Encrypted Packet]
    
    Note over Sensor,Server: Tamper Event Flow
    
    Sensor->>Tamper: PIN_CHANGE (casing opened)
    Tamper->>Tamper: ISR Handler (debounce)
    Tamper->>System: is_tampered() = true
    
    System->>System: transition_state(TAMPERED)
    System->>System: set_mode(TamperResponse)
    
    System->>Crypto: sign(tamper_event, device_key)
    System->>Network: send_packet(PRIORITY=Emergency)
    Network->>Server: [TAMPER ALERT]
    
    Server-->>Network: Acknowledgment
    
    Note over Sensor,Server: Anomaly Detection Flow
    
    Sensor->>System: MeterReading (100 Wh) - suspicious drop
    System->>Analytics: analyze(reading)
    Analytics->>Analytics: calculate_deviation()
    Analytics-->>System: AnomalyReport (CRITICAL, 90% drop)
    
    System->>System: Cross-Layer Validation
    System->>System: Check physical_tamper_detected
    System->>System: Check consumption_anomaly_detected
    
    alt Both Layers Compromised
        System->>Network: send_packet(PRIORITY=Emergency)
        Network->>Server: [MULTI-LAYER ATTACK DETECTED]
    end
```

### Component Interaction

```mermaid
classDiagram
    class GridShieldSystem {
        -SystemConfig config_
        -PlatformServices* platform_
        -TamperDetector tamper_detector_
        -CryptoEngine* crypto_engine_
        -PacketTransport* packet_transport_
        -AnomalyDetector anomaly_detector_
        +initialize(config, platform) Result~void~
        +start() Result~void~
        +process_cycle() Result~void~
        +send_meter_reading(reading) Result~void~
        +send_tamper_alert() Result~void~
    }
    
    class TamperDetector {
        -volatile bool is_tampered_
        -TamperConfig config_
        +initialize(config, platform) Result~void~
        +is_tampered() bool
        +get_tamper_type() TamperType
        -interrupt_handler(context) void
    }
    
    class CryptoEngine {
        -IPlatformCrypto& platform_crypto_
        +generate_keypair(keypair) Result~void~
        +sign(keypair, message, signature) Result~void~
        +verify(keypair, message, signature) Result~bool~
        +hash_sha256(data, hash) Result~void~
    }
    
    class PacketTransport {
        -IPlatformComm& comm_
        +send_packet(packet, crypto, keypair) Result~void~
        +receive_packet(crypto, keypair, timeout) Result~SecurePacket~
    }
    
    class AnomalyDetector {
        -ConsumptionProfile profile_
        -StaticBuffer recent_readings_
        +initialize(baseline_profile) Result~void~
        +analyze(reading) Result~AnomalyReport~
        +update_profile(reading) Result~void~
    }
    
    class PlatformServices {
        +IPlatformTime* time
        +IPlatformGPIO* gpio
        +IPlatformInterrupt* interrupt
        +IPlatformCrypto* crypto
        +IPlatformComm* comm
    }
    
    GridShieldSystem --> TamperDetector
    GridShieldSystem --> CryptoEngine
    GridShieldSystem --> PacketTransport
    GridShieldSystem --> AnomalyDetector
    GridShieldSystem --> PlatformServices
    
    TamperDetector --> PlatformServices
    CryptoEngine --> PlatformServices
    PacketTransport --> PlatformServices
```

---

## Layer Architecture

### Layer 1: Physical Security

**Purpose:** Detect physical tampering attempts in real-time

**Components:**
- `TamperDetector` - ISR-safe tamper event handler
- `TamperConfig` - Debounce timing, sensitivity settings
- Platform GPIO/Interrupt abstraction

**Detection Mechanisms:**
- Casing open detection (limit switch)
- Power loss monitoring (backup capacitor)
- Magnetic interference detection
- Physical shock detection

**Flow:**
```
Sensor Trigger → ISR Handler → Debounce → Validate → Set Flag → Emergency Alert
```

**Key Features:**
- Interrupt-driven (non-blocking)
- Debounce logic (configurable, default 50ms)
- Priority flagging for immediate transmission
- Operates on backup power during main power loss

**Files:**
- `include/common/hardware/tamper.hpp`
- `src/common/hardware/tamper.cpp`

---

### Layer 2: Network Security

**Purpose:** Ensure authenticity, integrity, and confidentiality of transmitted data

**Components:**
- `CryptoEngine` - ECC signing, AES-GCM encryption, SHA-256 hashing
- `PacketTransport` - Secure packet serialization and transmission
- `SecurePacket` - Wire protocol with cryptographic protection

**Packet Structure:**
```
[HEADER: 24B] [PAYLOAD: 0-512B] [SIGNATURE: 64B] [FOOTER: 1B]
```

**Security Features:**
- **Authentication:** ECDSA signature (secp256r1)
- **Integrity:** SHA-256 checksum
- **Replay Protection:** Sequence numbers
- **Frame Validation:** Magic bytes (0xA5, 0x5A)

**Cryptographic Algorithms:**
- **ECC:** 256-bit Elliptic Curve (secp256r1)
- **Signature:** ECDSA (64 bytes: r + s)
- **Encryption:** AES-256-GCM (planned)
- **Hashing:** SHA-256 (32 bytes)

**Files:**
- `include/common/security/crypto.hpp`
- `src/common/security/crypto.cpp`
- `include/common/network/packet.hpp`
- `src/common/network/packet.cpp`

---

### Layer 3: Application Security

**Purpose:** Detect abnormal consumption patterns indicating manipulation

**Components:**
- `AnomalyDetector` - Statistical deviation analyzer
- `ConsumptionProfile` - Historical baseline (24-hour profile)
- `CrossLayerValidation` - Multi-layer threat correlation

**Detection Logic:**

```
Expected Value = hourly_avg_wh[current_hour]
Deviation% = |current - expected| / expected * 100

if Deviation% > Threshold:
    Classify Anomaly Type
    Calculate Severity (Low → Critical)
    Generate Alert
```

**Anomaly Types:**
- `UnexpectedDrop` - Sudden consumption decrease (tampering indicator)
- `UnexpectedSpike` - Unusual consumption increase
- `ZeroConsumption` - Complete power draw cessation
- `PatternDeviation` - Behavioral change from profile

**Profile Learning:**
- Continuously updates 24-hour consumption baseline
- Confidence increases with more data samples
- Adapts to seasonal/behavioral changes

**Files:**
- `include/common/analytics/detector.hpp`
- `src/common/analytics/detector.cpp`

---

## Component Details

### Core Module

#### error.hpp - Result<T> Monad

Type-safe error handling without exceptions:

```cpp
Result<MeterReading> read_meter() {
    if (sensor_failed) {
        return GS_MAKE_ERROR(ErrorCode::SensorReadFailure);
    }
    return Result<MeterReading>(reading);
}

// Usage
auto result = read_meter();
if (result.is_ok()) {
    process(result.value());
} else {
    log_error(result.error());
}
```

**Features:**
- No exceptions (embedded-safe)
- Automatic error context capture (file, line)
- `GS_TRY` macro for early returns
- Move semantics for zero-copy

#### types.hpp - Domain Types

**MeterReading (24 bytes):**
```cpp
struct MeterReading {
    timestamp_t timestamp;    // 8 bytes
    uint32_t energy_wh;       // 4 bytes
    uint32_t voltage_mv;      // 4 bytes
    uint16_t current_ma;      // 2 bytes
    uint16_t power_factor;    // 2 bytes
    uint8_t phase;            // 1 byte
    uint8_t reserved[3];      // 3 bytes (alignment)
};
```

**StaticBuffer<T, N>:**
- Fixed-size container (no heap allocation)
- Placement new for type-safe storage
- Move-only semantics
- Ideal for embedded systems

#### system.hpp - Main Orchestrator

**Responsibilities:**
- Lifecycle management (init → start → operate → shutdown)
- Layer coordination
- Cross-layer validation
- Automatic tamper response

**State Machine:**
```
Uninitialized → Initializing → Ready → Operating
                                  ↓
                              Tampered → Error → Shutdown
```

---

### Platform Abstraction Layer

**Interface Definitions:**
```cpp
class IPlatformTime {
    virtual timestamp_t get_timestamp_ms() = 0;
    virtual void delay_ms(uint32_t ms) = 0;
};

class IPlatformGPIO {
    virtual Result<void> configure(uint8_t pin, PinMode mode) = 0;
    virtual Result<bool> read(uint8_t pin) = 0;
    virtual Result<void> write(uint8_t pin, bool value) = 0;
};

class IPlatformCrypto {
    virtual Result<void> random_bytes(uint8_t* buffer, size_t length) = 0;
    virtual Result<uint32_t> crc32(const uint8_t* data, size_t length) = 0;
    virtual Result<void> sha256(const uint8_t* data, size_t length, uint8_t* hash) = 0;
};
```

**Implementations:**
- **Native:** `platform_native.hpp` - Uses `rweather/Crypto` & C++ STL
- **Arduino:** `platform_arduino.hpp` - Uses `rweather/Crypto` & Arduino API

**Benefits:**
- Platform-agnostic core logic
- Easy testing on PC
- Simple hardware porting

---

## Data Flow

### Normal Operation Flow

```mermaid
graph TD
    A[Sensor Reading] --> B{Time Interval?}
    B -->|Reading Due| C[Read Meter Hardware]
    B -->|Heartbeat Due| D[Generate Heartbeat]
    
    C --> E[Create MeterReading]
    E --> F[Anomaly Analysis]
    F --> G[Update Profile]
    
    G --> H[Sign Data with ECC]
    H --> I[Build SecurePacket]
    I --> J[Serialize to Bytes]
    J --> K[Transmit via Comm]
    K --> L[Server Receive]
    
    D --> H
    
    style F fill:#e3f2fd
    style H fill:#e8f5e9
```

### Tamper Event Flow

```mermaid
graph TD
    A[Tamper Sensor Triggered] --> B[ISR Handler]
    B --> C{Debounce Valid?}
    C -->|No| D[Ignore]
    C -->|Yes| E[Set is_tampered = true]
    
    E --> F[Store Timestamp]
    F --> G[Determine Tamper Type]
    G --> H[Main Loop Detects]
    
    H --> I[Transition State to TAMPERED]
    I --> J[Set Mode to TamperResponse]
    J --> K[Create TamperEvent]
    
    K --> L[Sign with Priority=Emergency]
    L --> M[Bypass Normal Queue]
    M --> N[Transmit Immediately]
    N --> O[Server Alerts P2TL]
    
    style E fill:#ffebee
    style I fill:#fff3e0
    style N fill:#ffcdd2
```

### Anomaly Detection Flow

```mermaid
graph TD
    A[MeterReading Received] --> B[Extract Timestamp]
    B --> C[Calculate Expected Value from Profile]
    C --> D[Compute Deviation%]
    
    D --> E{Deviation > Threshold?}
    E -->|No| F[Report: None]
    E -->|Yes| G[Classify Anomaly Type]
    
    G --> H{Current < Expected?}
    H -->|Yes| I[Type: UnexpectedDrop]
    H -->|No| J[Type: UnexpectedSpike]
    
    I --> K[Calculate Severity]
    J --> K
    
    K --> L{Deviation >= 80%?}
    L -->|Yes| M[Severity: Critical]
    L -->|No| N{Deviation >= 60%?}
    N -->|Yes| O[Severity: High]
    N -->|No| P[Severity: Medium/Low]
    
    M --> Q[Generate Alert]
    O --> Q
    P --> Q
    
    Q --> R[Cross-Layer Validation]
    R --> S{Physical + Consumption?}
    S -->|Yes| T[Priority: Emergency]
    S -->|No| U[Priority: Normal]
    
    style I fill:#ffebee
    style M fill:#d32f2f
    style T fill:#ff5252
```

---

## Security Model

### Threat Model

**Threats Addressed:**

1. **Physical Tampering**
   - Casing removal/opening
   - Component replacement
   - Wire bypass
   - **Mitigation:** Tamper switches, power-loss detection

2. **Network Attacks**
   - Man-in-the-middle (MITM)
   - Replay attacks
   - Data injection
   - **Mitigation:** ECC signatures, sequence numbers, integrity checksums

3. **Consumption Manipulation**
   - Meter slowdown
   - Load masking
   - Data falsification
   - **Mitigation:** Anomaly detection, profile deviation analysis

### Defense Layers

```
┌─────────────────────────────────────────────────┐
│  Layer 3: Application Security                 │
│  • Anomaly Detection                            │
│  • Profile Learning                             │
│  • Cross-Layer Correlation                      │
├─────────────────────────────────────────────────┤
│  Layer 2: Network Security                     │
│  • ECDSA Authentication                         │
│  • AES-GCM Encryption                           │
│  • SHA-256 Integrity                            │
├─────────────────────────────────────────────────┤
│  Layer 1: Physical Security                    │
│  • Tamper Detection                             │
│  • Power-Loss Alerting                          │
│  • ISR-Safe Handlers                            │
└─────────────────────────────────────────────────┘
```

### Cryptographic Properties

**Elliptic Curve Cryptography (secp256r1):**
- Key Size: 256 bits (32 bytes private, 64 bytes public)
- Security Level: Equivalent to 3072-bit RSA
- Signature Size: 64 bytes (r + s)
- **Why ECC?** Smaller keys = less bandwidth + faster computation for embedded

**AES-256-GCM:**
- Encryption: 256-bit key
- Authentication: Built-in MAC (16-byte tag)
- Mode: Galois/Counter Mode (authenticated encryption)

**SHA-256:**
- Hash size: 256 bits (32 bytes)
- Collision resistance: 2^128 operations
- Used for: Packet integrity, message digests

---

## File Organization

### Directory Structure

```
gridshield/
├── CMakeLists.txt                      # Root build configuration
├── CMakePresets.json                   # Build presets (native, AVR)
├── BUILD.md                            # Build instructions
├── README.md                           # Project overview
│
├── include/                            # Header files
│   ├── common/                         # Platform-agnostic
│   │   ├── utils/
│   │   │   └── gs_macros.hpp           # C++17 macros (GS_MOVE, GS_LIKELY, etc.)
│   │   ├── core/
│   │   │   ├── error.hpp               # Result<T> monad, ErrorCode
│   │   │   ├── types.hpp               # MeterReading, StaticBuffer, etc.
│   │   │   └── system.hpp              # GridShieldSystem orchestrator
│   │   ├── security/
│   │   │   └── crypto.hpp              # CryptoEngine, ECCKeyPair
│   │   ├── hardware/
│   │   │   └── tamper.hpp              # TamperDetector
│   │   ├── network/
│   │   │   └── packet.hpp              # SecurePacket, PacketTransport
│   │   └── analytics/
│   │       └── detector.hpp            # AnomalyDetector
│   │
│   ├── platform/
│   │   └── platform.hpp                # HAL interfaces (IPlatformTime, etc.)
│   │
│   ├── native/
│   │   └── platform_native.hpp         # PC testing implementation
│   │
│   └── arduino/
│       └── platform_arduino.hpp        # AVR production implementation
│
├── src/                                # Implementation files
│   ├── common/
│   │   ├── core/system.cpp
│   │   ├── hardware/tamper.cpp
│   │   ├── security/crypto.cpp
│   │   ├── network/packet.cpp
│   │   └── analytics/detector.cpp
│   │
│   ├── platform/platform.cpp           # HAL common code
│   │
│   ├── native/
│   │   └── main.cpp                    # PC entry point (testing)
│   │
│   └── arduino/
│       └── gridshield.ino              # Arduino entry point (production)
│
├── docs/                               # Documentation
│   ├── ARCHITECTURE.md                 # This file
│   ├── API.md                          # API reference
│   ├── PROPOSAL.md                     # Project proposal
│   └── requirements.md                 # System requirements
│
└── bin/                                # Build outputs
    ├── NATIVE/GridShield               # PC executable
    └── AVR/GridShield.hex              # Arduino firmware
```

### Compilation Units

**Header-Only:**
- `gs_macros.hpp` - Platform detection, move semantics
- `error.hpp` - Result<T> template
- `types.hpp` - Domain type definitions
- `platform.hpp` - HAL interfaces

**Implementation Files:**
- `system.cpp` - Main orchestrator logic
- `tamper.cpp` - Physical security layer
- `crypto.cpp` - Cryptographic operations
- `packet.cpp` - Network protocol
- `detector.cpp` - Anomaly detection
- `main.cpp` / `gridshield.ino` - Entry points

---

## Memory Architecture

### Static Memory (BSS/Data)

| Component | Size | Description |
|-----------|------|-------------|
| System state | ~200 B | Configuration, flags, timestamps |
| Crypto buffers | ~256 B | Keys, signatures, hashes |
| Packet buffers | ~650 B | Header + payload + footer |
| Analytics profile | ~200 B | 24-hour consumption history |
| Recent readings | ~2.4 KB | StaticBuffer<MeterReading, 100> |
| **Total** | **~3.7 KB** | Well within 8 KB RAM limit |

### Stack Usage

| Context | Size | Description |
|---------|------|-------------|
| Main task | ~2.0 KB | Function call depth, local variables |
| ISR handlers | ~512 B | Interrupt service routine stack |
| **Total** | **~2.5 KB** | Peak usage during normal operation |

### Code Size (Flash)

| Component | Size (Estimated) | Notes |
|-----------|------------------|-------|
| Core logic | ~20 KB | System, tamper, analytics |
| Platform abstraction | ~5 KB | HAL implementations |
| Crypto (placeholder) | ~10 KB | Placeholder hashes/signatures |
| **Subtotal** | **~35 KB** | Without production crypto |
| + mbedTLS/uECC | +50 KB | Production cryptography |
| **Production Total** | **~85 KB** | Fits in 256 KB flash |

**Target Hardware:**
- **Arduino Mega 2560:** 256 KB flash, 8 KB RAM ✅
- **Arduino Uno:** 32 KB flash, 2 KB RAM ❌ (insufficient)

---

## Production Deployment

### Step 1: Hardware Platform Selection

**Recommended:** Arduino Mega 2560
- MCU: ATmega2560 @ 16 MHz
- Flash: 256 KB
- RAM: 8 KB
- EEPROM: 4 KB

**Alternative:** ESP32
- MCU: Xtensa LX6 @ 240 MHz
- Flash: 4 MB
- RAM: 520 KB
- WiFi/Bluetooth: Integrated

### Step 2: Crypto Library Integration

**Replace placeholder crypto with production libraries:**

```cpp
// In crypto.cpp
#include <uECC.h>        // Micro-ECC for ECDSA
#include <Crypto.h>      // Arduino Crypto for SHA-256

core::Result<void> CryptoEngine::sign(...) {
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    if (!uECC_sign(keypair.get_private_key(), hash, 32, signature_out, curve)) {
        return GS_MAKE_ERROR(ErrorCode::SignatureInvalid);
    }
    return Result<void>();
}
```

### Step 3: Secure Key Storage

**Option A: ATECC608 Secure Element**
```cpp
#include <ATECC608.h>

ATECC608 secure_element;
secure_element.getPublicKey(slot, public_key);
secure_element.sign(slot, hash, signature);
```

**Option B: EEPROM Storage (Less Secure)**
```cpp
#include <EEPROM.h>

// Store encrypted key
EEPROM.put(KEY_ADDRESS, encrypted_private_key);
```

### Step 4: Communication Module

**LoRa (Long-Range, Low-Power):**
```cpp
#include <LoRa.h>

class LoRaComm : public IPlatformComm {
    Result<size_t> send(const uint8_t* data, size_t length) override {
        LoRa.beginPacket();
        LoRa.write(data, length);
        return Result<size_t>(LoRa.endPacket());
    }
};
```

**NB-IoT (Cellular Network):**
```cpp
#include <TinyGsmClient.h>

class NBIoTComm : public IPlatformComm {
    TinyGsm modem(SerialAT);
    Result<size_t> send(const uint8_t* data, size_t length) override {
        return modem.sendData(data, length);
    }
};
```

### Step 5: Sensor Integration

**Current Sensor (ACS712):**
```cpp
const int CURRENT_PIN = A0;
float read_current_ma() {
    int raw = analogRead(CURRENT_PIN);
    float voltage = (raw / 1024.0) * 5.0;
    float current = (voltage - 2.5) / 0.185;  // 5A sensor
    return current * 1000.0;  // Convert to mA
}
```

**Voltage Sensor (ZMPT101B):**
```cpp
const int VOLTAGE_PIN = A1;
float read_voltage_mv() {
    int raw = analogRead(VOLTAGE_PIN);
    float voltage = (raw / 1024.0) * 5.0 * (220.0 / 5.0);  // Scale factor
    return voltage * 1000.0;  // Convert to mV
}
```

### Step 6: Testing Strategy

**Unit Testing (PC):**
```bash
cmake --preset native-debug
cmake --build --preset native-debug
./bin/NATIVE/GridShield
```

**Integration Testing (Arduino):**
```bash
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino
arduino-cli monitor -p COM3 -b 115200
```

**Hardware-in-Loop (HIL):**
- Simulate tamper events with physical switches
- Inject current/voltage variations
- Monitor serial output for correct responses

---

## Performance Benchmarks

**Target MCU:** ATmega2560 @ 16 MHz

| Operation | Time | Notes |
|-----------|------|-------|
| Tamper detection (ISR) | <5 ms | Interrupt-driven |
| SHA-256 hash (512B) | ~15 ms | Software implementation |
| ECC signature | ~50 ms | uECC library |
| AES-256 encrypt (512B) | ~20 ms | Crypto library |
| Anomaly analysis | <10 ms | Statistical computation |
| **Full cycle** | **<100 ms** | Sensor → Encrypt → Transmit |

**Power Consumption:**
- Active (processing): ~50 mA @ 5V
- Sleep (between cycles): <1 mA
- Tamper alert (backup power): ~20 mA for 5 seconds

---

## Extensibility Points

### Adding New Sensors

1. Extend `TamperType` enum:
```cpp
enum class TamperType : uint8_t {
    // Existing...
    MagneticInterference = 2,
    VibrationDetected = 7,     // NEW
};
```

2. Implement detection logic in `TamperDetector::handle_tamper_event()`

### Adding New Packet Types

1. Extend `PacketType` enum:
```cpp
enum class PacketType : uint8_t {
    // Existing...
    ConfigUpdate = 7,          // NEW
};
```

2. Add handler in `GridShieldSystem::process_cycle()`

### Adding New Anomaly Detectors

1. Implement `IAnomalyDetector` interface
2. Register with `GridShieldSystem`
3. Configure thresholds in `SystemConfig`

---

## Security Hardening Checklist

### Development Phase
- ✅ Zero heap allocation (predictable memory)
- ✅ Bounds checking on all buffers
- ✅ Volatile state in ISR handlers
- ✅ Result-based error handling (no exceptions)
- ✅ Type-safe interfaces (no void*)
- ✅ Const correctness throughout

### Production Phase
- [ ] Enable stack canaries (`-fstack-protector-strong`)
- [ ] Enable position-independent code (PIE)
- [ ] Implement secure erase for cryptographic keys
- [ ] Add watchdog timer support
- [ ] Implement secure boot chain
- [ ] Add OTA update with signature verification
- [ ] Implement rate limiting for anti-DoS
- [ ] Add encrypted logging to flash

---

## License & Attribution

**License:** MIT License - See LICENSE.md

**Developed by:**
- Muhammad Ichwan Fauzi (System Architecture)
- Rafi Indra Pramudhito Zuhayr (Firmware Implementation)
- Cesar Ardika Bhayangkara (Hardware Integration)

**Institut Teknologi PLN - 2025**

---

## References

- [C++17 Standard](https://en.cppreference.com/w/cpp/17)
- [Arduino Language Reference](https://www.arduino.cc/reference/en/)
- [micro-ecc Library](https://github.com/kmackay/micro-ecc)
- [Arduino Crypto Library](https://github.com/rweather/arduinolibs)
- [CMake Documentation](https://cmake.org/documentation/)