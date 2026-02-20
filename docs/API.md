# GridShield - API Reference

Complete API documentation for GridShield Multi-Layer AMI Security System.

**Version:** 1.0.0  
**Language:** C++17  
**Namespace:** `gridshield`

---

## Table of Contents

- [Core Module](#core-module)
- [Security Module](#security-module)
- [Hardware Module](#hardware-module)
- [Network Module](#network-module)
- [Analytics Module](#analytics-module)
- [Platform Abstraction](#platform-abstraction)
- [Utilities Module](#utilities-module)
- [Error Handling](#error-handling)
- [Type Definitions](#type-definitions)

---

## Core Module

### GridShieldSystem

**Header:** `include/common/core/system.hpp`

Main orchestrator coordinating all security layers.

#### Constructor

```cpp
GridShieldSystem() noexcept;
```

Creates an uninitialized system instance.

#### Public Methods

##### initialize()

```cpp
core::Result<void> initialize(
    const SystemConfig& config,
    platform::PlatformServices& platform
) noexcept;
```

Initializes the system with configuration and platform services.

**Parameters:**
- `config` - System configuration (meter ID, intervals, baseline profile)
- `platform` - Platform abstraction layer services

**Returns:** `Result<void>` - Success or error code

**Errors:**
- `SystemAlreadyInitialized` - System already initialized
- `InvalidParameter` - Invalid platform services
- `ResourceExhausted` - Out of memory

**Example:**
```cpp
SystemConfig config;
config.meter_id = 0x1234567890ABCDEF;
config.heartbeat_interval_ms = 60000;

platform::PlatformServices services;
// ... setup services

GridShieldSystem system;
auto result = system.initialize(config, services);
if (result.is_error()) {
    // Handle error
}
```

---

##### start()

```cpp
core::Result<void> start() noexcept;
```

Starts the security system (begins monitoring and processing).

**Returns:** `Result<void>` - Success or error code

**Errors:**
- `InvalidState` - System not ready to start

---

##### process_cycle()

```cpp
core::Result<void> process_cycle() noexcept;
```

Executes one processing cycle (should be called in main loop).

**Returns:** `Result<void>` - Success or error code

**Description:**
- Checks for tamper events
- Sends heartbeats
- Processes periodic readings
- Performs cross-layer validation

**Example:**
```cpp
while (true) {
    auto result = system.process_cycle();
    if (result.is_error()) {
        log_error(result.error());
    }
    platform.time->delay_ms(100);
}
```

---

##### send_meter_reading()

```cpp
core::Result<void> send_meter_reading(
    const core::MeterReading& reading
) noexcept;
```

Sends a meter reading with cryptographic protection.

**Parameters:**
- `reading` - Meter reading data (energy, voltage, current, etc.)

**Returns:** `Result<void>` - Success or error code

**Example:**
```cpp
core::MeterReading reading;
reading.timestamp = platform.time->get_timestamp_ms();
reading.energy_wh = 1500;
reading.voltage_mv = 220000;
reading.current_ma = 6818;

auto result = system.send_meter_reading(reading);
```

---

##### send_tamper_alert()

```cpp
core::Result<void> send_tamper_alert() noexcept;
```

Sends an emergency tamper alert with highest priority.

**Returns:** `Result<void>` - Success or error code

---

##### send_heartbeat()

```cpp
core::Result<void> send_heartbeat() noexcept;
```

Sends a heartbeat packet to indicate system is alive.

**Returns:** `Result<void>` - Success or error code

---

##### stop()

```cpp
core::Result<void> stop() noexcept;
```

Stops the security system (pauses monitoring).

**Returns:** `Result<void>` - Success or error code

---

##### shutdown()

```cpp
core::Result<void> shutdown() noexcept;
```

Completely shuts down the system and releases resources.

**Returns:** `Result<void>` - Success or error code

---

##### get_state()

```cpp
core::SystemState get_state() const noexcept;
```

Returns the current system state.

**Returns:** Current `SystemState` enum value

**Possible States:**
- `Uninitialized`
- `Initializing`
- `Ready`
- `Operating`
- `Tampered`
- `PowerLoss`
- `Error`
- `Shutdown`

---

##### get_mode()

```cpp
OperationMode get_mode() const noexcept;
```

Returns the current operation mode.

**Returns:** Current `OperationMode` enum value

**Possible Modes:**
- `Normal`
- `TamperResponse`
- `LowPower`
- `Maintenance`

---

### SystemConfig

**Header:** `include/common/core/system.hpp`

Configuration structure for GridShieldSystem.

#### Members

```cpp
struct SystemConfig {
    core::meter_id_t meter_id;                      // Unique meter identifier
    hardware::TamperConfig tamper_config;           // Tamper detection config
    analytics::ConsumptionProfile baseline_profile; // Baseline consumption
    uint32_t heartbeat_interval_ms;                 // Heartbeat interval
    uint32_t reading_interval_ms;                   // Reading interval
};
```

**Example:**
```cpp
SystemConfig config;
config.meter_id = 0xABCDEF1234567890;
config.heartbeat_interval_ms = 60000;  // 1 minute
config.reading_interval_ms = 5000;     // 5 seconds

config.tamper_config.sensor_pin = 2;
config.tamper_config.debounce_ms = 50;

for (size_t i = 0; i < 24; ++i) {
    config.baseline_profile.hourly_avg_wh[i] = 1200;
}
```

---

## Security Module

### CryptoEngine

**Header:** `include/common/security/crypto.hpp`

Lightweight cryptography engine for ECC signatures and AES-GCM encryption.

#### Constructor

```cpp
explicit CryptoEngine(platform::IPlatformCrypto& platform_crypto) noexcept;
```

Creates a crypto engine with platform-specific crypto primitives.

---

#### Public Methods

##### generate_keypair()

```cpp
core::Result<void> generate_keypair(ECCKeyPair& keypair) noexcept;
```

Generates a new ECC key pair (secp256r1).

**Parameters:**
- `keypair` - Output key pair object

**Returns:** `Result<void>` - Success or error code

**Example:**
```cpp
security::ECCKeyPair device_keypair;
auto result = crypto_engine.generate_keypair(device_keypair);
```

---

##### sign()

```cpp
core::Result<void> sign(
    const ECCKeyPair& keypair,
    const uint8_t* message,
    size_t msg_len,
    uint8_t* signature_out
) noexcept;
```

Signs a message with ECDSA.

**Parameters:**
- `keypair` - Key pair (must have private key)
- `message` - Message to sign
- `msg_len` - Message length
- `signature_out` - Output buffer (64 bytes)

**Returns:** `Result<void>` - Success or error code

**Example:**
```cpp
uint8_t signature[64];
auto result = crypto_engine.sign(
    device_keypair,
    data, data_len,
    signature
);
```

---

##### verify()

```cpp
core::Result<bool> verify(
    const ECCKeyPair& keypair,
    const uint8_t* message,
    size_t msg_len,
    const uint8_t* signature
) noexcept;
```

Verifies an ECDSA signature.

**Parameters:**
- `keypair` - Key pair (must have public key)
- `message` - Original message
- `msg_len` - Message length
- `signature` - Signature to verify (64 bytes)

**Returns:** `Result<bool>` - true if valid, false if invalid

---

##### hash_sha256()

```cpp
core::Result<void> hash_sha256(
    const uint8_t* data,
    size_t length,
    uint8_t* hash_out
) noexcept;
```

Computes SHA-256 hash.

**Parameters:**
- `data` - Input data
- `length` - Data length
- `hash_out` - Output buffer (32 bytes)

**Returns:** `Result<void>` - Success or error code

---

##### random_bytes()

```cpp
core::Result<void> random_bytes(
    uint8_t* buffer,
    size_t length
) noexcept;
```

Generates cryptographically secure random bytes.

**Parameters:**
- `buffer` - Output buffer
- `length` - Number of bytes to generate

**Returns:** `Result<void>` - Success or error code

---

### ECCKeyPair

**Header:** `include/common/security/crypto.hpp`

ECC key pair container (secp256r1 curve).

#### Public Methods

##### generate()

```cpp
core::Result<void> generate() noexcept;
```

Generates a new random key pair (requires production crypto library).

---

##### load_private_key()

```cpp
core::Result<void> load_private_key(
    const uint8_t* key,
    size_t length
) noexcept;
```

Loads a private key from bytes.

**Parameters:**
- `key` - Private key bytes (32 bytes)
- `length` - Must be 32

---

##### load_public_key()

```cpp
core::Result<void> load_public_key(
    const uint8_t* key,
    size_t length
) noexcept;
```

Loads a public key from bytes.

**Parameters:**
- `key` - Public key bytes (64 bytes: x + y)
- `length` - Must be 64

---

##### get_private_key()

```cpp
const uint8_t* get_private_key() const noexcept;
```

Returns pointer to private key (32 bytes) or nullptr.

---

##### get_public_key()

```cpp
const uint8_t* get_public_key() const noexcept;
```

Returns pointer to public key (64 bytes) or nullptr.

---

##### has_private_key()

```cpp
bool has_private_key() const noexcept;
```

Checks if private key is loaded.

---

##### has_public_key()

```cpp
bool has_public_key() const noexcept;
```

Checks if public key is loaded.

---

##### clear()

```cpp
void clear() noexcept;
```

Securely erases all key material.

---

### KeyStorage

**Header:** `include/common/security/key_storage.hpp`

Manages secure storage of cryptographic keys in persistent memory (EEPROM/Flash).

#### Storage Layout

```
[MAGIC: 4B] [VERSION: 1B] [RSVD: 3B] [PUBKEY: 64B] [PRIVKEY: 32B] [CRC32: 4B]
Total: 108 bytes
```

#### Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `STORAGE_MAGIC` | `0x47534B53` | "GSKS" identifier |
| `STORAGE_VERSION` | `1` | Storage format version |
| `STORAGE_SIZE` | `108` | Total bytes used |
| `DEFAULT_ADDRESS` | `0` | Default EEPROM address |

#### Constructor

```cpp
explicit KeyStorage(platform::PlatformServices& platform) noexcept;
```

Creates a key storage manager with platform services.

**Parameters:**
- `platform` - Platform abstraction layer services (requires storage and crypto)

---

#### Public Methods

##### save()

```cpp
core::Result<void> save(
    const ECCKeyPair& keypair,
    uint32_t address = DEFAULT_ADDRESS
) noexcept;
```

Saves a key pair to persistent storage.

**Parameters:**
- `keypair` - Key pair to save (must have both private and public keys)
- `address` - Storage address (default: 0)

**Returns:** `Result<void>` - Success or error code

**Errors:**
- `KeyGenerationFailed` - Invalid keypair (missing keys)
- `CryptoFailure` - CRC32 computation failed
- `HardwareFailure` - Storage write failed

**Example:**
```cpp
security::KeyStorage storage(platform);
security::ECCKeyPair keypair;

// Generate or load keypair...
crypto_engine.generate_keypair(keypair);

// Save to EEPROM
auto result = storage.save(keypair);
if (result.is_error()) {
    // Handle storage error
}
```

---

##### load()

```cpp
core::Result<void> load(
    ECCKeyPair& keypair,
    uint32_t address = DEFAULT_ADDRESS
) noexcept;
```

Loads a key pair from persistent storage.

**Parameters:**
- `keypair` - Output key pair object
- `address` - Storage address (default: 0)

**Returns:** `Result<void>` - Success or error code

**Errors:**
- `IntegrityViolation` - Storage not initialized or corrupted (invalid magic/CRC)
- `HardwareFailure` - Storage read failed
- `CryptoFailure` - Key loading failed

**Example:**
```cpp
security::KeyStorage storage(platform);
security::ECCKeyPair keypair;

auto result = storage.load(keypair);
if (result.is_error()) {
    if (result.error().code == core::ErrorCode::IntegrityViolation) {
        // No saved keys - generate new ones
        crypto_engine.generate_keypair(keypair);
        storage.save(keypair);
    }
}
```

---

##### erase()

```cpp
core::Result<void> erase(uint32_t address = DEFAULT_ADDRESS) noexcept;
```

Erases keys from storage (fills with zeros).

**Parameters:**
- `address` - Storage address (default: 0)

**Returns:** `Result<void>` - Success or error code

**Example:**
```cpp
// Factory reset - erase all stored keys
storage.erase();
```

---

## Hardware Module

### TamperDetector

**Header:** `include/common/hardware/tamper.hpp`

Physical tamper detection with interrupt support.

#### Public Methods

##### initialize()

```cpp
core::Result<void> initialize(
    const TamperConfig& config,
    platform::PlatformServices& platform
) noexcept;
```

Initializes the tamper detector.

**Parameters:**
- `config` - Tamper detection configuration
- `platform` - Platform services

**Returns:** `Result<void>` - Success or error code

---

##### start()

```cpp
core::Result<void> start() noexcept;
```

Starts tamper monitoring (attaches interrupt).

**Returns:** `Result<void>` - Success or error code

---

##### stop()

```cpp
core::Result<void> stop() noexcept;
```

Stops tamper monitoring (detaches interrupt).

**Returns:** `Result<void>` - Success or error code

---

##### is_tampered()

```cpp
bool is_tampered() const noexcept;
```

Checks if tamper event is active.

**Returns:** true if tampered, false otherwise

---

##### get_tamper_type()

```cpp
TamperType get_tamper_type() const noexcept;
```

Returns the type of tamper detected.

**Returns:** `TamperType` enum value

**Possible Types:**
- `None`
- `CasingOpened`
- `MagneticInterference`
- `TemperatureAnomaly`
- `VibrationDetected`
- `PowerCutAttempt`
- `PhysicalShock`

---

##### get_tamper_timestamp()

```cpp
core::timestamp_t get_tamper_timestamp() const noexcept;
```

Returns the timestamp when tamper was detected.

**Returns:** Timestamp in milliseconds

---

##### acknowledge_tamper()

```cpp
core::Result<void> acknowledge_tamper() noexcept;
```

Acknowledges the tamper event (does not clear it).

**Returns:** `Result<void>` - Success or error code

---

##### reset()

```cpp
core::Result<void> reset() noexcept;
```

Resets tamper state (clears tamper flag).

**Returns:** `Result<void>` - Success or error code

---

### TamperConfig

**Header:** `include/common/hardware/tamper.hpp`

Configuration for tamper detection.

#### Members

```cpp
struct TamperConfig {
    uint8_t sensor_pin;         // GPIO pin for tamper switch
    uint8_t backup_power_pin;   // GPIO pin for power monitoring
    uint16_t debounce_ms;       // Debounce time in milliseconds
    uint8_t sensitivity;        // Sensitivity level (0-255)
};
```

---

## Network Module

### SecurePacket

**Header:** `include/common/network/packet.hpp`

Secure packet with cryptographic protection.

#### Public Methods

##### build()

```cpp
core::Result<void> build(
    PacketType type,
    core::meter_id_t meter_id,
    core::Priority priority,
    const uint8_t* payload,
    uint16_t payload_len,
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair
) noexcept;
```

Builds a secure packet with signature.

**Parameters:**
- `type` - Packet type (MeterData, TamperAlert, Heartbeat, etc.)
- `meter_id` - Meter identifier
- `priority` - Packet priority
- `payload` - Payload data
- `payload_len` - Payload length (max 512 bytes)
- `crypto` - Crypto engine for signing
- `keypair` - Device key pair

**Returns:** `Result<void>` - Success or error code

---

##### parse()

```cpp
core::Result<void> parse(
    const uint8_t* buffer,
    size_t buffer_len,
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& server_keypair
) noexcept;
```

Parses and verifies a received packet.

**Parameters:**
- `buffer` - Received packet bytes
- `buffer_len` - Buffer length
- `crypto` - Crypto engine for verification
- `server_keypair` - Server public key

**Returns:** `Result<void>` - Success or error code

---

##### serialize()

```cpp
core::Result<size_t> serialize(
    uint8_t* buffer,
    size_t buffer_size
) const noexcept;
```

Serializes packet to bytes for transmission.

**Parameters:**
- `buffer` - Output buffer
- `buffer_size` - Buffer size

**Returns:** `Result<size_t>` - Number of bytes written

---

##### is_valid()

```cpp
bool is_valid() const noexcept;
```

Checks if packet is valid and ready to send/has been parsed successfully.

**Returns:** true if valid, false otherwise

---

##### header()

```cpp
const PacketHeader& header() const noexcept;
```

Returns the packet header.

---

##### payload()

```cpp
const uint8_t* payload() const noexcept;
```

Returns pointer to payload data.

---

##### payload_length()

```cpp
uint16_t payload_length() const noexcept;
```

Returns payload length in bytes.

---

### PacketTransport

**Header:** `include/common/network/packet.hpp`

Handles packet transmission and reception.

#### Constructor

```cpp
explicit PacketTransport(platform::IPlatformComm& comm) noexcept;
```

Creates a packet transport with communication interface.

---

#### Public Methods

##### send_packet()

```cpp
core::Result<void> send_packet(
    const SecurePacket& packet,
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair
) noexcept;
```

Sends a secure packet.

**Parameters:**
- `packet` - Packet to send
- `crypto` - Crypto engine
- `keypair` - Device key pair

**Returns:** `Result<void>` - Success or error code

---

##### receive_packet()

```cpp
core::Result<SecurePacket> receive_packet(
    security::ICryptoEngine& crypto,
    const security::ECCKeyPair& keypair,
    uint32_t timeout_ms
) noexcept;
```

Receives and parses a secure packet.

**Parameters:**
- `crypto` - Crypto engine
- `keypair` - Server public key
- `timeout_ms` - Timeout in milliseconds

**Returns:** `Result<SecurePacket>` - Received packet or error

---

### PacketType

**Header:** `include/common/network/packet.hpp`

Packet type enumeration.

```cpp
enum class PacketType : uint8_t {
    Invalid = 0,
    MeterData = 1,
    TamperAlert = 2,
    Heartbeat = 3,
    Command = 4,
    Acknowledgment = 5,
    KeyExchange = 6
};
```

---

## Analytics Module

### AnomalyDetector

**Header:** `include/common/analytics/detector.hpp`

Consumption anomaly detection with profile learning.

#### Public Methods

##### initialize()

```cpp
core::Result<void> initialize(
    const ConsumptionProfile& baseline_profile
) noexcept;
```

Initializes detector with baseline profile.

**Parameters:**
- `baseline_profile` - Historical consumption baseline

**Returns:** `Result<void>` - Success or error code

---

##### analyze()

```cpp
core::Result<AnomalyReport> analyze(
    const core::MeterReading& reading
) noexcept;
```

Analyzes a meter reading for anomalies.

**Parameters:**
- `reading` - Meter reading to analyze

**Returns:** `Result<AnomalyReport>` - Analysis report

**Example:**
```cpp
auto result = anomaly_detector.analyze(reading);
if (result.is_ok()) {
    const auto& report = result.value();
    if (report.severity >= AnomalySeverity::High) {
        trigger_alert(report);
    }
}
```

---

##### update_profile()

```cpp
core::Result<void> update_profile(
    const core::MeterReading& reading
) noexcept;
```

Updates consumption profile with new reading (learning).

**Parameters:**
- `reading` - New meter reading

**Returns:** `Result<void>` - Success or error code

---

##### get_profile()

```cpp
const ConsumptionProfile& get_profile() const noexcept;
```

Returns the current consumption profile.

**Returns:** Reference to consumption profile

---

##### reset_profile()

```cpp
core::Result<void> reset_profile() noexcept;
```

Resets the consumption profile to defaults.

**Returns:** `Result<void>` - Success or error code

---

### AnomalyReport

**Header:** `include/common/analytics/detector.hpp`

Report of anomaly analysis.

#### Members

```cpp
struct AnomalyReport {
    core::timestamp_t timestamp;      // When analyzed
    AnomalyType type;                 // Type of anomaly
    AnomalySeverity severity;         // Severity level
    uint16_t confidence;              // Confidence (0-100)
    uint32_t current_value;           // Current reading
    uint32_t expected_value;          // Expected reading
    uint32_t deviation_percent;       // Deviation percentage
};
```

---

### AnomalyType

```cpp
enum class AnomalyType : uint8_t {
    None = 0,
    UnexpectedDrop = 1,
    UnexpectedSpike = 2,
    PatternDeviation = 3,
    ZeroConsumption = 4,
    ErraticBehavior = 5
};
```

---

### AnomalySeverity

```cpp
enum class AnomalySeverity : uint8_t {
    None = 0,
    Low = 1,
    Medium = 2,
    High = 3,
    Critical = 4
};
```

---

## Platform Abstraction

### PlatformServices

**Header:** `include/platform/platform.hpp`

Aggregates all platform interfaces.

#### Members

```cpp
struct PlatformServices {
    IPlatformTime* time;
    IPlatformGPIO* gpio;
    IPlatformInterrupt* interrupt;
    IPlatformCrypto* crypto;
    IPlatformStorage* storage;
    IPlatformComm* comm;
};
```

#### Methods

##### is_valid()

```cpp
bool is_valid() const noexcept;
```

Checks if all required services are non-null.

**Returns:** true if valid, false otherwise

---

### IPlatformTime

**Header:** `include/platform/platform.hpp`

Time and delay abstraction.

#### Methods

```cpp
virtual core::timestamp_t get_timestamp_ms() noexcept = 0;
virtual void delay_ms(uint32_t ms) noexcept = 0;
```

---

### IPlatformGPIO

**Header:** `include/platform/platform.hpp`

GPIO abstraction.

#### Methods

```cpp
virtual core::Result<void> configure(uint8_t pin, PinMode mode) noexcept = 0;
virtual core::Result<bool> read(uint8_t pin) noexcept = 0;
virtual core::Result<void> write(uint8_t pin, bool value) noexcept = 0;
```

#### PinMode

```cpp
enum class PinMode : uint8_t {
    Input = 0,
    Output = 1,
    InputPullup = 2,
    InputPulldown = 3
};
```

---

### IPlatformComm

**Header:** `include/platform/platform.hpp`

Communication abstraction.

#### Methods

```cpp
virtual core::Result<void> init() noexcept = 0;
virtual core::Result<void> shutdown() noexcept = 0;
virtual core::Result<size_t> send(const uint8_t* data, size_t length) noexcept = 0;
virtual core::Result<size_t> receive(uint8_t* buffer, size_t max_length, uint32_t timeout_ms) noexcept = 0;
virtual bool is_connected() noexcept = 0;
```

---

## Utilities Module

### Platform Detection Macros

**Header:** `include/common/utils/gs_macros.hpp`

Compile-time platform and compiler detection.

#### Platform Macros

| Macro | Value when Active | Description |
|-------|-------------------|-------------|
| `GS_PLATFORM_ARDUINO` | `1` | Compiling for Arduino/AVR |
| `GS_PLATFORM_NATIVE` | `1` | Compiling for PC (Native) |

**Example:**
```cpp
#if GS_PLATFORM_ARDUINO
    // Arduino-specific code
    Serial.println("Hello Arduino!");
#else
    // Native platform code
    std::cout << "Hello PC!" << std::endl;
#endif
```

#### Compiler Macros

| Macro | Value when Active | Description |
|-------|-------------------|-------------|
| `GS_COMPILER_CLANG` | `1` | Clang compiler |
| `GS_COMPILER_GCC` | `1` | GCC compiler |
| `GS_COMPILER_MSVC` | `1` | Microsoft Visual C++ |

---

### Move Semantics

**Header:** `include/common/utils/gs_macros.hpp`

Cross-platform move semantics for C++17.

#### GS_MOVE

```cpp
#define GS_MOVE(x) /* platform-specific move */
```

Moves an object (equivalent to `std::move()` on native, manual implementation on AVR).

**Example:**
```cpp
StaticBuffer<uint8_t, 256> source;
// ... fill source
StaticBuffer<uint8_t, 256> dest = GS_MOVE(source);
```

#### GS_FORWARD

```cpp
#define GS_FORWARD(T, x) /* platform-specific forward */
```

Perfect forwarding (equivalent to `std::forward<T>()`).

---

### Compiler Hints

**Header:** `include/common/utils/gs_macros.hpp`

Optimization hints for the compiler.

| Macro | Description | GCC/Clang | MSVC |
|-------|-------------|-----------|------|
| `GS_LIKELY(x)` | Branch prediction hint (likely true) | `__builtin_expect` | No-op |
| `GS_UNLIKELY(x)` | Branch prediction hint (likely false) | `__builtin_expect` | No-op |
| `GS_INLINE` | Force inline | `always_inline` | `__forceinline` |
| `GS_NOINLINE` | Prevent inlining | `noinline` | `__declspec(noinline)` |
| `GS_PACKED` | Pack struct without padding | `packed` | N/A |
| `GS_ALIGN(n)` | Align to n bytes | `aligned(n)` | `__declspec(align(n))` |

**Example:**
```cpp
if (GS_UNLIKELY(result.is_error())) {
    // Error handling path (rarely taken)
    return result.error();
}
```

---

### C++17 Attributes

**Header:** `include/common/utils/gs_macros.hpp`

| Macro | C++17 Equivalent | Description |
|-------|------------------|-------------|
| `GS_NODISCARD` | `[[nodiscard]]` | Warn if return value is discarded |
| `GS_FALLTHROUGH` | `[[fallthrough]]` | Intentional switch fallthrough |
| `GS_MAYBE_UNUSED` | `[[maybe_unused]]` | Suppress unused variable warnings |
| `GS_CONSTEXPR` | `constexpr` | Compile-time evaluation |

---

### Arduino-Specific Macros

**Header:** `include/common/utils/gs_macros.hpp`

| Macro | Description |
|-------|-------------|
| `GS_PROGMEM` | Store in program memory (flash) |
| `GS_NOINIT` | Do not initialize on startup |

**Note:** These macros are no-ops on native platform.

---

### Assertions

**Header:** `include/common/utils/gs_macros.hpp`

#### GS_ASSERT

```cpp
#define GS_ASSERT(expr) /* platform-specific assert */
```

Runtime assertion (enabled only on native platform with debug builds).

#### GS_STATIC_ASSERT

```cpp
#define GS_STATIC_ASSERT(expr, msg) static_assert(expr, msg)
```

Compile-time assertion with message.

---

### Type Traits (AVR)

**Header:** `include/common/utils/gs_typetraits.hpp`

Minimal type traits for AVR (where `<type_traits>` is unavailable).

#### remove_reference

```cpp
template<typename T>
struct remove_reference;
```

Removes reference from type.

**Example:**
```cpp
remove_reference<int&>::type;   // int
remove_reference<int&&>::type;  // int
remove_reference<int>::type;    // int
```

---

### Utility Functions (AVR)

**Header:** `include/common/utils/gs_utils.hpp`

Minimal utility functions for AVR.

#### move()

```cpp
template<typename T>
typename remove_reference<T>::type&& move(T&& arg);
```

Manual move implementation for AVR (use `GS_MOVE` macro instead for cross-platform code).

**Note:** On native platform, includes `<utility>` and provides `MOVE(x)` as `std::move(x)`.

---

## Error Handling

### Result<T>

**Header:** `include/common/core/error.hpp`

Type-safe error handling monad (no exceptions).

#### Methods

##### is_ok()

```cpp
bool is_ok() const noexcept;
```

Checks if result contains a value.

---

##### is_error()

```cpp
bool is_error() const noexcept;
```

Checks if result contains an error.

---

##### value()

```cpp
T& value() noexcept;
const T& value() const noexcept;
```

Returns the contained value (unsafe - check `is_ok()` first).

---

##### value_or()

```cpp
T value_or(const T& default_val) const noexcept;
```

Returns value or default if error.

---

##### error()

```cpp
ErrorContext error() const noexcept;
```

Returns the error context.

---

### ErrorCode

**Header:** `include/common/core/error.hpp`

Error code enumeration organized by category.

```cpp
enum class ErrorCode : uint16_t {
    Success = 0,
    
    // System errors (100-199)
    SystemNotInitialized = 100,
    SystemAlreadyInitialized = 101,
    SystemShutdown = 102,
    InvalidState = 103,
    ResourceExhausted = 104,
    
    // Hardware errors (200-299)
    HardwareFailure = 200,
    SensorReadFailure = 201,
    SensorNotCalibrated = 202,
    TamperDetected = 203,
    PowerLossDetected = 204,
    
    // Security errors (300-399)
    CryptoFailure = 300,
    AuthenticationFailed = 301,
    IntegrityViolation = 302,
    KeyGenerationFailed = 303,
    SignatureInvalid = 304,
    EncryptionFailed = 305,
    DecryptionFailed = 306,
    
    // Network errors (400-499)
    NetworkTimeout = 400,
    NetworkDisconnected = 401,
    TransmissionFailed = 402,
    InvalidPacket = 403,
    BufferOverflow = 404,
    
    // Analytics errors (500-599)
    AnomalyDetected = 500,
    ProfileMismatch = 501,
    ThresholdExceeded = 502,
    DataInvalid = 503,
    
    // Configuration errors (600-699)
    InvalidParameter = 600,
    ConfigurationError = 601,
    CalibrationRequired = 602,
    
    // Generic errors (900-999)
    Unknown = 900,
    NotImplemented = 901,
    NotSupported = 902
};
```

#### Error Categories

| Range | Category | Description |
|-------|----------|-------------|
| 0 | Success | Operation completed successfully |
| 100-199 | System | Initialization, state, resource errors |
| 200-299 | Hardware | Sensor, tamper, power errors |
| 300-399 | Security | Crypto, auth, integrity errors |
| 400-499 | Network | Timeout, transmission, packet errors |
| 500-599 | Analytics | Anomaly detection errors |
| 600-699 | Configuration | Parameter, calibration errors |
| 900-999 | Generic | Unknown, not implemented errors |

---

### Macros

#### GS_MAKE_ERROR

```cpp
#define GS_MAKE_ERROR(code) \
    ::gridshield::core::ErrorContext((code), __LINE__, __FILE__)
```

Creates an error with source location.

**Example:**
```cpp
return GS_MAKE_ERROR(core::ErrorCode::InvalidParameter);
```

---

#### GS_TRY

```cpp
#define GS_TRY(expr) \
    do { \
        auto gs_result_ = (expr); \
        if (GS_UNLIKELY(gs_result_.is_error())) { \
            return gs_result_.error(); \
        } \
    } while(0)
```

Early return on error.

**Example:**
```cpp
GS_TRY(system.initialize(config, platform));
GS_TRY(system.start());
```

---

#### GS_TRY_ASSIGN

```cpp
#define GS_TRY_ASSIGN(var, expr) \
    do { \
        auto gs_result_ = (expr); \
        if (GS_UNLIKELY(gs_result_.is_error())) { \
            return gs_result_.error(); \
        } \
        var = GS_MOVE(gs_result_.value()); \
    } while(0)
```

Assign value on success, return on error.

**Example:**
```cpp
AnomalyReport report;
GS_TRY_ASSIGN(report, detector.analyze(reading));
```

---

## Type Definitions

### MeterReading

**Header:** `include/common/core/types.hpp`

Meter reading data (24 bytes).

```cpp
struct MeterReading {
    timestamp_t timestamp;    // Timestamp in milliseconds
    uint32_t energy_wh;       // Energy in watt-hours
    uint32_t voltage_mv;      // Voltage in millivolts
    uint16_t current_ma;      // Current in milliamperes
    uint16_t power_factor;    // Power factor (0-1000, scaled by 10)
    uint8_t phase;            // Phase number
};
```

---

### TamperEvent

**Header:** `include/common/core/types.hpp`

Tamper event data (16 bytes).

```cpp
struct TamperEvent {
    timestamp_t timestamp;  // When tamper occurred
    uint32_t metadata;      // Additional metadata
    uint16_t sensor_id;     // Sensor identifier
    uint8_t event_type;     // TamperType
    uint8_t severity;       // Priority level
};
```

---

### StaticBuffer<T, N>

**Header:** `include/common/core/types.hpp`

Fixed-size container (no heap allocation).

#### Methods

```cpp
bool push(const T& item) noexcept;
bool push(T&& item) noexcept;
bool pop(T& item) noexcept;
void clear() noexcept;

size_t size() const noexcept;
size_t capacity() const noexcept;
bool empty() const noexcept;
bool full() const noexcept;

T& operator[](size_t idx) noexcept;
const T& operator[](size_t idx) const noexcept;
```

**Example:**
```cpp
core::StaticBuffer<MeterReading, 100> buffer;
buffer.push(reading);

MeterReading temp;
if (buffer.pop(temp)) {
    process(temp);
}
```

---

### SystemState

**Header:** `include/common/core/types.hpp`

System state enumeration.

```cpp
enum class SystemState : uint8_t {
    Uninitialized = 0,
    Initializing = 1,
    Ready = 2,
    Operating = 3,
    Tampered = 4,
    PowerLoss = 5,
    Error = 6,
    Shutdown = 7
};
```

---

### Priority

**Header:** `include/common/core/types.hpp`

Packet priority enumeration.

```cpp
enum class Priority : uint8_t {
    Lowest = 0,
    Low = 1,
    Normal = 2,
    High = 3,
    Critical = 4,
    Emergency = 5
};
```

---

## Constants

### Cryptographic Constants

**Header:** `include/common/security/crypto.hpp`

```cpp
constexpr size_t ECC_KEY_SIZE = 32;           // 256-bit private key
constexpr size_t ECC_SIGNATURE_SIZE = 64;     // r + s (32 + 32)
constexpr size_t ECC_PUBLIC_KEY_SIZE = 64;    // x + y (32 + 32)
constexpr size_t AES_KEY_SIZE = 32;           // 256-bit AES key
constexpr size_t AES_GCM_TAG_SIZE = 16;       // GCM authentication tag
constexpr size_t SHA256_HASH_SIZE = 32;       // SHA-256 output
```

---

### Protocol Constants

**Header:** `include/common/network/packet.hpp`

```cpp
constexpr uint16_t PROTOCOL_VERSION = 0x0100;
constexpr uint16_t MAX_PAYLOAD_SIZE = 512;
constexpr uint8_t MAGIC_HEADER = 0xA5;
constexpr uint8_t MAGIC_FOOTER = 0x5A;
```

---

## Usage Examples

### Complete System Setup

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"

int main() {
    // 1. Create platform services
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::native::NativeInterrupt interrupt;
    platform::native::NativeCrypto crypto;
    platform::native::NativeComm comm;
    
    platform::PlatformServices services;
    services.time = &time;
    services.gpio = &gpio;
    services.interrupt = &interrupt;
    services.crypto = &crypto;
    services.comm = &comm;
    
    // 2. Configure system
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = 5000;
    
    // 3. Initialize and run
    GridShieldSystem system;
    GS_TRY(system.initialize(config, services));
    GS_TRY(system.start());
    
    // 4. Main loop
    while (true) {
        GS_TRY(system.process_cycle());
        time.delay_ms(100);
    }
    
    return 0;
}
```

---

## Platform Implementations

### Native Platform

**Header:** `include/native/platform_native.hpp`

Implementations for PC testing:
- `NativeTime` - Uses `std::chrono`
- `NativeGPIO` - Software simulation
- `NativeCrypto` - Uses `std::random`
- `NativeComm` - In-memory buffers

### Arduino Platform

**Header:** `include/arduino/platform_arduino.hpp`

Implementations for AVR:
- `ArduinoTime` - Uses `millis()`
- `ArduinoGPIO` - Uses `pinMode()`, `digitalRead()`, `digitalWrite()`
- `ArduinoCrypto` - Uses `random()`, Crypto library for SHA-256
- `ArduinoSerial` - Uses `Serial` interface

---

## See Also

- [Quick Start Guide](QUICKSTART.md) - Get started in 5 minutes
- [Architecture](ARCHITECTURE.md) - System design overview
- [Build Guide](../BUILD.md) - Build instructions

---

**Institut Teknologi PLN - 2025**