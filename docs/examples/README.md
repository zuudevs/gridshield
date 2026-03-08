# API Examples

Complete usage examples for all GridShield public modules. Each file demonstrates
real-world usage patterns with proper error handling via `Result<T>`.

> **Note:** These are documentation-only examples showing API usage patterns.
> They are not compiled as part of the firmware build.

## Examples

| File | Module | What It Shows |
|------|--------|---------------|
| [example_system.cpp](example_system.cpp) | Core | System lifecycle: init → start → process → stop → shutdown |
| [example_crypto.cpp](example_crypto.cpp) | Security | Keypair generation, ECDSA sign/verify, AES-GCM encrypt/decrypt, SHA-256 |
| [example_packet.cpp](example_packet.cpp) | Network | Build, serialize, parse, and transport secure packets |
| [example_tamper.cpp](example_tamper.cpp) | Hardware | Tamper detector setup, ISR flow, polling, and event handling |
| [example_detector.cpp](example_detector.cpp) | Analytics | Anomaly detection, profile learning, threat analysis |
| [example_hkdf.cpp](example_hkdf.cpp) | Security | HKDF-SHA256 extract and expand for key derivation |

## Common Patterns

### Error Handling

All GridShield APIs return `core::Result<T>` (never throw exceptions):

```cpp
auto result = some_api_call();
if (result.is_error()) {
    // Handle error: result.error()
    return result.error();
}
// Use result: result.value()
```

### GS_TRY Macro

For early-return on error (propagate errors up):

```cpp
core::Result<void> my_function() {
    GS_TRY(step_one());    // Returns error if step_one fails
    GS_TRY(step_two());    // Only runs if step_one succeeded
    return core::Result<void>{};
}
```

### Platform Services

All modules depend on `PlatformServices` for hardware abstraction:

```cpp
platform::mock::MockTime mock_time;
platform::mock::MockGPIO mock_gpio;
platform::mock::MockCrypto mock_crypto;
platform::mock::MockComm mock_comm;
platform::mock::MockInterrupt mock_interrupt;
platform::mock::MockStorage mock_storage;

platform::PlatformServices services;
services.time = &mock_time;
services.gpio = &mock_gpio;
services.crypto = &mock_crypto;
services.comm = &mock_comm;
services.interrupt = &mock_interrupt;
services.storage = &mock_storage;
```
