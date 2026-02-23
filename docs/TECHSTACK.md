# GridShield Technology Stack

**Version:** 3.0.0  
**Last Updated:** February 2026  
**Maintained By:** GridShield Development Team

## Overview

GridShield utilizes a carefully selected technology stack optimized for embedded systems, security, and scalability. This document is scoped to the **POC (Proof of Concept)** — technologies actively used by a 3-person team — with future/scale-up items clearly separated.

---

## POC Scope Summary

| Layer | Technology | PIC |
|---|---|---|
| **Firmware** | C++17, ESP-IDF v5.5, ESP32 | Rafi |
| **Simulation** | QEMU (ESP32 emulator) | All |
| **Crypto** | micro-ecc (ECDSA secp256r1) | Rafi |
| **Backend** | Python 3.11+, FastAPI, SQLite | Ichwan |
| **Frontend** | Simple web dashboard | Ichwan |
| **Hardware** | ESP32 DevKit V1, sensors | Cesar |

---

## Embedded Firmware Layer

### Primary Language

**Modern C++ (C++17)**
- **Rationale**: Memory efficiency, hardware-level control, zero-cost abstractions
- **Features Used**:
  - `constexpr` for compile-time computations
  - Template metaprogramming for generic algorithms
  - Range-based loops for readability
- **Compiler**: GCC (Xtensa toolchain via ESP-IDF)
- **Standards**: C++17
- **Constraints**: No exceptions, RTTI, or dynamic allocation in critical paths

### Development Environment

**ESP-IDF (Espressif IoT Development Framework)**
- **Version**: v5.5.3
- **Purpose**: Official development framework for ESP32 (build system + SDK)
- **Build Command**: `idf.py build`
- **Benefits**:
  - Full ESP32 hardware access (GPIO, WiFi, Crypto HW, etc.)
  - FreeRTOS integration
  - Menuconfig for hardware configuration
  - QEMU integration for simulation

**QEMU Simulator**
- **Purpose**: Simulated ESP32 tanpa hardware fisik
- **Features**:
  - Full Xtensa CPU emulation
  - GPIO simulation (tamper sensor input)
  - Serial console output
  - GDB debugging support
- **Integration**: Via `idf.py qemu monitor`
- **Installation**: `idf_tools.py install qemu-xtensa`

### Target Hardware Platform

| Platform | MCU | Clock | RAM | Flash | Crypto HW | WiFi | Status |
|---|---|---|---|---|---|---|---|
| **ESP32 DevKit V1** | Xtensa LX6 | 240 MHz | 520 KB | 4 MB | ✅ SHA/AES | ✅ Built-in | ✅ **Active** |
| **QEMU (Emulated)** | Xtensa LX6 | Emulated | Emulated | Emulated | ❌ | ❌ | ✅ **Active** |
| STM32F4 | ARM Cortex-M4 | 168 MHz | 192 KB | 1 MB | ✅ Partial | ❌ | 🔄 Future |

### Libraries & Dependencies

**Cryptography (PIC: Rafi)**
- **micro-ecc**: Lightweight ECC implementation (ECDSA secp256r1)

**Communication (PIC: Rafi)**
- **Serial (UART)**: Debug output + data transmission ke backend (POC)
- **WiFiClient**: HTTP communication ke backend (optional untuk POC)

**Sensors (PIC: Cesar)**
- **Custom HAL**: Hardware abstraction for tamper sensors
- **GPIO/Interrupt**: Built-in ESP32 API via ESP-IDF

---

## Backend Services Layer

### Primary Language (PIC: Ichwan)

**Python 3.11+**
- **Use Cases**:
  - Anomaly detection algorithms (Layer 3)
  - REST API for receiving meter data
  - Data storage and retrieval
- **Key Libraries**:
  - `fastapi` 0.100+: REST API framework
  - `paho-mqtt`: MQTT client (optional)
  - `sqlalchemy` 2.0+: Database ORM
  - `numpy` 1.24+: Numerical computing (anomaly stats)
  - `uvicorn`: ASGI server

### Database (PIC: Ichwan)

**SQLite 3.40+**
- **File**: `gridshield.db`
- **Purpose**: Local data storage, historical records
- **Benefits**: Zero-configuration, embedded, serverless
- **Schema**: Time-series optimized tables

> [!NOTE]
> PostgreSQL, Redis, dan database enterprise lainnya tidak dibutuhkan untuk POC. SQLite cukup untuk demo dengan puluhan meter.

### Communication Protocol (POC)

**Serial (UART) — Primary**
- ESP32 mengirim data via Serial ke PC
- Python backend membaca dari serial port
- Paling simple untuk POC demo

