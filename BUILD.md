# GridShield Build Guide

Build instructions for GridShield firmware using **ESP-IDF v5.5** and **QEMU** simulation.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Quick Start](#quick-start)
- [Build Commands](#build-commands)
- [QEMU Simulation](#qemu-simulation)
- [Debugging with GDB](#debugging-with-gdb)
- [Automation Script](#automation-script)
- [Project Structure](#project-structure)
- [Troubleshooting](#troubleshooting)

---

## Prerequisites

### ESP-IDF v5.5+

Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/).

**Windows (recommended path):**
```
C:\esp\v5.5.3\esp-idf\
```

### QEMU (Optional — for simulation)

Install QEMU for ESP32 via ESP-IDF tools:

```bash
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

Or via the automation script:

```powershell
.\scripts\script.ps1 --setup
```

---

## Quick Start

### Using Automation Script (Recommended)

```powershell
# First time: install QEMU
.\scripts\script.ps1 --setup

# Build firmware
.\scripts\script.ps1 --build

# Build + Run in QEMU with IDF Monitor
.\scripts\script.ps1 --run
```

### Using ESP-IDF Directly

```bash
cd firmware

# Set target (one-time)
idf.py set-target esp32

# Build
idf.py build

# Run in QEMU
idf.py qemu monitor
```

---

## Build Commands

All commands are run from the `firmware/` directory with ESP-IDF environment activated.

| Command | Description |
|---------|-------------|
| `idf.py set-target esp32` | Set build target (one-time) |
| `idf.py build` | Compile firmware |
| `idf.py fullclean` | Clean all build artifacts |
| `idf.py qemu monitor` | Run in QEMU with IDF Monitor |
| `idf.py qemu` | Run in QEMU (raw console) |
| `idf.py qemu --gdb monitor` | Run QEMU with GDB server |
| `idf.py gdb` | Attach GDB to QEMU |
| `idf.py menuconfig` | Open ESP-IDF config UI |

### Build Output

```
firmware/build/
├── GRIDSHEILD.bin        # Firmware binary
├── GRIDSHEILD.elf        # ELF with debug symbols
├── bootloader/           # Second-stage bootloader
└── partition_table/      # Partition layout
```

---

## QEMU Simulation

### Run with IDF Monitor (Colored Output)

```bash
idf.py qemu monitor
```

- **Exit:** `Ctrl+]`
- Features: colored log output, serial input

### Run Raw QEMU Console

```bash
idf.py qemu
```

- **QEMU console:** `Ctrl+A` then `C`
- **Exit:** `Ctrl+A` then `Q`

### Expected Output

```
[GridShield] ==============================================
[GridShield] GridShield v1.1 [ESP32 - QEMU Simulation]
[GridShield] Platform: ESP-IDF + QEMU
[GridShield] ==============================================
[GridShield] System started successfully
[GridShield] Entering main processing loop...
[GridShield] Cycle 1/20 OK
[GridShield] Cycle 2/20 OK
...
[GridShield] Simulation complete — all cycles finished
```

---

## Debugging with GDB

Use two terminals for interactive debugging:

**Terminal 1 — Start QEMU with GDB server:**

```powershell
.\scripts\script.ps1 --debug
```

**Terminal 2 — Attach GDB:**

```powershell
.\scripts\script.ps1 --gdb
```

Or manually:

```bash
# Terminal 1
idf.py qemu --gdb monitor

# Terminal 2
idf.py gdb
```

---

## Automation Script

The `scripts/script.ps1` script automates common workflows:

```
.\scripts\script.ps1 <command>

Commands:
  -e,  --env       Open shell with ESP-IDF environment activated
  -b,  --build     Build firmware
  -r,  --run       Build + Run in QEMU (with IDF Monitor)
  -rr, --run-raw   Build + Run in QEMU (raw console)
  -d,  --debug     Build + Run in QEMU with GDB server
  -g,  --gdb       Attach GDB to running QEMU (second terminal)
  -c,  --clean     Full clean build artifacts
  -s,  --setup     Install QEMU (one-time)
  -h,  --help      Show help
```

---

## Project Structure

```
firmware/
├── CMakeLists.txt               # Root ESP-IDF project config
├── sdkconfig                    # ESP-IDF configuration
├── include/
│   ├── common/                  # Platform-agnostic headers
│   │   ├── core/                # error.hpp, types.hpp, system.hpp
│   │   ├── security/            # crypto.hpp, key_storage.hpp
│   │   ├── hardware/            # tamper.hpp
│   │   ├── network/             # packet.hpp
│   │   ├── analytics/           # detector.hpp
│   │   └── utils/               # gs_macros.hpp, gs_utils.hpp
│   └── platform/                # platform.hpp, mock_platform.hpp
├── main/
│   ├── CMakeLists.txt           # Component registration
│   ├── app_main.cpp             # ESP-IDF entry point
│   └── src/                     # Implementation files
│       ├── analytics/           # detector.cpp
│       ├── core/                # system.cpp
│       ├── hardware/            # tamper.cpp
│       ├── network/             # packet.cpp
│       ├── platform/            # platform.cpp
│       └── security/            # crypto.cpp
└── lib/
    └── micro-ecc/               # ECC library
```

---

## Troubleshooting

### Issue: "ESP-IDF not found"

**Fix:** Ensure ESP-IDF is installed and exported:

```bash
. $IDF_PATH/export.sh    # Linux/macOS
C:\esp\v5.5.3\esp-idf\export.bat  # Windows
```

### Issue: "qemu-system-xtensa not found"

**Fix:** Install QEMU via ESP-IDF tools:

```bash
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa
. $IDF_PATH/export.sh    # Re-export PATH
```

### Issue: Build fails with C++ flag warnings on uECC.c

**Cause:** C++ flags (`-fno-rtti`, `-fno-exceptions`) applied to C files. These are warnings only and **do not affect the build**.

### Issue: "Cannot find gs_macros.hpp"

**Fix:** Ensure `main/CMakeLists.txt` includes the correct paths:

```cmake
INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/common"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/platform"
```

### Issue: Full clean rebuild

```bash
cd firmware
idf.py fullclean
idf.py set-target esp32
idf.py build
```

---

## License

MIT License — See [LICENSE](LICENSE) for details.

**Institut Teknologi PLN — 2025**