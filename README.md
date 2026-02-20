# GridShield

**Multi-Layer Security Framework for Advanced Metering Infrastructure (AMI)**

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE)
[![C++17](https://img.shields.io/badge/C%2B%2B-17-00599C?logo=cplusplus)](https://en.cppreference.com/w/cpp/17)
[![CMake](https://img.shields.io/badge/CMake-3.20%2B-064F8C?logo=cmake)](https://cmake.org)
[![Platform](https://img.shields.io/badge/Platform-Native%20%7C%20Arduino-green)](BUILD.md)

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
- **Lightweight ECC (secp256r1)** optimized for 8KB RAM
- ECDSA packet signing + SHA256 integrity checks
- Replay protection with sequence numbering

### 📊 Analytics Layer
- Real-time consumption anomaly detection
- Profile-based behavioral analysis (24-hour baseline)
- Cross-layer validation engine

### 🎯 Production Ready
- **Zero heap allocation** design (embedded-friendly)
- Type-safe error handling (no exceptions)
- Platform abstraction layer (HAL) for portability
- Extensive test coverage with mock implementations

## 🚀 Quick Start

### Installation

```bash
# Clone repository
git clone https://github.com/yourusername/gridshield.git
cd gridshield

# Native build (PC testing)
cmake --preset native-debug
cmake --build --preset native-debug
./bin/NATIVE/GridShield
```

### Basic Usage

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"

using namespace gridshield;

int main() {
    // Setup platform services
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::PlatformServices services{&time, &gpio, ...};
    
    // Configure system
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.tamper_config.sensor_pin = 2;
    
    // Initialize GridShield
    GridShieldSystem system;
    system.initialize(config, services);
    system.start();
    
    // Main processing loop
    while (true) {
        system.process_cycle();
        delay(100);
    }
}
```

## 📦 Target Platforms

| Platform | MCU | Flash | RAM | Status |
|----------|-----|-------|-----|--------|
| **Arduino Mega** | ATmega2560 | 256 KB | 8 KB | ✅ Recommended |
| **Arduino Uno** | ATmega328P | 32 KB | 2 KB | ⚠️ Too small |
| **ESP32** | Xtensa | 4 MB | 520 KB | 🔜 Planned |
| **Native (PC)** | x86/x64 | - | - | ✅ Testing |

## 📚 Documentation

- [**Quick Start Guide**](docs/QUICKSTART.md) - 5-minute tutorial
- [**Build Instructions**](BUILD.md) - Compilation for all platforms
- [**Architecture**](docs/ARCHITECTURE.md) - System design with diagrams
- [**API Reference**](docs/API.md) - Function documentation
- [**Roadmap**](docs/ROADMAP.md) - Planned features

## 🏗️ Project Structure

```
gridshield/
├── include/
│   ├── common/          # Platform-agnostic code
│   │   ├── core/        # Result<T>, types, system orchestrator
│   │   ├── security/    # Crypto engine (ECC, AES-GCM)
│   │   ├── hardware/    # Tamper detector
│   │   ├── network/     # Secure packet protocol
│   │   └── analytics/   # Anomaly detection
│   ├── platform/        # HAL interfaces
│   ├── native/          # PC mock implementation
│   └── arduino/         # AVR drivers
├── src/
│   ├── common/          # Core implementations
│   ├── native/main.cpp         # PC entry point
│   └── arduino/main.ino        # Arduino entry point
└── docs/                # Documentation
```

## 🤝 Contributing

We welcome contributions! Please read [CONTRIBUTING.md](CONTRIBUTING.md) for:
- Code style guidelines (C++17 best practices)
- Pull request process
- Testing requirements

## 🔒 Security

Found a vulnerability? **Do not** open a public issue. See [SECURITY.md](SECURITY.md) for responsible disclosure.

## 📄 License

This project is licensed under the **MIT License** - see [LICENSE](LICENSE) file for details.

## 👥 Authors

- **Muhammad Ichwan Fauzi** - team Leader, Project Manager
- **Rafi Indra Pramudhito Zuhayr** - Firmware Implementation, System Architecture
- **Cesar Ardika Bhayangkara** - Hardware Integration

**Institut Teknologi PLN** - 2025

## 🌟 Acknowledgments

- Inspired by NIST SP 800-53 security controls
- Built with lessons from IoT security research
- Cryptography references: uECC, mbedTLS

---

**⭐ Star this repo if GridShield helps your project!**