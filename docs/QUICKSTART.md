# GridShield - Quick Start Guide

Get GridShield running in **5 minutes** with ESP-IDF and QEMU simulation.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Build & Run](#build--run)
- [Understanding the Output](#understanding-the-output)
- [Next Steps](#next-steps)

---

## Prerequisites

### ESP-IDF v5.5+

Follow the official [ESP-IDF installation guide](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/get-started/).

**Windows (recommended path):**
```
C:\esp\v5.5.3\esp-idf\
```

### QEMU (Optional)

```powershell
# Install via automation script
.\scripts\script.ps1 --setup

# Or manually
python $IDF_PATH/tools/idf_tools.py install qemu-xtensa qemu-riscv32
```

---

## Installation

```bash
# Clone repository
git clone https://github.com/zuudevs/gridshield.git
cd gridshield
```

---

## Build & Run

### Option A: Using Automation Script (Recommended)

```powershell
# Build firmware
.\scripts\script.ps1 --build

# Build + Run in QEMU
.\scripts\script.ps1 --run
```

### Option B: Using ESP-IDF Directly

```bash
cd firmware

# Set target (first time only)
idf.py set-target esp32

# Build
idf.py build

# Run in QEMU
idf.py qemu monitor
```

---

## Understanding the Output

**Expected output when running in QEMU:**

```
[GridShield] ==============================================
[GridShield] GridShield v3.0.1 [ESP32 - QEMU Simulation]
[GridShield] Platform: ESP-IDF + QEMU
[GridShield] ==============================================

[GridShield] [Init] Configuring system...
[GridShield]   Meter ID: 0x1234567890ABCDEF
[GridShield]   Tamper Pin: 4 | Debounce: 50ms
[GridShield]   Max Cycles: 20

[GridShield] System started successfully
[GridShield] Entering main processing loop...

[GridShield] Cycle 1/20 OK
[GridShield] Cycle 2/20 OK
...
[GridShield] Cycle 20/20 OK
[GridShield] Simulation complete — all cycles finished
```

**🎉 Success!** You've just run GridShield's multi-layer security system in QEMU.

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

---

## Code Overview

### Entry Point

The `app_main()` function in `firmware/main/app_main.cpp` is the ESP-IDF entry point:

```cpp
#include "core/system.hpp"
#include "platform/mock_platform.hpp"

void app_main(void) {
    // Setup mock platform services for QEMU
    gridshield::platform::MockTime mock_time;
    gridshield::platform::MockGPIO mock_gpio;
    // ... more mock services ...

    // Configure system
    gridshield::SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    config.tamper_config.sensor_pin = 4;

    // Initialize and run
    gridshield::GridShieldSystem system;
    system.initialize(config, services);
    system.start();

    while (cycle < max_cycles) {
        system.process_cycle();
        vTaskDelay(pdMS_TO_TICKS(500));
        ++cycle;
    }
}
```

### Key Concepts

**Result<T> Monad** — Type-safe error handling without exceptions:
```cpp
auto result = system.process_cycle();
if (result.is_ok()) {
    // success
} else {
    auto error = result.error();
    // handle error
}
```

**Platform Abstraction Layer (HAL)** — Clean hardware interfaces:
```cpp
class IPlatformTime {
    virtual timestamp_t get_timestamp_ms() = 0;
    virtual void delay_ms(uint32_t milliseconds) = 0;
};
```

---

## Next Steps

### 📚 Learn More

- [**Architecture**](ARCHITECTURE.md) — System design with diagrams
- [**API Reference**](API.md) — Firmware & backend API docs
- [**Build Guide**](../BUILD.md) — Advanced build configurations

### 🖥️ Backend & Frontend

**Start the Backend:**
```bash
cd backend
pip install -r requirements.txt
uvicorn app.main:app --reload --port 8000
```

- Swagger UI: [http://localhost:8000/docs](http://localhost:8000/docs)

**Start the Frontend Dashboard:**
```bash
cd frontend
npm install
npm run dev
```

- Dashboard: [http://localhost:5173](http://localhost:5173)

### 🔧 Customize

**1. Configure Tamper Detection:**
```cpp
config.tamper_config.sensor_pin = 3;
config.tamper_config.debounce_ms = 100;
```

**2. Adjust Anomaly Thresholds:**
```cpp
config.baseline_profile.variance_threshold = 40;  // More lenient (40%)
config.baseline_profile.variance_threshold = 15;  // Stricter (15%)
```

**3. Change Cycle Count:**
```cpp
const int max_cycles = 100;  // Longer simulation
```

### 🐛 Troubleshooting

**Build fails with "ESP-IDF not found":**
```bash
# Ensure ESP-IDF environment is exported
C:\esp\v5.5.3\esp-idf\export.bat  # Windows
. $IDF_PATH/export.sh              # Linux/macOS
```

**QEMU not found:**
```powershell
.\scripts\script.ps1 --setup
```

---

## Support

- **Documentation:** `docs/` folder
- **Issues:** Report bugs on GitHub
- **Contact:** zuudevs@gmail.com

---

**Ready to secure your AMI system?** Start building!

**Institut Teknologi PLN — 2026**