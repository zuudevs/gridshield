# GridShield

**Multi-Layer Security Framework for Advanced Metering Infrastructure (AMI)**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![Version](https://img.shields.io/badge/Version-3.0.1-brightgreen)](.)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5-E7352C?logo=espressif)](https://docs.espressif.com/projects/esp-idf/)
[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20QEMU-green)](BUILD.md)
[![CI](https://img.shields.io/badge/CI-GitHub%20Actions-blue?logo=githubactions)](https://github.com/zuudevs/gridshield/actions)

---

## 🛡️ Overview

GridShield is a production-grade security solution designed to protect smart electricity meters from physical tampering, network attacks, and consumption fraud. Built for resource-constrained embedded systems, it implements a **defense-in-depth** strategy across three security layers.

> **Why GridShield?** Traditional meter security relies on single-point defenses. GridShield correlates physical, network, and behavioral signals to detect sophisticated attacks that bypass conventional safeguards.

## ✨ Key Features

### 🔐 Physical Security Layer
- **ISR-driven tamper detection** with debouncing logic
- Power-loss alerting via backup capacitor
- Priority flagging for emergency transmission

### 🌐 Network Security Layer
- **Lightweight ECC (secp256r1)** via micro-ecc
- ECDSA packet signing + SHA256 integrity checks
- Replay protection with sequence numbering

### 📊 Analytics Layer
- Real-time consumption anomaly detection
- Profile-based behavioral analysis (24-hour baseline)
- Cross-layer validation engine

### 🎯 Production Ready
- **Zero heap allocation** design (embedded-friendly)
- Type-safe error handling via `Result<T>` monad (no exceptions)
- Platform abstraction layer (HAL) for portability
- Mock implementations for simulation testing

### 🖥️ Backend & Dashboard
- **FastAPI** REST backend with 9 API endpoints
- SQLite database with SQLAlchemy ORM
- **Vite + Chart.js** real-time web dashboard
- 4 dashboards: Overview, Alerts, Anomalies, Fleet Management

### 🧪 CI/CD & Testing
- **152 unit tests** across 17 test suites
- 6-job GitHub Actions pipeline (build, test, backend-lint, frontend-build, clang-tidy, coverage)
- LibFuzzer + ASan/UBSan fuzzing for packet parser
- Code coverage reports via gcov/lcov
- Hardware tested on ESP32-D0WD rev1.1 (Dual Core 240MHz)

## 🚀 Quick Start

### Prerequisites

- **ESP-IDF v5.5+** — [Installation Guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/)
- **QEMU** (for simulation, optional)

### Build & Run

```powershell
# Clone repository
git clone https://github.com/zuudevs/gridshield.git
cd gridshield

# Build firmware
.\scripts\script.ps1 --build

# Run in QEMU simulator
.\scripts\script.ps1 --run
```

Or manually with ESP-IDF:

```bash
cd firmware
idf.py set-target esp32
idf.py build
idf.py -p COM3 flash         # flash to physical ESP32
idf.py -p COM3 monitor       # serial monitor
idf.py qemu monitor          # or run in QEMU simulator
```

See [BUILD.md](BUILD.md) for full instructions.

## 📦 Target Platform

| Platform | MCU | Flash | RAM | Status |
|----------|-----|-------|-----|--------|
| **ESP32 DevKit V1** | Xtensa LX6 | 4 MB | 520 KB | ✅ Active |
| **QEMU (Simulation)** | Emulated Xtensa | — | — | ✅ Active |

## 📚 Documentation

- [**Build Instructions**](BUILD.md) — Build & simulate with ESP-IDF + QEMU
- [**Architecture**](docs/ARCHITECTURE.md) — System design with diagrams
- [**API Reference**](docs/API.md) — Firmware & backend API documentation
- [**Quick Start Guide**](docs/QUICKSTART.md) — Getting started tutorial
- [**Tech Stack**](docs/TECHSTACK.md) — Technology choices
- [**Roadmap**](docs/ROADMAP.md) — Future development plans
- [**Changelog**](docs/CHANGELOG.md) — Version history

## 🏗️ Project Structure

```
gridshield/
├── firmware/                    # ESP-IDF firmware project
│   ├── CMakeLists.txt           # Root build config
│   ├── include/                 # Header files
│   │   ├── common/              # Platform-agnostic headers
│   │   └── platform/            # HAL interfaces + mock impls
│   ├── main/                    # Implementation files
│   ├── test_app/                # Unity test suites (152 tests)
│   ├── fuzz/                    # LibFuzzer harness
│   ├── coverage/                # gcov/lcov coverage scripts
│   └── lib/micro-ecc/           # ECC library (secp256r1)
├── backend/                     # FastAPI REST backend
│   ├── app/                     # Python application
│   │   ├── main.py              # FastAPI app entry
│   │   ├── routes.py            # API endpoints
│   │   ├── models.py            # SQLAlchemy models
│   │   └── schemas.py           # Pydantic schemas
│   └── requirements.txt         # Python dependencies
├── frontend/                    # Vite + Chart.js dashboard
│   ├── src/
│   │   ├── pages/               # Dashboard, Alerts, Anomalies, Fleet
│   │   ├── components/          # Navbar, Chart components
│   │   └── api.js               # Backend API client
│   └── package.json
├── .github/workflows/           # CI/CD (6-job pipeline)
├── scripts/script.ps1           # Build/run automation
└── docs/                        # Documentation
```

## 🤝 Contributing

We welcome contributions! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for code style guidelines and PR process.

## 🔒 Security

Found a vulnerability? **Do not** open a public issue. See [SECURITY.md](SECURITY.md) for responsible disclosure.

## 📄 License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) file for details.

## 👥 Authors

- **Muhammad Ichwan Fauzi** — Team Leader, Project Manager
- **Rafi Indra Pramudhito Zuhayr** — Firmware Implementation, System Architecture
- **Cesar Ardika Bhayangkara** — Hardware Integration

**Institut Teknologi PLN** — 2026

## 🌟 Acknowledgments

- Inspired by NIST SP 800-53 security controls
- Built with lessons from IoT security research
- Cryptography: micro-ecc (secp256r1)

---

**⭐ Star this repo if GridShield helps your project!**