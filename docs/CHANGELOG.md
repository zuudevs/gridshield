# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-02-23

### Changed
- **Build System Migration**:
  - Migrated from CMake/PlatformIO to **ESP-IDF v5.5** as the sole build system.
  - Build command is now `idf.py build` (replaces `cmake --preset`).
  - ESP32 is now the primary (and only) target MCU.
- **Simulation Migration**:
  - Replaced Renode simulator with **QEMU** (ESP32 emulation via ESP-IDF).
  - Simulation command: `idf.py qemu monitor`.
  - Added GDB debugging support via `idf.py qemu --gdb`.
- **Project Structure Cleanup**:
  - Merged `firmware_/renode/` into `firmware/` — single firmware directory.
  - Renamed `qemu_main.cpp` → `app_main.cpp` (ESP-IDF convention).
  - Flattened `main/common/` + `main/platform/` into `main/src/`.
  - Trimmed `lib/micro-ecc/` — removed tests, scripts, examples.
- **Entry Point**:
  - `app_main()` now uses FreeRTOS `vTaskDelay()` instead of busy-wait loop.
  - Added `GS_QEMU_BUILD=1` compile definition for QEMU-specific code paths.
- **Documentation**:
  - Updated all docs (`README.md`, `BUILD.md`, `QUICKSTART.md`, `ARCHITECTURE.md`, `TECHSTACK.md`) to reflect ESP-IDF + QEMU workflow.

### Added
- **Automation Script** (`scripts/script.ps1`):
  - `--build` — Build firmware via ESP-IDF.
  - `--run` — Run in QEMU with IDF Monitor.
  - `--run-raw` — Run in QEMU raw console.
  - `--debug` — Run QEMU with GDB server.
  - `--gdb` — Attach GDB to running QEMU.
  - `--setup` — Install QEMU via `idf_tools.py`.
  - `--clean` — Full clean build artifacts.
  - `--env` — Open shell with ESP-IDF environment.

### Removed
- Arduino/AVR support (CMake presets, platform_arduino.hpp, Arduino CLI).
- PlatformIO build system and configurations.
- Wokwi simulator integration.
- Renode simulator and `renode_main.cpp`.
- Native PC build target (`main.cpp`, `platform_native.hpp`).
- `arduino.hpp` compatibility shim.

## [Unreleased]

### Added
- **Documentation**:
  - Added `KeyStorage` class API documentation to `docs/API.md`.
  - Added `Utilities Module` section to `docs/API.md` (macros, type traits, compiler hints).
  - Added complete `ErrorCode` enumeration with category descriptions.
  - Created `examples/README.md` with usage guides for SimpleMeter and demo_native examples.

### Changed
- **Documentation Translations**:
  - Translated `docs/PROPOSAL.md` from Indonesian to English.
  - Translated `docs/requirements.md` from Indonesian to English.
  - Translated `docs/MUST_READ/workflow.md` from Indonesian to English with GridShield-specific workflow.

### Fixed
- **Documentation Consistency**:
  - Fixed incorrect `gridshield.ino` filename references to `main.ino` across all documentation.
  - Updated file references in `BUILD.md`, `QUICKSTART.md`, and `ARCHITECTURE.md`.

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
- **[CRITICAL] ECDSA Signature Buffer Overflow** (`crypto.cpp`):
  OpenSSL `ECDSA_sign()` produces DER-encoded signatures (~72 bytes) but
  the output buffer was only 64 bytes. Replaced with `ECDSA_do_sign()` /
  `ECDSA_do_verify()` using raw BIGNUM r,s conversion to/from fixed 64-byte
  (r || s) format.
- **[CRITICAL] ISR Blocking Delay** (`tamper.cpp`, `tamper.hpp`):
  `delay_ms()` was called inside the interrupt handler, causing hang.
  Refactored to deferred debounce: ISR now only sets `pending_tamper_`
  flag, actual debounce confirmation done via `poll()` in `process_cycle()`.
- **Wrong Macro Names** (`mock_platform.hpp`):
  Replaced `PLATFORM_NATIVE` → `GS_PLATFORM_NATIVE` and
  `MAKE_ERROR` → `GS_MAKE_ERROR` to match project convention.
- **StaticBuffer Alignment** (`types.hpp`):
  Added `alignas(T)` to raw storage array in `StaticBuffer<T, N>` to
  prevent undefined behavior with aligned types.

### Added
- **StaticBuffer::pop_front()** (`types.hpp`):
  Added FIFO removal method. `AnomalyDetector::update_profile()` now
  correctly drops the oldest reading.

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

[2.0.0]: https://github.com/zuudevs/gridshield/compare/v1.1.0...v2.0.0
[1.1.0]: https://github.com/zuudevs/gridshield/compare/v1.0.1...v1.1.0
[1.0.1]: https://github.com/zuudevs/gridshield/compare/v0.1.0...v1.0.1
[0.1.0]: https://github.com/zuudevs/gridshield/releases/tag/v0.1.0