**HTTP REST — Alternative**
- ESP32 mengirim via WiFi HTTP POST ke FastAPI
- Lebih realistis untuk demo production-like

---

## Frontend & Visualization (PIC: Ichwan)

### Dashboard (POC Scope)

**Simple Web Interface**
- **HTML + JavaScript** (vanilla, tanpa framework berat)
- **Chart.js**: Data visualization (consumption graph)
- **WebSocket** atau polling: Live meter status updates

**Features:**
- Status meter (Green/Red)
- Real-time consumption graph
- Tamper alert notifications
- Anomaly detection log

> [!NOTE]
> React, Vue, TailwindCSS, Grafana — semua ini tidak diperlukan untuk POC. Dashboard sederhana cukup.

---

## Development Tools

### Version Control
- **Git**: Source code management
- **GitHub**: Repository hosting

### Build & Simulation (PIC: Rafi)

**ESP-IDF CLI (`idf.py`)**
```
Build Commands:
  idf.py build            # Build firmware
  idf.py fullclean        # Clean build artifacts
  idf.py set-target esp32 # Set build target

QEMU Commands:
  idf.py qemu monitor     # Run in QEMU with colored output
  idf.py qemu --gdb       # Run with GDB server
  idf.py gdb              # Attach GDB debugger
```

**Automation Script**
- `scripts/script.ps1` — PowerShell script for streamlined workflow

### Code Quality Tools

**C++ Tools (Firmware)**
- **clang-format**: Code formatting
- **clang-tidy**: Static analysis

**Python Tools (Backend)**
- **black**: Code formatter
- **pylint**: Linting

---

## Communication Protocols

### POC Protocols
- **UART/Serial**: Primary firmware ↔ backend communication
- **HTTP/HTTPS**: REST API (ESP32 WiFi → FastAPI)
- **WebSocket**: Dashboard live updates

### Hardware Protocols
- **GPIO**: Tamper sensor digital input
- **Interrupt**: ISR for tamper detection
- **I2C**: Sensor communication (if needed)

### Data Formats
- **Binary**: Optimized secure packet (firmware → backend)
- **JSON**: API responses (backend → dashboard)

---

## Security Tools

### Cryptographic Libraries

**micro-ecc (Embedded)**
- **Purpose**: Lightweight ECC for ESP32
- **Algorithm**: ECDSA secp256r1
- **Size**: < 20KB code footprint

---

## Hardware Tools & Equipment

### Development Boards (PIC: Cesar)
- ESP32 DevKit V1 (×2) — primary target
- Logic Analyzer (debugging)

### Sensors Inventory (PIC: Cesar)
- Hall Effect Sensors (ACS712) — current measurement
- Voltage Sensors (ZMPT101B) — voltage measurement
- Limit Switches — tamper detection
- Temperature Sensors (DS18B20) — temperature monitoring

### Power Supply
- USB Power Banks (portable testing)
- Supercapacitors (backup power simulation)

---

## Scale-Up Stack (Future — NOT in POC)

Teknologi berikut akan dipertimbangkan saat scaling dari POC ke production:

| Category | Technology | When |
|---|---|---|
| Backend Language | Go 1.21+ | > 1,000 meters |
| Database | PostgreSQL 15+ (TimescaleDB) | > 5,000 meters |
| Cache | Redis 7.0+ | High-throughput scenarios |
| Message Broker | Mosquitto 2.0+ / EMQX | Production MQTT |
| Frontend | React 18+ / Vue 3+ | Full dashboard |
| Monitoring | Grafana 10.0+ / Prometheus | Production monitoring |
| Container | Docker 24.0+ / Docker Compose | Production deployment |
| Cloud | AWS IoT Core / Azure IoT Hub | Enterprise deployment |
| Full Crypto | OpenSSL 3.0+ / libsodium | Backend encryption |
| Security Testing | AFL++, OWASP ZAP, Nmap | Pre-production audit |
| MCU Support | STM32F4, Nordic nRF52 | Multi-platform |
| RTOS | Zephyr | Alternative RTOS |

---

## Recommended Development Setup (POC)

```
Required:
- VSCode + ESP-IDF Extension
- ESP-IDF v5.5+ (with toolchain)
- QEMU (via idf_tools.py)
- Python 3.11+
- Git

Hardware (when ready for physical testing):
- ESP32 DevKit V1 (×2)
- Limit switch sensors
- ACS712 current sensor
- Breadboard + jumper wires
```

---

**Document Version:** 3.0.0  
**Last Updated:** February 2026  
**Review Cycle:** Quarterly