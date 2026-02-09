# GridShield Build Guide (C++17)

## Prerequisites

### For Native Development (PC Testing)
- CMake ≥ 3.20
- C++17 compiler:
  - GCC ≥ 7.0
  - Clang ≥ 5.0
  - MSVC 2017+

### For Arduino/AVR Deployment
- Arduino CLI ≥ 1.4.1
- AVR toolchain (via Arduino)

---

## Quick Start

### Native (Development)

```bash
# Configure
cmake --preset native-debug

# Build
cmake --build --preset native-debug

# Run
./bin/NATIVE/GridShield
```

### Arduino (Production)

```bash
# Install core
arduino-cli core install arduino:avr

# Compile
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino

# Upload
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

---

## Configuration Profiles

### native-debug
- Platform: x86/x64
- Optimization: -O0
- Sanitizers: ON (Linux/macOS)
- Use: Development, debugging

### native-release
- Platform: x86/x64
- Optimization: -O3
- Sanitizers: OFF
- Use: Performance testing

### Arduino Mega (Recommended)
- MCU: ATmega2560
- Flash: 256 KB
- RAM: 8 KB
- Use: Production deployment

### Arduino Uno (Limited)
- MCU: ATmega328P
- Flash: 32 KB (NOT ENOUGH)
- RAM: 2 KB (NOT ENOUGH)
- Use: NOT RECOMMENDED

---

## Manual Build

### Linux/macOS
```bash
mkdir build && cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Windows (PowerShell)
```powershell
mkdir build; cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j8
```

---

## Production Libraries

Install via arduino-cli:

```bash
# SHA256 (optional, improves security)
arduino-cli lib install Crypto
```

Replace placeholders in code:
- `src/common/security/crypto.cpp`: Uncomment uECC integration
- `include/arduino/platform_arduino.hpp`: Uncomment SHA256

---

## Memory Optimization

### Firmware Size (Estimated)
```
Core logic:        ~35 KB
+ Crypto (real):   ~50 KB
Total:             ~85 KB
```

**Target:** ATmega2560 (256 KB Flash)

### RAM Usage
```
Static:   ~1.3 KB
Stack:    ~2.5 KB
Total:    ~3.8 KB
```

**Target:** ATmega2560 (8 KB RAM)

### Optimization Flags (Already Applied)
- `-Os`: Size optimization
- `-flto`: Link-time optimization
- `-ffunction-sections`: Dead code elimination

---

## Troubleshooting

### "Cannot find gs_macros.hpp"
**Fix:** Ensure include paths:
```cmake
include_directories(include/common include/platform)
```

### "undefined reference to std::move"
**Fix:** Add `-std=c++17` flag:
```bash
g++ -std=c++17 ...
```

### "Sketch too big"
**Fix:** Use Arduino Mega, NOT Uno

### Upload fails
**Fix:** Check port:
```bash
arduino-cli board list
```

---

## CMake Presets

List available:
```bash
cmake --list-presets
```

Output:
```
native-debug   - PC Development + sanitizers
native-release - PC Performance testing
```

---

## Serial Monitor

```bash
# Arduino CLI
arduino-cli monitor -p COM3 -b 115200

# Screen (Linux)
screen /dev/ttyUSB0 115200
```

Expected output:
```
=== GridShield v1.0 ===
Booting...
Initializing... OK
Starting... OK
System running.
```

---

## Production Deployment

### 1. Hardware Integration
Replace mock drivers in `platform_arduino.hpp` with actual:
- SPI/I2C for secure element (ATECC608)
- LoRa/NB-IoT communication
- Real sensor inputs

### 2. Cryptography
Integrate production libraries:
```cpp
// crypto.cpp
#include <uECC.h>
const struct uECC_Curve_t* curve = uECC_secp256r1();
```

### 3. Security Hardening
- [ ] Enable watchdog timer
- [ ] Implement secure boot
- [ ] Add OTA updates
- [ ] Store keys in EEPROM/Secure Element
- [ ] Enable stack canaries

---

## File Structure

```
gridshield/
├── CMakeLists.txt              # Root build config
├── include/
│   ├── common/                 # Platform-agnostic code
│   │   ├── utils/gs_macros.hpp # Platform macros (C++17)
│   │   ├── core/
│   │   │   ├── error.hpp       # Result<T> monad
│   │   │   ├── types.hpp       # Domain types
│   │   │   └── system.hpp      # Main orchestrator
│   │   ├── security/crypto.hpp # Crypto engine
│   │   ├── hardware/tamper.hpp # Tamper detection
│   │   ├── network/packet.hpp  # Secure packets
│   │   └── analytics/detector.hpp # Anomaly detection
│   ├── platform/platform.hpp   # HAL interfaces
│   ├── native/platform_native.hpp # PC mock
│   └── arduino/platform_arduino.hpp # AVR drivers
├── src/
│   ├── common/                 # Implementation
│   ├── native/main.cpp         # PC entry point
│   └── arduino/gridshield.ino  # Arduino entry point
└── CMakePresets.json           # Build presets
```

---

## License
MIT - See LICENSE file