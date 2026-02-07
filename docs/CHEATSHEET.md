# GridShield - Command Cheat Sheet

## Quick Build Commands

```bash
# Native (PC Testing)
cmake --preset native-debug && cmake --build --preset native-debug
./bin/NATIVE/GridShield

# Arduino Mega (Production)
cmake --preset avr-mega && cmake --build --preset avr-mega

# Upload to Mega
cmake --build build/avr-mega --target upload -DAVR_UPLOAD_PORT=COM3
```

## List Available Presets
```bash
cmake --list-presets
```

## Clean Build
```bash
rm -rf build/
```

## Serial Monitor
```bash
arduino-cli monitor -p COM3 -b 115200
```

## Memory Check (after build)
```bash
avr-size --format=avr --mcu=atmega2560 bin/AVR/GridShield.elf
```

## Manual Upload
```bash
avrdude -p atmega2560 -c wiring -P COM3 -b 115200 \
        -U flash:w:bin/AVR/GridShield.hex:i
```

## VSCode Shortcuts
- `F7` - Build
- `Ctrl+Shift+P` → "CMake: Select Configure Preset"

## Preset Names
- `native-debug` - PC with sanitizers
- `native-release` - PC optimized
- `avr-uno` - Arduino Uno (32KB flash)
- `avr-mega` - Arduino Mega (256KB flash) ⭐