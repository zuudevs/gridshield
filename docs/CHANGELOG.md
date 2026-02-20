# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Implementation of Elliptic Curve Cryptography (ECC) on ESP32.
- Integration of physical tamper sensors (Hall/Limit Switch).
- Backend anomaly detection engine (Python/Go).

## [1.1.0] - 2026-02-20

### Changed
- **Embedded Crypto Standardization**:
  - Removed **OpenSSL** dependency from Native build.
  - Native platform now uses the **same** `rweather/Crypto` and `micro-ecc` libraries as Arduino.
  - Ensured cryptographic consistency (1:1 behavior) between Native verification and Production-ready code.
- **Build System**:
  - Updated `CMakeLists.txt` to link against PlatformIO managed libraries.
  - Added `platformio lib install` requirement for Native builds.
  - Added `ctest` support with `gridshield_test` target.

### Fixed
- **AES-GCM Implementation**: Refactored `crypto.cpp` to use `GCM<AES256>` from `rweather/Crypto` on all platforms.
- **Documentation**: Updated `BUILD.md` and `README.md` to reflect new build process and file structure.

## [1.0.1] - 2026-02-20

### Fixed
- **[CRITICAL] ECDSA Signature Buffer Overflow** (`src/common/security/crypto.cpp`):
  OpenSSL `ECDSA_sign()` produces DER-encoded signatures (~72 bytes) but
  the output buffer was only 64 bytes. Replaced with `ECDSA_do_sign()` /
  `ECDSA_do_verify()` using raw BIGNUM r,s conversion to/from fixed 64-byte
  (r || s) format.
- **[CRITICAL] ISR Blocking Delay** (`src/common/hardware/tamper.cpp`,
  `include/common/hardware/tamper.hpp`):
  `delay_ms()` was called inside the interrupt handler, causing Arduino to
  hang indefinitely. Refactored to deferred debounce: ISR now only sets a
  `pending_tamper_` flag, actual debounce confirmation is done via new
  `poll()` method called from `process_cycle()`.
- **Wrong Macro Names** (`include/platform/mock_platform.hpp`):
  Replaced `PLATFORM_NATIVE` → `GS_PLATFORM_NATIVE` (8 locations) and
  `MAKE_ERROR` → `GS_MAKE_ERROR` (8 locations) to match project convention.
- **StaticBuffer Alignment** (`include/common/core/types.hpp`):
  Added `alignas(T)` to raw storage array in `StaticBuffer<T, N>` to
  prevent undefined behavior with aligned types like `MeterReading`.

### Added
- **Arduino Interrupt Implementation** (`include/arduino/platform_arduino.hpp`):
  Replaced no-op `ArduinoInterrupt` with full implementation using static
  trampoline ISRs for Arduino Mega 2560 (6 interrupt-capable pins). Bridges
  Arduino's `void(*)()` ISR to the HAL's `void(*)(void*)` callback pattern.
- **StaticBuffer::pop_front()** (`include/common/core/types.hpp`):
  Added FIFO removal method. `AnomalyDetector::update_profile()` now
  correctly drops the oldest reading instead of the newest when the
  buffer is full.

## [0.1.0] - 2026-02-09

### Added
- Initial project structure and repository setup.
- **Documentation**:
  - Added `README.md`, `LICENSE`, `SECURITY.md`.
  - Added `CONTRIBUTING.md` with comprehensive development guidelines.
  - Added `CODE_OF_CONDUCT.md`.
  - Added `BUILD.md` for build instructions.
  - Added `docs/ARCHITECTURE.md` for high-level design.
  - Added `docs/PROPOSAL.md` and `docs/requirements.md`.
- **Build System**:
  - CMake build configuration with presets for Native and AVR.
  - Directory structure for `src`, `include`, and `tests`.
- **Standards**:
  - Defined C++17 coding standards.
  - Established error handling patterns using `Result<T>`.

[Unreleased]: https://github.com/yourusername/gridshield/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yourusername/gridshield/releases/tag/v0.1.0