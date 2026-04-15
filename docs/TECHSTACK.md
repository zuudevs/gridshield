# GridShield Technology Stack

**Version:** 3.3.1  
**Last Updated:** April 2026  
**Maintained By:** GridShield Development Team  
**Implementasi Aktual:** Seluruh implementasi teknis di repository ini dikerjakan oleh **Rafi Indra Pramudhito Zuhayr**

## Overview

GridShield utilizes a carefully selected technology stack optimized for embedded systems, security, and scalability. This document is scoped to the **POC (Proof of Concept)** — technologies actively used by a 3-person team — with future/scale-up items clearly separated.

---

## POC Scope Summary

| Layer | Technology | PIC |
|---|---|---|
| **Firmware** | C++17, ESP-IDF v5.5, ESP32 | Rafi |
| **Simulation** | QEMU (ESP32 emulator) | Rafi |
| **Crypto** | micro-ecc (ECDSA secp256r1) | Rafi |
| **Backend** | Python 3.11+, FastAPI, SQLite | Ichwan |
| **Frontend** | Vite + Chart.js (vanilla JS) | Ichwan |
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
- **Purpose**: Simulated ESP32 without physical hardware
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
- **Serial (UART)**: Debug output + data transmission to backend (POC)
- **WiFiClient**: HTTP communication to backend (optional for POC)

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
> PostgreSQL, Redis, and other enterprise databases are not needed for POC. SQLite is sufficient for demos with dozens of meters.

### Communication Protocol (POC)

**Serial (UART) — Primary**
- ESP32 sends data via Serial to PC
- Python backend reads from serial port
- Simplest approach for POC demo

**HTTP REST — Alternative**
- ESP32 sends via WiFi HTTP POST to FastAPI
- More realistic for production-like demo

---

## Frontend & Visualization (PIC: Ichwan)

### Dashboard (POC Scope)

**Vite + Vanilla JavaScript SPA**
- **Build Tool**: Vite 6.x (hot module replacement, fast builds)
- **Chart.js**: Data visualization (consumption graph, KPI charts)
- **SPA Router**: Custom client-side routing (4 pages)
- **API Client**: Fetch-based backend polling

**Pages:**
- **Dashboard** — KPIs, consumption chart, recent alerts
- **Alerts** — Tamper alert management
- **Anomalies** — Detection logs with filtering
- **Fleet** — Meter management console

> [!NOTE]
> React, Vue, TailwindCSS, Grafana — none of these are needed for POC. A simple SPA with Chart.js is sufficient.

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
- **GPIO**: Tamper sensor digital input (GPIO4, ISR FALLING edge)
- **Interrupt**: ISR for tamper detection (50ms debounce)
- **UART2**: PZEM-004T energy meter (RX→GPIO16, TX→GPIO17)
- **OneWire**: DHT11 temperature/humidity (GPIO13, open-drain)
- **PWM (LEDC)**: Buzzer alert tones (GPIO25)
- **Digital OUT**: Relay control (GPIO26), LED heartbeat (GPIO2)

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
- Multimeter Digital (validation)

### Hardware Inventory (PIC: Cesar)
- **PZEM-004T v3.0** Energy Meter — V/I/P/E/F/PF via UART2 Modbus
- **PZKHCT** CT Clamp — current transformer for PZEM
- **DHT11** Temp/Humidity — thermal monitoring via GPIO13 (open-drain)
- **JQC-3FF-S-Z** Relay — AC load control via GPIO26
- **Piezo Buzzer** — alert tones via GPIO25 (LEDC PWM)
- **Tamper Switch** (pull-up) — enclosure protection via GPIO4 ISR
- **MCB IC60N** — AC circuit breaker
- **MEAN WELL IRM-10-3.3** — AC-DC PSU 3.3V 3A
- **TP4056** — Li-ion charger module
- **MT3608** — DC-DC boost converter (3.3V → 5V)
- **Li-ion 18650 2500mAh** — backup battery (UPS)

> **📋 Hardware Design:** Lihat [Wiring Guide](WIRING_GUIDE.md) dan [IoT Hardware Design](design/iot-design.html) untuk wiring schematic, pinout, dan assembly guide lengkap.

---

## Scale-Up Stack (Future — NOT in POC)

Technologies below will be considered when scaling from POC to production:

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

Hardware (actual components):
- ESP32 DevKit V1
- PZEM-004T v3.0 + PZKHCT CT Clamp
- DHT11 temperature/humidity sensor
- JQC-3FF-S-Z relay module
- Piezo buzzer (3.3V)
- Tamper switch (pull-up)
- MCB IC60N
- MEAN WELL IRM-10-3.3 (AC-DC PSU)
- TP4056 Li-ion charger module
- MT3608 DC-DC boost converter
- Li-ion 18650 2500mAh battery
- Terminal blocks + breadboard + jumper wires
```

---

**Document Version:** 3.3.1  
**Last Updated:** April 2026  
**Review Cycle:** Quarterly