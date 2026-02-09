# GridShield Build Guide

Complete build instructions for GridShield Multi-Layer AMI Security System (C++17).

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Platforms](#build-platforms)
- [Configuration Profiles](#configuration-profiles)
- [Advanced Build Options](#advanced-build-options)
- [Production Deployment](#production-deployment)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### For Native Development (PC Testing)

**Required:**
- CMake ≥ 3.23
- Ninja build system (recommended) or Make
- C++17 compliant compiler:
  - GCC ≥ 7.0
  - Clang ≥ 5.0
  - MSVC 2017+

**Installation:**

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install cmake ninja-build g++

# macOS
brew install cmake ninja

# Windows (via Chocolatey)
choco install cmake ninja visualstudio2022buildtools
```

### For Arduino/AVR Deployment

**Required:**
- Arduino CLI ≥ 1.4.1
- AVR toolchain (installed via Arduino CLI)

**Installation:**

```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Windows (PowerShell)
iwr -useb https://raw.githubusercontent.com/arduino/arduino-cli/master/install.ps1 | iex

# Install AVR core
arduino-cli core install arduino:avr
```

---

## Quick Start

### Native Development (PC)

Build and run the development version:

```bash
# Configure
cmake --preset native-debug

# Build
cmake --build --preset native-debug

# Run
./bin/NATIVE/GridShield
```

**Expected Output:**
```
═══════════════════════════════════════════
  GridShield AMI Security System v1.0.0
═══════════════════════════════════════════
[Initialization]
Meter ID: 0x1234567890ABCDEF
✓ System initialized successfully
✓ System started
```

### Arduino Production (Mega 2560)

Compile and upload to Arduino Mega:

```bash
# Compile
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino

# Upload (adjust COM port)
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino

# Monitor serial output
arduino-cli monitor -p COM3 -b 115200
```

---

## Build Platforms

### 1. Native (x86/x64) - Development Platform

**Purpose:** Local development, debugging, and testing

**Characteristics:**
- Full STL support
- Address sanitizers available
- Fast compilation
- No hardware constraints

**Build Commands:**

```bash
# Debug build (with sanitizers)
cmake --preset native-debug
cmake --build --preset native-debug

# Release build (optimized)
cmake --preset native-release
cmake --build --preset native-release
```

**Output:** `bin/NATIVE/GridShield`

### 2. Arduino Mega 2560 - Production Platform ✅ RECOMMENDED

**Purpose:** Production deployment on ATmega2560

**Characteristics:**
- MCU: ATmega2560
- Flash: 256 KB (sufficient for full firmware + crypto)
- RAM: 8 KB (adequate for all buffers)
- Production-ready

**Build via CMake:**

```bash
cmake --preset avr-mega
cmake --build --preset avr-mega
```

**Build via Arduino CLI (Recommended):**

```bash
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

**Output:** `bin/AVR/GridShield.hex`

### 3. Arduino Uno - Limited Support ⚠️

**Warning:** Arduino Uno has only 32KB flash and 2KB RAM - **NOT SUFFICIENT** for full firmware.

**Build:**

```bash
cmake --preset avr-uno
cmake --build --preset avr-uno
```

**Limitations:**
- Firmware size: ~85 KB (exceeds 32 KB limit)
- RAM usage: ~3.8 KB (exceeds 2 KB limit)
- **Use Arduino Mega instead**

---

## Configuration Profiles

### native-debug

**Purpose:** Development and debugging on PC

```bash
cmake --preset native-debug
cmake --build --preset native-debug
```

**Features:**
- Platform: x86/x64
- Optimization: `-O0` (no optimization)
- Sanitizers: ON (AddressSanitizer, UndefinedBehaviorSanitizer)
- Debug symbols: Full
- Use case: Active development, bug hunting

### native-release

**Purpose:** Performance testing on PC

```bash
cmake --preset native-release
cmake --build --preset native-release
```

**Features:**
- Platform: x86/x64
- Optimization: `-O3` (maximum speed)
- Sanitizers: OFF
- Debug symbols: Minimal
- Use case: Benchmarking, performance validation

### avr-mega

**Purpose:** Production deployment on Arduino Mega

```bash
cmake --preset avr-mega
cmake --build --preset avr-mega
```

**Features:**
- MCU: ATmega2560
- Optimization: `-Os` (size optimization)
- Link-time optimization: ON
- Dead code elimination: ON
- Flash: 256 KB available

### avr-uno

**Purpose:** Testing on Arduino Uno (NOT RECOMMENDED)

```bash
cmake --preset avr-uno
cmake --build --preset avr-uno
```

**Limitations:**
- Flash: 32 KB (INSUFFICIENT)
- RAM: 2 KB (INSUFFICIENT)
- **Firmware will not fit**

---

## Advanced Build Options

### List Available Presets

```bash
cmake --list-presets
```

**Output:**
```
Available configure presets:
  "native-debug"   - Native Debug (PC Testing)
  "native-release" - Native Release (PC Optimized)
  "avr-uno"        - Arduino Uno (ATmega328P)
  "avr-mega"       - Arduino Mega (ATmega2560)
```

### Workflow Presets (All-in-One)

**Development Workflow** (configure + build + test):

```bash
cmake --workflow --preset dev
```

**Production Deployment** (configure + build):

```bash
cmake --workflow --preset deploy-mega
```

### Manual Build (Without Presets)

**Linux/macOS:**

```bash
mkdir build && cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

**Windows (PowerShell):**

```powershell
mkdir build; cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j8
```

### Clean Build

```bash
# Clean all builds
rm -rf build/

# Clean specific preset
rm -rf build/avr-mega
```

---

## Production Deployment

### 1. Install Production Libraries

Install cryptography libraries via Arduino CLI:

```bash
# SHA256 and AES (optional but recommended)
arduino-cli lib install Crypto
```

### 2. Enable Production Crypto

**Edit `src/common/security/crypto.cpp`:**

Uncomment uECC integration:

```cpp
#include <uECC.h>

core::Result<void> CryptoEngine::sign(...) {
    const struct uECC_Curve_t* curve = uECC_secp256r1();
    // Use real ECC signing
    if (!uECC_sign(keypair.get_private_key(), hash, SHA256_HASH_SIZE, 
                   signature_out, curve)) {
        return GS_MAKE_ERROR(core::ErrorCode::SignatureInvalid);
    }
    return core::Result<void>();
}
```

**Edit `include/arduino/platform_arduino.hpp`:**

Uncomment SHA256 implementation:

```cpp
#include <SHA256.h>

core::Result<void> ArduinoCrypto::sha256(...) {
    SHA256 sha256;
    sha256.update(data, length);
    sha256.finalize(hash_out, 32);
    return core::Result<void>();
}
```

### 3. Memory Optimization

**Firmware Size (Estimated):**

```
Core logic:        ~35 KB
+ Crypto (real):   ~50 KB
─────────────────────────
Total:             ~85 KB
```

**Target:** ATmega2560 (256 KB Flash) ✅

**RAM Usage:**

```
Static data:   ~1.3 KB
Stack:         ~2.5 KB
─────────────────────────
Total:         ~3.8 KB
```

**Target:** ATmega2560 (8 KB RAM) ✅

### 4. Hardware Integration

Replace mock drivers with actual hardware:

**SPI/I2C for Secure Element:**

```cpp
// platform_arduino.hpp
#include <Wire.h>
#include <ATECC608.h>

class ArduinoSecureElement {
    ATECC608 secure_element;
    // Implement secure key storage
};
```

**LoRa/NB-IoT Communication:**

```cpp
#include <LoRa.h>

class ArduinoLoRaComm : public IPlatformComm {
    core::Result<size_t> send(const uint8_t* data, size_t length) noexcept override {
        LoRa.beginPacket();
        LoRa.write(data, length);
        return core::Result<size_t>(LoRa.endPacket());
    }
};
```

### 5. Upload to Production Hardware

```bash
# Check available ports
arduino-cli board list

# Upload with verification
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino --verify

# Monitor serial output
arduino-cli monitor -p COM3 -b 115200
```

**Expected Serial Output:**

```
=== GridShield v1.0 ===
Booting...
Initializing... OK
Starting... OK
System running.
```

---

## Troubleshooting

### Issue: "Cannot find gs_macros.hpp"

**Cause:** Include paths not configured

**Fix:**

Ensure CMakeLists.txt has:

```cmake
target_include_directories(${PROJECT_NAME} PRIVATE
    include/common
    include/platform
    include/native
)
```

### Issue: "undefined reference to std::move"

**Cause:** Missing C++17 flag

**Fix:**

Add to CMakeLists.txt:

```cmake
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

Or compile manually:

```bash
g++ -std=c++17 -o gridshield src/main.cpp ...
```

### Issue: "Sketch too big for Arduino Uno"

**Error:**
```
Sketch uses 85234 bytes (262%) of program storage space.
Global variables use 3845 bytes (187%) of dynamic memory.
```

**Fix:** Use Arduino Mega 2560 instead:

```bash
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

### Issue: Upload fails with "Permission denied"

**Platform:** Linux/macOS

**Fix:**

Add user to dialout group:

```bash
sudo usermod -a -G dialout $USER
# Logout and login again
```

**Alternative:** Use sudo (not recommended):

```bash
sudo arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

### Issue: "avr-gcc: command not found"

**Cause:** AVR toolchain not installed

**Fix:**

```bash
# Install via Arduino CLI
arduino-cli core install arduino:avr

# Or set toolchain path manually
export PATH="/path/to/arduino/tools/avr-gcc/bin:$PATH"
```

### Issue: CMake preset not found

**Error:**
```
CMake Error: No such preset in .../CMakePresets.json: "native-debug"
```

**Fix:**

Update CMake to ≥ 3.23:

```bash
cmake --version
# If < 3.23, upgrade:
pip install --upgrade cmake
```

---

## Serial Monitor Options

### Arduino CLI

```bash
arduino-cli monitor -p COM3 -b 115200
```

### Screen (Linux/macOS)

```bash
screen /dev/ttyUSB0 115200
# Exit: Ctrl+A, then K
```

### PlatformIO

```bash
pio device monitor -b 115200
```

---

## Memory Usage Report

After building AVR target, check output:

```
AVR Memory Usage
Device: atmega2560

Program:   45678 bytes (17.4% Full)
(.text + .data + .bootloader)

Data:       3456 bytes (42.2% Full)
(.data + .bss + .noinit)
```

**Flash Limits:**
- Mega 2560: 256 KB ✅
- Uno: 32 KB ❌

**RAM Limits:**
- Mega 2560: 8 KB ✅
- Uno: 2 KB ❌

---

## File Structure Reference

```
gridshield/
├── CMakeLists.txt              # Root build config
├── CMakePresets.json           # Build presets
├── include/
│   ├── common/                 # Platform-agnostic code
│   │   ├── utils/gs_macros.hpp # Platform macros (C++17)
│   │   ├── core/
│   │   ├── security/
│   │   ├── hardware/
│   │   ├── network/
│   │   └── analytics/
│   ├── platform/               # HAL interfaces
│   ├── native/                 # PC implementation
│   └── arduino/                # AVR implementation
├── src/
│   ├── common/                 # Core implementations
│   ├── native/main.cpp         # PC entry point
│   └── arduino/gridshield.ino  # Arduino entry point
└── bin/                        # Build outputs
    ├── NATIVE/
    └── AVR/
```

---

## Next Steps

1. ✅ Build native version for testing
2. ✅ Test on PC with simulated sensors
3. 📦 Flash to Arduino Mega
4. 🔒 Integrate production crypto libraries
5. 🔌 Connect real hardware sensors
6. 🚀 Deploy to production environment

---

## Support

- **Documentation:** `docs/` folder
- **Issues:** GitHub Issues
- **Contact:** zuudevs@gmail.com

---

## License

MIT License - See LICENSE file for details.

**Institut Teknologi PLN - 2025**