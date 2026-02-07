# GridShield - Quick Start Guide

## Prerequisites

### Required Tools
```bash
# CMake
cmake --version  # ≥ 3.23

# Ninja (recommended build system)
ninja --version

# Arduino CLI (for AVR upload)
arduino-cli version  # ≥ 1.4.1
```

### Install Arduino Core (AVR only)
```bash
arduino-cli core install arduino:avr
```

---

## Build Commands (Using Presets)

### 1. Native Development (PC Testing)

**Debug build with sanitizers:**
```bash
cmake --preset native-debug
cmake --build --preset native-debug
./bin/NATIVE/GridShield
```

**Release build (optimized):**
```bash
cmake --preset native-release
cmake --build --preset native-release
./bin/NATIVE/GridShield
```

---

### 2. Arduino Uno (ATmega328P)

⚠️ **WARNING:** Uno has only 32KB flash and 2KB RAM. Firmware may not fit!

**Build:**
```bash
cmake --preset avr-uno
cmake --build --preset avr-uno
```

**Upload:**
```bash
# Windows
cmake --build build/avr-uno --target upload -DAVR_UPLOAD_PORT=COM3

# Linux/macOS
cmake --build build/avr-uno --target upload -DAVR_UPLOAD_PORT=/dev/ttyUSB0
```

---

### 3. Arduino Mega (ATmega2560) ✅ RECOMMENDED

**Build:**
```bash
cmake --preset avr-mega
cmake --build --preset avr-mega
```

**Upload:**
```bash
# Windows
cmake --build build/avr-mega --target upload -DAVR_UPLOAD_PORT=COM3

# Linux/macOS
cmake --build build/avr-mega --target upload -DAVR_UPLOAD_PORT=/dev/ttyUSB0
```

**Output:** `bin/AVR/GridShield.hex`

---

## Available Presets

List all presets:
```bash
cmake --list-presets
```

Output:
```
Available configure presets:
  "native-debug"   - Native Debug (PC Testing)
  "native-release" - Native Release (PC Optimized)
  "avr-uno"        - Arduino Uno (ATmega328P)
  "avr-mega"       - Arduino Mega (ATmega2560)
  "esp32"          - ESP32 (Xtensa) [not yet supported]
```

---

## Workflow Presets (All-in-One)

**Development workflow** (configure + build + test):
```bash
cmake --workflow --preset dev
```

**Deploy to Mega** (configure + build):
```bash
cmake --workflow --preset deploy-mega
```

---

## Manual Upload (Alternative)

If CMake upload target fails, use `avrdude` directly:

```bash
avrdude -p atmega2560 \
        -c wiring \
        -P /dev/ttyUSB0 \
        -b 115200 \
        -D \
        -U flash:w:bin/AVR/GridShield.hex:i
```

---

## Serial Monitor

**Arduino CLI:**
```bash
arduino-cli monitor -p COM3 -b 115200
```

**PlatformIO:**
```bash
pio device monitor -b 115200
```

**Screen (Linux):**
```bash
screen /dev/ttyUSB0 115200
```

---

## Troubleshooting

### "Preset not found"
**Fix:** Update CMake to ≥ 3.23
```bash
cmake --version
```

### "avr-gcc not found"
**Fix:** Set toolchain path in `cmake/toolchain-avr.cmake`:
```cmake
set(AVR_TOOLCHAIN_PATH "/path/to/arduino/tools/avr-gcc/...")
```

### "Sketch too big"
**Fix:** Use Arduino Mega instead of Uno:
```bash
cmake --preset avr-mega
```

### Upload fails "Permission denied"
**Fix (Linux):**
```bash
sudo usermod -a -G dialout $USER
# Logout and login again
```

---

## Clean Build

```bash
# Clean all presets
rm -rf build/

# Clean specific preset
rm -rf build/avr-mega
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

**Flash limit:** Mega = 256KB, Uno = 32KB  
**RAM limit:** Mega = 8KB, Uno = 2KB

---

## VSCode Integration

Install **CMake Tools** extension, then:

1. Open Command Palette (`Ctrl+Shift+P`)
2. Select `CMake: Select Configure Preset`
3. Choose `avr-mega` or `native-debug`
4. Press `F7` to build

---

## Next Steps

1. ✅ Build native version for testing
2. ✅ Flash to Arduino Mega
3. 📝 Read `docs/ARCHITECTURE.md` for code structure
4. 🔒 Replace placeholder crypto (see `BUILD.md`)
5. 🚀 Deploy to production hardware

---

## Support

- **Issues:** GitHub Issues
- **Docs:** `docs/` folder
- **Examples:** `examples/` folder (coming soon)