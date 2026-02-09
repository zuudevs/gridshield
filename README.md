# GridShield - AMI Security System

Multi-layer security for Advanced Metering Infrastructure (AMI).

## Tech Stack
- **Language:** C++17
- **Build:** CMake 3.20+
- **Platforms:** Native (x86/x64), Arduino AVR

## Quick Build

### Native (Development)
```bash
cmake --preset native-debug && cmake --build --preset native-debug
./bin/NATIVE/GridShield
```

### Arduino (Production)
```bash
arduino-cli compile --fqbn arduino:avr:mega src/arduino/gridshield.ino
arduino-cli upload -p COM3 --fqbn arduino:avr:mega src/arduino/gridshield.ino
```

## Architecture

```
┌─────────────────────────────┐
│   GridShield System         │ ← Orchestrator
└──────────┬──────────────────┘
           │
    ┌──────┼──────┐
    ▼      ▼      ▼
  Physical Network Analytics
  Security Security Detection
```

### Layers
1. **Physical:** Tamper detection, power-loss alert
2. **Network:** ECC crypto, secure packets (ECDSA)
3. **Analytics:** Consumption anomaly detection

## Project Structure
```
gridshield/
├── CMakeLists.txt           # Build config
├── include/
│   ├── common/              # Platform-agnostic
│   │   ├── utils/           # Macros (C++17)
│   │   ├── core/            # Error, types, system
│   │   ├── security/        # Crypto engine
│   │   ├── hardware/        # Tamper detector
│   │   ├── network/         # Secure packets
│   │   └── analytics/       # Anomaly detection
│   ├── platform/            # HAL interfaces
│   ├── native/              # PC implementation
│   └── arduino/             # AVR implementation
├── src/
│   ├── common/              # Implementations
│   ├── native/main.cpp      # PC entry
│   └── arduino/gridshield.ino  # Arduino entry
└── CMakePresets.json        # Build presets
```

## Key Features

### C++17 Compatibility
- Result<T> monad (no exceptions)
- Custom move semantics for AVR
- Zero heap allocation (embedded-friendly)

### Memory Optimized
- Flash: ~35 KB core, ~85 KB with crypto
- RAM: ~3.8 KB static + stack
- Target: ATmega2560 (256KB/8KB)

### Security
- ECC secp256r1 (placeholder, use uECC in production)
- ECDSA signatures
- SHA256 integrity
- Cross-layer validation

## Configuration

### Native Debug
```bash
cmake --preset native-debug
```
- Sanitizers: ON (Linux/macOS)
- Optimization: -O0

### Native Release
```bash
cmake --preset native-release
```
- Sanitizers: OFF
- Optimization: -O3

## Production Deployment

### 1. Install Libraries
```bash
arduino-cli lib install Crypto  # SHA256
```

### 2. Replace Placeholders
Uncomment production crypto in:
- `src/common/security/crypto.cpp`
- `include/arduino/platform_arduino.hpp`

### 3. Hardware Setup
- Arduino Mega 2560
- Tamper switch: Pin 2
- Serial: 115200 baud

## Troubleshooting

| Error | Fix |
|-------|-----|
| `gs_macros.hpp not found` | Check include paths |
| `undefined std::move` | Add `-std=c++17` |
| `Sketch too big` | Use Mega, NOT Uno |
| `Upload failed` | Run `arduino-cli board list` |

## Documentation
- **BUILD.md:** Detailed build instructions
- **ARCHITECTURE.md:** System design
- **PROPOSAL.md:** Project rationale

## License
MIT

## Authors
- Muhammad Ichwan Fauzi (202331227) - Architecture
- Rafi Indra Pramudhito Zuhayr (202331291) - Implementation  
- Cesar Ardika Bhayangkara (202311240) - Hardware

Institut Teknologi PLN - 2025