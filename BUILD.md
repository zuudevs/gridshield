# GridShield Build & Upload Guide

## Prerequisites

### For Native Development (PC Testing)
- CMake ≥ 3.20
- C++17 compiler (GCC/Clang/MSVC)

### For Arduino/AVR Deployment
- Arduino CLI ≥ 1.4.1
- AVR toolchain (installed via Arduino)

---

## Build Methods

### 1. Native Build (Development/Testing)

```bash
# Linux/macOS
mkdir build && cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)

# Windows (PowerShell)
mkdir build; cd build
cmake -DPLATFORM=NATIVE -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . -j8
```

**Output:** `bin/NATIVE/GridShield` (executable for PC)

---

### 2. Arduino Build via CMake (Advanced)

```bash
mkdir build-avr && cd build-avr
cmake -DPLATFORM=AVR -DCMAKE_BUILD_TYPE=Release ..
make
```

**Output:** `bin/AVR/GridShield.hex` (ready to upload)

**Upload:**
```bash
avrdude -p atmega328p -c arduino -P COM3 -b 115200 \
        -U flash:w:bin/AVR/GridShield.hex:i
```

---

### 3. Arduino CLI Build (Recommended for Beginners)

#### Setup
```bash
# Install AVR core
arduino-cli core install arduino:avr

# Install required libraries
arduino-cli lib install "Crypto" # For SHA256 (optional)
```

#### Compile
```bash
arduino-cli compile --fqbn arduino:avr:uno gridshield.ino
```

#### Upload
```bash
arduino-cli upload -p COM3 --fqbn arduino:avr:uno gridshield.ino
```

---

## Required Third-Party Libraries

### For Production Cryptography (Optional)

Replace placeholder crypto in `platform_arduino.hpp`:

1. **SHA256 Hash:**
   - Library: `Crypto` by Rhys Weatherley
   - Install: `arduino-cli lib install Crypto`
   - Usage: Uncomment lines marked `// NEED ADOPTION`

2. **ECC Signatures (Advanced):**
   - Library: `uECC` (micro-ecc)
   - Manual integration required (not Arduino Library Manager)

---

## Memory Optimization

### Flash Usage
- Core firmware: ~35 KB
- With Crypto lib: ~85 KB
- **Target MCU:** ATmega328P (32 KB Flash) - **too small!**
- **Recommended:** ATmega2560 (256 KB) or ESP32

### RAM Usage
- Static: ~1.3 KB
- Stack: ~2.5 KB
- **Target MCU:** ATmega328P (2 KB RAM) - **marginal!**
- **Recommended:** ESP32 (520 KB RAM)

### Optimization Flags (Already Applied)
- `-Os` - Optimize for size
- `-flto` - Link-time optimization
- `-ffunction-sections` - Dead code elimination

---

## Debugging

### Serial Monitor
```bash
arduino-cli monitor -p COM3 -b 115200
```

### Expected Output
```
=== GridShield v1.0 ===
Booting...
Initializing... OK
Starting... OK
System running.
```

---

## Common Errors

### 1. "ZMOVE not defined"
**Fix:** Include `utils/gs_macros.hpp` in all headers using move semantics.

### 2. "Cannot find Arduino.h"
**Fix:** Ensure AVR core is installed:
```bash
arduino-cli core list
```

### 3. "Sketch too big"
**Fix:** Use larger MCU (ESP32) or disable unused modules.

### 4. Upload fails
**Fix:** Check port and baud rate:
```bash
arduino-cli board list
```

---

## Production Deployment Checklist

- [ ] Replace mock crypto with mbedTLS/uECC
- [ ] Enable hardware random number generator
- [ ] Store keys in EEPROM/Secure Element
- [ ] Implement watchdog timer
- [ ] Add OTA update capability
- [ ] Enable encryption for serial communication
- [ ] Test tamper detection with real hardware

---

## File Structure

```
gridshield/
├── include/
│   ├── utils/gs_macros.hpp          # Platform macros
│   ├── core/
│   │   ├── error.hpp                # Result<T> monad
│   │   ├── types.hpp                # Domain types
│   │   └── system.hpp               # Main orchestrator
│   ├── platform/
│   │   ├── platform.hpp             # HAL interfaces
│   │   ├── mock_platform.hpp        # PC mock
│   │   └── arduino/
│   │       ├── gpio_arduino.hpp     # AVR GPIO
│   │       └── platform_arduino.hpp # AVR drivers
│   ├── hardware/tamper.hpp
│   ├── security/crypto.hpp
│   ├── network/packet.hpp
│   └── analytics/detector.hpp
├── src/
│   ├── main.cpp                     # Arduino entry point
│   ├── core/system.cpp
│   ├── hardware/tamper.cpp
│   ├── security/crypto.cpp
│   ├── network/packet.cpp
│   └── analytics/detector.cpp
├── CMakeLists.txt                   # Build configuration
└── gridshield.ino                   # Arduino wrapper
```

---

## License
MIT - See LICENSE file