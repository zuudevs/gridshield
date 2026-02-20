# GridShield Examples

This directory contains example projects demonstrating GridShield usage on different platforms.

---

## Available Examples

### 1. Arduino - SimpleMeter

**Location:** `arduino/SimpleMeter/SimpleMeter.ino`

A basic example showing GridShield integration with Arduino Mega 2560.

**Features Demonstrated:**
- System initialization with platform services
- Configuration of meter ID and heartbeat intervals
- Tamper detection setup (Pin 2)
- Simulated meter readings every 5 seconds
- Secure packet transmission

**Hardware Requirements:**
- Arduino Mega 2560
- Built-in LED on Pin 13
- Optional: Tamper switch on Pin 2

**Upload Instructions:**

```bash
# Using Arduino CLI
arduino-cli compile --fqbn arduino:avr:mega examples/arduino/SimpleMeter/SimpleMeter.ino
arduino-cli upload -p COM3 --fqbn arduino:avr:mega examples/arduino/SimpleMeter/SimpleMeter.ino

# Using PlatformIO
pio run -e megaatmega2560 --target upload
```

**Serial Output:**
```
SimpleMeter Example Starting...
GridShield Initialized.
Sending Reading...
Sending Reading...
```

---

### 2. Native - Demo

**Location:** `native/demo_native.cpp`

A comprehensive demo showcasing all GridShield features on PC (Native platform).

**Features Demonstrated:**
- Full system initialization
- ECC key pair generation
- Digital signature creation and verification
- AES-GCM encryption/decryption
- Meter reading simulation
- Tamper event simulation
- Anomaly detection
- Secure packet assembly

**Build Instructions:**

```bash
# Using CMake
cmake --preset native-release
cmake --build --preset native-release
./bin/NATIVE/GridShield

# Using PlatformIO
pio run -e native
.pio/build/native/program
```

**Sample Output:**
```
[SYSTEM INIT]
──────────────────────────────────────────────────
Initializing GridShield...
System initialized successfully.

[CRYPTO TEST]
──────────────────────────────────────────────────
Generating ECC key pair...
Key pair generated.
Signing message...
Signature verified: OK
```

---

## Creating New Examples

### Arduino Example Template

```cpp
#include <GridShield.h>
#include <arduino/platform_arduino.hpp>

using namespace gridshield;

// Platform implementations
static platform::arduino::ArduinoTime arduino_time;
static platform::arduino::ArduinoGPIO arduino_gpio;
static platform::arduino::ArduinoInterrupt arduino_interrupt;
static platform::arduino::ArduinoCrypto arduino_crypto;
static platform::arduino::ArduinoStorage arduino_storage;
static platform::arduino::ArduinoSerial arduino_serial;

static platform::PlatformServices services;
static GridShieldSystem* system_ptr = nullptr;

void setup() {
    // Setup platform services
    services.time = &arduino_time;
    services.gpio = &arduino_gpio;
    services.interrupt = &arduino_interrupt;
    services.crypto = &arduino_crypto;
    services.storage = &arduino_storage;
    services.comm = &arduino_serial;
    
    arduino_serial.init();
    
    // Configure system
    SystemConfig config;
    config.meter_id = 0x1122334455667788;
    config.heartbeat_interval_ms = 60000;
    config.reading_interval_ms = 5000;
    
    // Initialize
    system_ptr = new GridShieldSystem();
    system_ptr->initialize(config, services);
    system_ptr->start();
}

void loop() {
    if (system_ptr) {
        system_ptr->process_cycle();
    }
    delay(10);
}
```

### Native Example Template

```cpp
#include "core/system.hpp"
#include "platform_native.hpp"
#include <iostream>

using namespace gridshield;

int main() {
    // Setup platform
    platform::native::NativeTime native_time;
    platform::native::NativeGPIO native_gpio;
    platform::native::NativeInterrupt native_interrupt;
    platform::native::NativeCrypto native_crypto;
    platform::native::NativeStorage native_storage;
    platform::native::NativeSerial native_serial;
    
    platform::PlatformServices services;
    services.time = &native_time;
    services.gpio = &native_gpio;
    services.interrupt = &native_interrupt;
    services.crypto = &native_crypto;
    services.storage = &native_storage;
    services.comm = &native_serial;
    
    // Configure
    SystemConfig config;
    config.meter_id = 0x1234567890ABCDEF;
    
    // Initialize and run
    GridShieldSystem system;
    auto result = system.initialize(config, services);
    if (result.is_error()) {
        std::cerr << "Init failed\n";
        return 1;
    }
    
    system.start();
    
    // Main loop
    for (int i = 0; i < 10; ++i) {
        system.process_cycle();
    }
    
    return 0;
}
```

---

## Related Documentation

- [QUICKSTART.md](../docs/QUICKSTART.md) - Getting started guide
- [API.md](../docs/API.md) - Complete API reference
- [BUILD.md](../BUILD.md) - Build instructions
