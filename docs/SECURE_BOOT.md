# Secure Boot v2 & Flash Encryption Guide

> **Status:** Documentation only — runtime code does not need changes.  
> Enabling happens at flash time on real hardware.

---

## Prerequisites

- ESP-IDF v5.5+
- `espsecure.py` (included with ESP-IDF)
- Physical ESP32 device (cannot test on QEMU)

---

## 1. Secure Boot v2

Secure Boot v2 uses RSA-3072 or ECDSA to verify firmware signatures during boot.

### Generate Signing Key

```bash
# Generate RSA-3072 signing key (one-time, keep secure!)
espsecure.py generate_signing_key --version 2 signing_key.pem

# Backup the key — if lost, device cannot be updated
cp signing_key.pem /secure-backup/gridshield_signing_key.pem
```

### Enable via menuconfig

```bash
idf.py menuconfig
```

Navigate to:
```
Security features →
  [*] Enable hardware Secure Boot in bootloader (v2)
  (signing_key.pem) Secure boot private signing key
  [*] Sign binaries during build
```

### Build & Flash

```bash
idf.py build
idf.py -p COM3 flash
```

> [!CAUTION]
> Once Secure Boot is enabled and eFuses are burned, **it cannot be disabled**.
> The device will only boot firmware signed with your key.

---

## 2. Flash Encryption

Flash Encryption protects firmware and data stored in flash from being read externally.

### Enable via menuconfig

```bash
idf.py menuconfig
```

Navigate to:
```
Security features →
  [*] Enable flash encryption on boot
  (Development) Flash encryption mode
      Development — allows re-flashing (limited burns)
      Release — permanent (production)
```

### Development Mode Workflow

```bash
# First flash — ESP32 generates encryption key and burns eFuse
idf.py -p COM3 flash

# Subsequent flashes — encrypt locally before flashing
idf.py -p COM3 encrypted-flash monitor
```

> [!WARNING]
> Development mode allows **only 3 re-flash cycles** before eFuses are exhausted.
> Use Release mode only for production devices.

---

## 3. Combined Setup (Recommended for Production)

1. Generate signing key
2. Enable Secure Boot v2 in menuconfig
3. Enable Flash Encryption (Release mode) in menuconfig
4. Build and flash once
5. Verify boot log shows:
   ```
   I (123) boot: Secure boot V2 enabled
   I (456) flash_encrypt: Flash encryption mode: RELEASE
   ```

---

## 4. NVS Encryption

GridShield stores keys and config in NVS. Enable NVS encryption:

```
Component config →
  NVS →
    [*] Enable NVS encryption
```

This uses the flash encryption key to encrypt NVS partitions automatically.

---

## 5. sdkconfig.defaults

Add these commented options to `firmware/sdkconfig.defaults` when ready:

```ini
# === Security: Secure Boot v2 ===
# CONFIG_SECURE_BOOT=y
# CONFIG_SECURE_BOOT_V2_ENABLED=y
# CONFIG_SECURE_BOOT_SIGNING_KEY="signing_key.pem"
# CONFIG_SECURE_SIGNED_ON_BOOT=y
# CONFIG_SECURE_SIGNED_ON_UPDATE=y

# === Security: Flash Encryption ===
# CONFIG_SECURE_FLASH_ENC_ENABLED=y
# CONFIG_SECURE_FLASH_ENCRYPTION_MODE_DEVELOPMENT=y

# === Security: NVS Encryption ===
# CONFIG_NVS_ENCRYPTION=y
```

---

## 6. Recovery Procedures

### Lost Signing Key
- **No recovery** — device requires factory reset with new eFuse (if available)
- Always maintain secure backups of `signing_key.pem`

### Flash Encryption Key Rotation
- Only possible in Development mode
- Use `espefuse.py` to check remaining burn attempts:
  ```bash
  espefuse.py -p COM3 summary
  ```

### Factory Reset
```bash
# Erase all flash (only works if flash encryption not in Release mode)
esptool.py -p COM3 erase_flash
```
