# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Planned
- Implementation of Elliptic Curve Cryptography (ECC) on ESP32.
- Integration of physical tamper sensors (Hall/Limit Switch).
- Backend anomaly detection engine (Python/Go).

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