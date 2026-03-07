# GridShield - Project Roadmap

Future development plans and feature roadmap for GridShield AMI Security System.

**Last Updated:** March 2026  
**Current Version:** 3.1.0-fw  
**Next Target:** 3.2.0 (Q2 2027)

---

## Table of Contents

- [Vision](#vision)
- [Current Status](#current-status)
- [Roadmap Timeline](#roadmap-timeline)
- [Feature Backlogs](#feature-backlogs)
- [Technical Debt](#technical-debt)
- [Long-Term Goals](#long-term-goals)

---

## Vision

GridShield aims to become the **industry-standard open-source security framework** for Advanced Metering Infrastructure (AMI) systems, providing:

- **Production-Grade Cryptography** — Full integration with hardware security modules
- **Machine Learning** — Advanced anomaly detection using ML models
- **Cloud Integration** — Seamless connection to cloud analytics platforms
- **Regulatory Compliance** — Meet international smart meter security standards

---

## Current Status

### ✅ Completed (v2.0.0)

- [x] Multi-layer security architecture (Physical, Network, Analytics)
- [x] C++17 codebase with zero heap allocation
- [x] C++17 nested namespaces + NSDMI (codebase-wide)
- [x] Platform abstraction layer (HAL) with mock implementations
- [x] Result-based error handling (no exceptions)
- [x] Tamper detection with ISR + deferred debounce
- [x] Production crypto: ECDSA (micro-ecc), SHA-256
- [x] Secure packet protocol with signature verification
- [x] Statistical anomaly detection + profile learning
- [x] Cross-layer threat correlation
- [x] **ESP-IDF v5.5 build system** (replaces CMake/PlatformIO)
- [x] **ESP32 target platform** (Xtensa LX6)
- [x] **QEMU simulation** (replaces Renode/Wokwi)
- [x] GDB debugging support via QEMU
- [x] Automation script (`scripts/script.ps1`)
- [x] Comprehensive documentation (7 docs updated for v2.0.0)

### 🚧 In Progress

- [x] Unit test suite (Unity for ESP-IDF) — 152 tests, 17 suites
- [x] CI/CD pipeline (GitHub Actions) — 6-job workflow
- [x] Backend integration (Python + FastAPI) — 8 API endpoints
- [x] Hardware testing with physical ESP32 (ESP32-D0WD rev1.1, Dual Core 240MHz)

---

## Roadmap Timeline

### Q2 2026 — Security Hardening & Testing (v2.1.0)

**Target Release:** June 2026

#### High Priority

- [x] **Secure Key Storage** (v2.1.0)
  - [x] NVS (Non-Volatile Storage) encrypted key store
  - [x] Key rotation mechanism
  - [x] Backup key management

- [x] **Security Enhancements** (v2.1.0)
  - [x] Hardware RNG integration (ESP32 TRNG)
  - [x] Secure key derivation (HKDF)
  - [x] AES-256-GCM via mbedTLS
  - [x] ESP32 Secure Boot v2 (documentation + config)
  - [x] Flash encryption (documentation + config)
  - [x] Watchdog timer integration

- [x] **Testing Infrastructure**
  - [x] Unit tests with Unity (ESP-IDF) — 104 tests, 14 suites
  - [x] Integration tests via QEMU
  - [x] Fuzzing for packet parser (LibFuzzer + ASan/UBSan)
  - [x] Code coverage reports (≥45% native) — gcov/lcov + CI job

#### Medium Priority

- [x] **CI/CD Pipeline** (v2.1.0)
  - [x] GitHub Actions: build on push
  - [x] Automated QEMU test runs
  - [x] Lint + clang-tidy checks
  - [x] Release artifact packaging

- [x] **Documentation**
  - [x] Security audit report
  - [x] Threat model documentation
  - [x] API examples for all modules (6 examples in docs/examples/)

---

### Q3 2026 — Communication & Sensors (v2.2.0)

**Target Release:** September 2026

#### High Priority

- [x] **Communication Protocols**
  - [x] WiFi HTTP/MQTT (ESP32 built-in)
  - [x] MQTT over TLS
  - [x] LoRa/LoRaWAN driver (SX1276/SX1278)
  - [x] Modbus RTU/TCP
  - [x] CoAP protocol support

- [x] **Real Sensor Integration**
  - [x] ACS712 current sensor driver
  - [x] ZMPT101B voltage sensor driver
  - [x] PZEM-004T energy meter module
  - [x] DS18B20 temperature sensor
  - [x] MPU6050 accelerometer (shock detection)
  - [x] SensorManager aggregator with MeterReading conversion

#### Medium Priority

- [x] **OTA Updates**
  - [x] Over-the-air firmware update (IOtaManager interface)
  - [x] Signed firmware images (ECDSA P-256 + SHA-256)
  - [x] Rollback protection
  - [ ] Delta update support

- [x] **Power Management**
  - [x] ESP32 deep sleep modes
  - [x] Wake-on-tamper (GPIO)
  - [x] Adaptive duty cycling (mains/battery/solar)
  - [ ] Power consumption profiling

- [x] **System Integration**
  - [x] SensorManager, OtaManager, PowerManager in GridShieldSystem
  - [x] 47 new unit tests (3 suites), all passing
  - [x] Native test runner with clang++ (Windows)

---

### Q4 2026 — Cloud & Analytics (v3.0.0)

**Target Release:** December 2026

#### High Priority

- [x] **Cloud Integration**
  - [x] AWS IoT Core connector
  - [x] Azure IoT Hub connector
  - [x] MQTT-TLS with certificate management
  - [x] Device provisioning service

- [x] **Advanced Analytics**
  - [x] Machine Learning anomaly detection
  - [x] TensorFlow Lite Micro integration
  - [x] Time-series forecasting
  - [x] Edge AI inference on ESP32

- [x] **Dashboard & Monitoring**
  - [x] Web-based real-time dashboard
  - [x] Alert management system
  - [x] Historical data visualization
  - [x] Fleet management console

#### Medium Priority

- [x] **Protocol Enhancements**
  - [x] Multi-hop mesh networking (ESP-NOW)
  - [x] Time synchronization (SNTP)
  - [x] Remote configuration
  - [x] Certificate management (X.509)

- [x] **Performance Optimization**
  - [x] ESP32 hardware crypto acceleration (AES, SHA)
  - [x] Memory pool allocator
  - [x] Zero-copy packet processing
  - [x] Profiling and benchmarking tools — 55 native tests passing

---

### Q1 2027 — Fleet Management & Forensics (v3.1.0)

**Target Release:** March 2027 (completed March 2026)

#### High Priority

- [x] **Fleet Management API**
  - [x] Meter registration & CRUD endpoints
  - [x] Fleet statistics & aggregation
  - [x] Meter status tracking (online/offline/tampered)
  - [x] Meter last-seen timestamp auto-update

- [x] **Server-Side Anomaly Detection**
  - [x] Automatic anomaly detection on reading ingestion
  - [x] Configurable deviation thresholds (60%/80%)
  - [x] Severity classification (low/medium/high/critical)
  - [x] Anomaly type classification (spike/drop/zero-consumption)

- [x] **Forensics Module** (Firmware)
  - [x] Security event logger with circular buffer (64 events)
  - [x] Event timeline retrieval and filtering
  - [x] Cross-layer incident report generation
  - [x] Attack classification (physical/network/fraud/hybrid)
  - [x] Confidence scoring for coordinated attacks

#### Medium Priority

- [x] **Backend Testing**
  - [x] pytest + httpx test infrastructure
  - [x] Meter CRUD tests (12 tests)
  - [x] Reading API tests (8 tests)
  - [x] Alert API tests (6 tests)
  - [x] Anomaly engine tests (5 tests)

- [x] **Firmware Tests**
  - [x] EventLogger tests (10 tests)
  - [x] IncidentReportGenerator tests (6 tests)

---

## Feature Backlogs

### Core Features

#### Security Enhancements

- [ ] **Cryptographic Agility**
  - [ ] Support multiple ECC curves (P-256, P-384, Curve25519)
  - [ ] Post-quantum cryptography (Kyber, Dilithium)
  - [ ] Hash algorithm selection (SHA-256, SHA-3, BLAKE2)
  - [ ] Cipher suite negotiation

- [ ] **Secure Communication**
  - [ ] TLS 1.3 support via mbedTLS (ESP-IDF built-in)
  - [ ] DTLS for UDP
  - [ ] Certificate pinning
  - [ ] Perfect forward secrecy (PFS)
  - [ ] Mutual TLS (mTLS)

- [ ] **Key Management**
  - [ ] Hierarchical key derivation
  - [ ] Key escrow and recovery
  - [ ] Hardware Security Module (HSM) integration
  - [ ] Key lifecycle management

#### Hardware Features

- [ ] **Extended Tamper Detection**
  - [ ] Capacitive touch detection
  - [ ] Light sensor (casing removal)
  - [ ] Gyroscope (orientation change)
  - [ ] Pressure sensor (enclosure integrity)

- [ ] **Sensor Fusion**
  - [ ] Multi-sensor correlation
  - [ ] Kalman filtering
  - [ ] Sensor calibration
  - [ ] Fault detection and isolation (FDI)

#### Analytics Features

- [ ] **Advanced Anomaly Detection**
  - [ ] Isolation Forest algorithm
  - [ ] One-Class SVM
  - [ ] Autoencoder-based detection
  - [ ] Adaptive thresholds

- [ ] **Behavioral Analysis**
  - [ ] User consumption patterns
  - [ ] Seasonal decomposition
  - [ ] Appliance load disaggregation
  - [ ] Predictive maintenance

- [x] **Forensics**
  - [x] Attack signature event logger (SecurityEvent + EventLogger)
  - [x] Incident timeline reconstruction (get_timeline)
  - [ ] Evidence preservation
  - [x] Automated reporting (IncidentReportGenerator)

---

### Developer Experience

#### Tooling

- [ ] **Development Tools**
  - [ ] VS Code + ESP-IDF extension setup guide
  - [x] GDB debugging via QEMU
  - [x] Automation script (`script.ps1`)
  - [x] Code formatter (`.clang-format` + `.clang-tidy`)

- [ ] **Build & Deploy**
  - [ ] GitHub Actions workflows
  - [ ] Automated QEMU testing in CI
  - [ ] Artifact registry
  - [ ] Release automation
  - [ ] Semantic versioning

- [ ] **Quality Assurance**
  - [ ] Static analysis (clang-tidy rules)
  - [ ] Dynamic analysis (ASan via QEMU)
  - [ ] Mutation testing
  - [ ] Security scanning (SAST)

#### Documentation

- [ ] **Enhanced Documentation**
  - [ ] Migration guides (v1.x → v2.x)
  - [ ] Best practices guide
  - [ ] Security hardening checklist
  - [ ] Example projects

- [ ] **Community Resources**
  - [ ] Video tutorial series
  - [ ] Blog post series
  - [ ] Case studies
  - [ ] FAQ and troubleshooting

---

### Platform Integrations

#### Additional MCU Support (Future)

- [ ] STM32F4/L5 (ARM Cortex-M4, TrustZone)
- [ ] Nordic nRF52/nRF53 (BLE)
- [ ] TI CC1352 (Zigbee/Thread)
- [ ] Raspberry Pi Pico (RP2040)

#### Cloud Services

- [ ] **IoT Platforms**: ThingSpeak, Blynk, Ubidots
- [ ] **Data Pipelines**: Apache Kafka, InfluxDB, Prometheus, Grafana

---

## Technical Debt

### High Priority

- [x] ~~Replace placeholder crypto with production libraries~~ *(done v1.1.0)*
- [x] ~~Add comprehensive error logging (ESP_LOGx)~~ *(done v2.0.1)*
- [x] ~~Implement retry logic for network failures~~ *(done v2.0.1)*
- [x] ~~Add configuration validation in `app_main.cpp`~~ *(done v2.0.1)*
- [x] ~~Remove unused `GS_RENODE_BUILD` guards (migration leftover)~~ *(done v2.0.1)*

### Medium Priority

- [x] ~~Optimize packet serialization (reduce copies)~~ *(done v2.0.2)*
- [x] ~~Add runtime configuration via NVS~~ *(done v2.0.1)*
- [x] ~~Implement graceful degradation~~ *(done v2.0.2)*
- [x] ~~Add telemetry and diagnostics~~ *(done v2.0.2)*

### Low Priority

- [ ] Improve code comments and inline documentation
- [ ] Reduce header dependencies
- [ ] Add static analysis annotations
- [ ] Create architecture decision records (ADRs)

---

## Long-Term Goals (2027+)

### Research & Innovation

- [ ] **Quantum-Resistant Security**
  - [ ] NIST post-quantum algorithms
  - [ ] Hybrid classical/quantum schemes

- [ ] **AI/ML Enhancements**
  - [ ] Federated learning for anomaly detection
  - [ ] Transfer learning for new deployments
  - [ ] Explainable AI (XAI) for audit trails

### Ecosystem Development

- [ ] **Standardization**
  - [ ] Publish security protocol specification
  - [ ] Industry consortium participation
  - [ ] Reference implementation certification

- [ ] **Partnerships**
  - [ ] Utility company pilot programs
  - [ ] Hardware manufacturer collaborations
  - [ ] Academic research partnerships

---

## Contributing

We welcome contributions to the roadmap! Please:

1. **Review existing items** — Check if your idea is already listed
2. **Open a discussion** — Propose new features or changes
3. **Submit PRs** — Implement items from the backlog
4. **Provide feedback** — Help prioritize features

### Priority Criteria

Features are prioritized based on:

1. **Security Impact** — Does it improve security posture?
2. **User Value** — How many users benefit?
3. **Effort** — Implementation complexity and time
4. **Dependencies** — Prerequisite features or blockers
5. **Compliance** — Regulatory or standard requirements

---

## Version History

| Version | Release Date | Key Features |
|---------|-------------|--------------|
| **0.1.0** | February 2026 | Initial structure, documentation |
| **1.0.0** | February 2026 | Multi-layer security, CMake build |
| **1.0.1** | February 2026 | Critical bug fixes (ECDSA, ISR) |
| **1.1.0** | February 2026 | Production crypto (micro-ecc, Arduino Crypto) |
| **2.0.0** | February 2026 | ESP-IDF + QEMU migration, project restructure |
| **2.0.1** | February 2026 | C++17 modernization, ESP_LOG, config validation, 60→90 tests |
| **2.0.2** | February 2026 | Graceful degradation, telemetry, security docs, release pipeline, 104 tests |
| **2.1.0** | February 2026 | Security hardening, testing, CI/CD, 104 tests |
| **2.2.0** | September 2026 | Communication protocols, sensors, 151 tests |
| **3.0.0** | December 2026 | Cloud integration, ML analytics, 206 tests (total) |
| **3.0.1** | March 2026 | CI fixes, hardware testing (ESP32-D0WD), HKDF test fix, 152 tests |
| **3.1.0** | March 2026 | Fleet management API, server-side anomaly detection, forensics module, backend tests (31), firmware tests (+16 = 168 total) |

---

## Feedback & Suggestions

Have ideas for the roadmap? We'd love to hear from you!

- **GitHub Discussions:** Share your thoughts
- **Feature Requests:** Open an issue with tag `enhancement`
- **Email:** zuudevs@gmail.com

---

## License

This roadmap is part of the GridShield project (MIT License).

**Institut Teknologi PLN — 2026**