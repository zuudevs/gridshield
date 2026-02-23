# GridShield - Command Cheat Sheet

## Build & Run (Automation Script)

```powershell
.\scripts\script.ps1 --build       # Build firmware
.\scripts\script.ps1 --run         # Build + Run QEMU (IDF Monitor)
.\scripts\script.ps1 --run-raw     # Build + Run QEMU (raw console)
.\scripts\script.ps1 --debug       # Build + Run QEMU with GDB server
.\scripts\script.ps1 --gdb         # Attach GDB (second terminal)
.\scripts\script.ps1 --clean       # Full clean
.\scripts\script.ps1 --setup       # Install QEMU (one-time)
.\scripts\script.ps1 --env         # Open ESP-IDF shell
```

## ESP-IDF Commands (Manual)

```bash
cd firmware

idf.py set-target esp32     # Set target (one-time)
idf.py build                # Build firmware
idf.py fullclean            # Clean all build artifacts
idf.py qemu monitor         # Run in QEMU + IDF Monitor
idf.py qemu                 # Run in QEMU (raw)
idf.py qemu --gdb monitor   # QEMU + GDB server
idf.py gdb                  # Attach GDB
idf.py menuconfig           # Open config UI
```

## QEMU Shortcuts

- **Exit IDF Monitor:** `Ctrl+]`
- **QEMU Console:** `Ctrl+A` then `C`
- **Exit QEMU:** `Ctrl+A` then `Q`

## VSCode Shortcuts

- `Ctrl+Shift+P` → "ESP-IDF: Build"
- `Ctrl+Shift+P` → "ESP-IDF: Flash"
- `Ctrl+Shift+P` → "ESP-IDF: Monitor"