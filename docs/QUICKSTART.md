# GridShield - Quick Start Guide

Get GridShield running in **5 minutes** with this step-by-step tutorial.

---

## Table of Contents

- [Prerequisites](#prerequisites)
- [Installation](#installation)
- [Hello World - Native PC](#hello-world---native-pc)
- [Hello World - Arduino](#hello-world---arduino)
- [Next Steps](#next-steps)

---

## Prerequisites

**Choose your platform:**

### Option A: Native (PC Testing)
```bash
# Ubuntu/Debian
sudo apt install cmake ninja-build g++

# macOS
brew install cmake ninja

# Windows
choco install cmake ninja visualstudio2022buildtools
```

### Option B: Arduino (Production)
```bash
# Install Arduino CLI
curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh

# Install AVR core
arduino-cli core install arduino:avr
```

---

## Installation

**Clone and navigate to project:**

```bash
git clone https://github.com/your-org/gridshield.git
cd gridshield
```

---

## Hello World - Native PC

### Step 1: Build the Project

```bash
# Configure
cmake --preset native-debug

# Build
cmake --build --preset native-debug
```

**Expected output:**
```
[100%] Built target GridShield
```

### Step 2: Run the Demo

```bash
./bin/NATIVE/GridShield
```

**Expected output:**
```
═══════════════════════════════════════════
  GridShield AMI Security System v1.0.0
═══════════════════════════════════════════

[Initialization]
Meter ID: 0x1234567890ABCDEF
✓ System initialized successfully
✓ System started

[Phase 1: Normal Operation]
Cycle 1: ✓ Processing complete
Cycle 2: ✓ Processing complete
Cycle 3: ✓ Processing complete

[Phase 2: Tamper Detection]
Simulating physical tamper event...
✓ Tamper event processed
System State: ⚠️  TAMPERED (CRITICAL)

[Phase 3: Consumption Anomaly Detection]
Sending normal consumption readings...
  Reading 1: 1000 Wh ✓
  Reading 2: 1010 Wh ✓
  Reading 3: 1020 Wh ✓
Simulating anomalous consumption drop...
  Anomalous reading: 100 Wh ⚠️
  Analytics layer flagged potential manipulation

[Shutdown]
✓ System shutdown successful
```

**🎉 Success!** You've just run GridShield's multi-layer security demonstration.

---

## Hello World - Arduino

### Step 1: Compile the Firmware

```bash
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

**Expected output:**
```
Sketch uses 45678 bytes (17.4%) of program storage space.
Global variables use 3456 bytes (42.2%) of dynamic memory.
```

### Step 2: Upload to Arduino Mega

**Connect your Arduino Mega via USB, then:**

```bash
# Windows
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino

# Linux/macOS
arduino-cli upload -p /dev/ttyUSB0 --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

### Step 3: Monitor Serial Output

```bash
# Windows
arduino-cli monitor -p COM3 -b 115200

# Linux/macOS
arduino-cli monitor -p /dev/ttyUSB0 -b 115200
```

**Expected output:**
```
=== GridShield v1.0 ===
Booting...
Initializing... OK
Starting... OK
System running.
```

**🎉 Success!** GridShield is now running on your Arduino Mega.

---

## Complete Example: Custom Security System

Here's a minimal example showing how to use GridShield in your own application:

### Example 1: Basic Meter Reading System

**File:** `examples/basic_meter.cpp`

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"  // Use platform_arduino.hpp for Arduino

#include <iostream>

using namespace gridshield;

int main() {
    // 1. Setup platform services
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::native::NativeInterrupt interrupt;
    platform::native::NativeCrypto crypto;
    platform::native::NativeComm comm;
    
    platform::PlatformServices services;
    services.time = &time;
    services.gpio = &gpio;
    services.interrupt = &interrupt;
    services.crypto = &crypto;
    services.comm = &comm;
    
    // 2. Initialize communication
    auto init_result = comm.init();
    if (init_result.is_error()) {
        std::cerr << "Failed to initialize communication\n";
        return 1;
    }
    
    // 3. Configure system
    SystemConfig config;
    config.meter_id = 0xABCDEF1234567890;
    config.heartbeat_interval_ms = 30000;  // 30 seconds
    config.reading_interval_ms = 5000;     // 5 seconds
    
    // Configure tamper detection
    config.tamper_config.sensor_pin = 2;
    config.tamper_config.debounce_ms = 50;
    
    // Initialize baseline profile
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        config.baseline_profile.hourly_avg_wh[i] = 1200;  // 1.2 kWh baseline
    }
    config.baseline_profile.variance_threshold = 25;  // 25% deviation threshold
    
    // 4. Create and initialize system
    GridShieldSystem system;
    
    auto result = system.initialize(config, services);
    if (result.is_error()) {
        std::cerr << "System initialization failed\n";
        return 1;
    }
    
    // 5. Start the system
    result = system.start();
    if (result.is_error()) {
        std::cerr << "System start failed\n";
        return 1;
    }
    
    std::cout << "GridShield system started successfully\n";
    
    // 6. Main processing loop
    for (int i = 0; i < 10; ++i) {
        result = system.process_cycle();
        
        if (result.is_error()) {
            std::cerr << "Cycle error: " << static_cast<int>(result.error().code) << "\n";
        } else {
            std::cout << "Cycle " << (i + 1) << " completed\n";
        }
        
        // Check system state
        if (system.get_state() == core::SystemState::Tampered) {
            std::cout << "⚠️  SECURITY ALERT: Tamper detected!\n";
        }
        
        time.delay_ms(1000);  // 1 second delay
    }
    
    // 7. Cleanup
    system.shutdown();
    
    std::cout << "System shutdown complete\n";
    return 0;
}
```

**Build and run:**

```bash
# Add to CMakeLists.txt:
# add_executable(basic_meter examples/basic_meter.cpp ${COMMON_SOURCES})

cmake --preset native-debug
cmake --build --preset native-debug
./bin/NATIVE/basic_meter
```

---

### Example 2: Send Custom Meter Reading

**File:** `examples/send_reading.cpp`

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"
#include <iostream>

using namespace gridshield;

int main() {
    // Setup (same as Example 1)
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::native::NativeInterrupt interrupt;
    platform::native::NativeCrypto crypto;
    platform::native::NativeComm comm;
    
    platform::PlatformServices services;
    services.time = &time;
    services.gpio = &gpio;
    services.interrupt = &interrupt;
    services.crypto = &crypto;
    services.comm = &comm;
    
    comm.init();
    
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    
    GridShieldSystem system;
    system.initialize(config, services);
    system.start();
    
    // Create a meter reading
    core::MeterReading reading;
    reading.timestamp = time.get_timestamp_ms();
    reading.energy_wh = 1500;        // 1.5 kWh consumed
    reading.voltage_mv = 220000;     // 220V
    reading.current_ma = 6818;       // 6.818A
    reading.power_factor = 950;      // 0.95 power factor
    reading.phase = 0;
    
    std::cout << "Sending meter reading...\n";
    std::cout << "  Energy: " << reading.energy_wh << " Wh\n";
    std::cout << "  Voltage: " << reading.voltage_mv / 1000.0 << " V\n";
    std::cout << "  Current: " << reading.current_ma / 1000.0 << " A\n";
    
    // Send the reading (encrypted and signed)
    auto result = system.send_meter_reading(reading);
    
    if (result.is_ok()) {
        std::cout << "✓ Reading sent successfully\n";
    } else {
        std::cerr << "✗ Failed to send reading\n";
    }
    
    system.shutdown();
    return 0;
}
```

---

### Example 3: Anomaly Detection

**File:** `examples/detect_anomaly.cpp`

```cpp
#include "analytics/detector.hpp"
#include <iostream>

using namespace gridshield;

int main() {
    // Create anomaly detector
    analytics::AnomalyDetector detector;
    
    // Initialize with baseline profile
    analytics::ConsumptionProfile baseline;
    for (size_t i = 0; i < analytics::PROFILE_HISTORY_SIZE; ++i) {
        baseline.hourly_avg_wh[i] = 1000 + (i * 20);  // Varying by hour
    }
    baseline.daily_avg_wh = 1200;
    baseline.variance_threshold = 30;  // 30% threshold
    baseline.profile_confidence = 85;
    
    detector.initialize(baseline);
    
    // Test normal reading
    core::MeterReading normal_reading;
    normal_reading.timestamp = 3600000;  // 1 hour
    normal_reading.energy_wh = 1020;     // Close to expected 1000
    
    auto result = detector.analyze(normal_reading);
    if (result.is_ok()) {
        const auto& report = result.value();
        std::cout << "Normal Reading Analysis:\n";
        std::cout << "  Type: " << static_cast<int>(report.type) << "\n";
        std::cout << "  Severity: " << static_cast<int>(report.severity) << "\n";
        std::cout << "  Deviation: " << report.deviation_percent << "%\n";
    }
    
    // Test suspicious reading (90% drop)
    core::MeterReading suspicious_reading;
    suspicious_reading.timestamp = 3600000;
    suspicious_reading.energy_wh = 100;  // Far below expected 1000
    
    result = detector.analyze(suspicious_reading);
    if (result.is_ok()) {
        const auto& report = result.value();
        std::cout << "\nSuspicious Reading Analysis:\n";
        std::cout << "  Type: " << static_cast<int>(report.type) 
                  << " (UnexpectedDrop)\n";
        std::cout << "  Severity: " << static_cast<int>(report.severity) 
                  << " (Critical)\n";
        std::cout << "  Deviation: " << report.deviation_percent << "%\n";
        std::cout << "  ⚠️  ANOMALY DETECTED!\n";
    }
    
    return 0;
}
```

**Expected output:**
```
Normal Reading Analysis:
  Type: 0
  Severity: 0
  Deviation: 2%

Suspicious Reading Analysis:
  Type: 1 (UnexpectedDrop)
  Severity: 4 (Critical)
  Deviation: 90%
  ⚠️  ANOMALY DETECTED!
```

---

## Next Steps

### 📚 Learn More

- **[Architecture](ARCHITECTURE.md)** - Understand the system design
- **[API Reference](API.md)** - Explore all available classes and functions
- **[Build Guide](../BUILD.md)** - Advanced build configurations

### 🔧 Customize Your System

**1. Configure Tamper Detection:**

```cpp
config.tamper_config.sensor_pin = 3;           // Change pin
config.tamper_config.debounce_ms = 100;        // Increase debounce
config.tamper_config.sensitivity = 200;        // Adjust sensitivity
```

**2. Adjust Anomaly Thresholds:**

```cpp
config.baseline_profile.variance_threshold = 40;  // More lenient (40%)
config.baseline_profile.variance_threshold = 15;  // Stricter (15%)
```

**3. Change Communication Intervals:**

```cpp
config.heartbeat_interval_ms = 120000;  // 2 minutes
config.reading_interval_ms = 10000;     // 10 seconds
```

### 🚀 Deploy to Production

**1. Install Production Libraries:**

```bash
arduino-cli lib install Crypto  # SHA-256, AES
```

**2. Enable Real Cryptography:**

Edit `src/common/security/crypto.cpp` and uncomment uECC integration.

**3. Connect Hardware:**

- Tamper switch on pin 2
- Current sensor (ACS712) on A0
- Voltage sensor (ZMPT101B) on A1
- LoRa module on SPI pins

**4. Upload and Monitor:**

```bash
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino
arduino-cli monitor -p COM3 -b 115200
```

### 🐛 Troubleshooting

**Build fails with "cannot find gs_macros.hpp":**
```bash
# Ensure include directories are correct
cmake --preset native-debug -DCMAKE_VERBOSE_MAKEFILE=ON
```

**Arduino upload fails:**
```bash
# Check available ports
arduino-cli board list

# Try different baud rate
arduino-cli upload -p COM3 --fqbn arduino:avr:mega --upload-speed 57600 src/arduino/gridshield.ino
```

**System crashes on Arduino:**
- Check RAM usage: Must be < 8 KB for Mega
- Reduce buffer sizes in `core/types.hpp`
- Enable watchdog timer for auto-recovery

---

## Code Templates

### Minimal Arduino Sketch

```cpp
#include "core/system.hpp"
#include "platform_arduino.hpp"

using namespace gridshield;

static platform::arduino::ArduinoTime arduino_time;
static platform::arduino::ArduinoGPIO arduino_gpio;
static platform::arduino::ArduinoInterrupt arduino_interrupt;
static platform::arduino::ArduinoCrypto arduino_crypto;
static platform::arduino::ArduinoSerial arduino_serial;

static platform::PlatformServices services;
static GridShieldSystem* system_ptr = nullptr;

void setup() {
    // Configure platform
    services.time = &arduino_time;
    services.gpio = &arduino_gpio;
    services.interrupt = &arduino_interrupt;
    services.crypto = &arduino_crypto;
    services.comm = &arduino_serial;
    
    arduino_serial.init();
    Serial.println("Initializing...");
    
    // Create system
    system_ptr = new GridShieldSystem();
    
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    
    system_ptr->initialize(config, services);
    system_ptr->start();
    
    Serial.println("Ready!");
}

void loop() {
    system_ptr->process_cycle();
    delay(100);
}
```

### Native PC Template

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"
#include <iostream>

int main() {
    platform::native::NativeTime time;
    platform::native::NativeGPIO gpio;
    platform::native::NativeInterrupt interrupt;
    platform::native::NativeCrypto crypto;
    platform::native::NativeComm comm;
    
    platform::PlatformServices services;
    services.time = &time;
    services.gpio = &gpio;
    services.interrupt = &interrupt;
    services.crypto = &crypto;
    services.comm = &comm;
    
    comm.init();
    
    GridShieldSystem system;
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    
    system.initialize(config, services);
    system.start();
    
    // Your code here
    
    system.shutdown();
    return 0;
}
```

---

## Support

- **Documentation:** `docs/` folder
- **Examples:** Check `examples/` directory (to be added)
- **Issues:** Report bugs on GitHub
- **Contact:** zuudevs@gmail.com

---

**Ready to secure your AMI system?** Start with Example 1 and customize from there!

**Institut Teknologi PLN - 2025**