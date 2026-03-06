# GridShield — Ideation & Technology Justification

**Version:** 2.0.0  
**Last Updated:** February 2026  
**Authors:** M. Ichwan Fauzi, Rafi Indra Pramudhito Zuhayr, Cesar Ardika Bhayangkara  
**Implementasi Aktual:** Seluruh implementasi teknis di repository ini dikerjakan oleh **Rafi Indra Pramudhito Zuhayr**

---

## 1. Ide Awal & Latar Belakang Masalah

### Masalah Inti
Pencurian listrik di Indonesia menyebabkan **Non-Technical Losses (NTL)** yang signifikan bagi PLN. Metode pencurian semakin canggih:
- **Fisik:** Buka casing meter, bypass terminal, ganti komponen internal
- **Digital:** Injeksi data palsu (*False Data Injection*) ke jalur komunikasi
- **Hybrid:** Kombinasi manipulasi fisik + digital yang sulit dideteksi satu layer saja

### Insight Kunci
> Tidak ada single-point security yang cukup. Solusi harus multi-layer agar serangan di satu layer tetap terdeteksi oleh layer lain.

---

## 2. Alternatif yang Dipertimbangkan

### Mengapa Multi-Layer (3 Layer)?

| Pendekatan | Pro | Kontra | Keputusan |
|---|---|---|---|
| Single Layer (Fisik saja) | Murah, sederhana | Tidak mendeteksi serangan digital | ❌ Ditolak |
| Dual Layer (Fisik + Crypto) | Cukup aman | Manipulasi data halus lolos | ❌ Kurang |
| **Triple Layer (Fisik + Crypto + Anomali)** | Defense-in-Depth lengkap | Lebih kompleks | ✅ **Dipilih** |

### Mengapa ECC, Bukan RSA?

| Kriteria | RSA-2048 | ECC-256 (secp256r1) |
|---|---|---|
| Key Size | 256 bytes | 32 bytes |
| Signature Size | 256 bytes | 64 bytes |
| Security Level | 112 bit | 128 bit |
| RAM di ESP32 | ~4 KB | ~1 KB |
| Kecepatan Sign (ESP32) | ~500ms | ~60ms |
| **Cocok IoT?** | ❌ Terlalu berat | ✅ **Ideal** |

### Mengapa C++17, Bukan Rust/MicroPython?

| Bahasa | Pro | Kontra | Keputusan |
|---|---|---|---|
| **C++17** | Zero-cost abstraction, `constexpr`, mature embedded support | Lebih verbose | ✅ **Dipilih** |
| Rust | Memory safety, modern syntax | Ecosystem embedded belum mature, learning curve tinggi | ❌ Ditolak |
| MicroPython | Cepat prototipe | Terlalu lambat untuk crypto real-time, GC unpredictable | ❌ Ditolak |

### Mengapa ESP32, Bukan Arduino Mega/STM32?

| Platform | Clock | RAM | Flash | Crypto HW | WiFi | Harga | Keputusan |
|---|---|---|---|---|---|---|---|
| Arduino Mega | 16 MHz | 8 KB | 256 KB | ❌ | ❌ | Rp 150K | ❌ Terlalu lambat |
| **ESP32** | 240 MHz | 520 KB | 4 MB | ✅ SHA/AES | ✅ Built-in | Rp 60K | ✅ **Dipilih** |
| STM32F4 | 168 MHz | 192 KB | 1 MB | ✅ Partial | ❌ | Rp 120K | 🔄 Future |

**Justifikasi:** ESP32 paling cost-effective, punya hardware acceleration untuk crypto, WiFi built-in untuk koneksi ke server, dan bisa disimulasikan via **Wokwi** tanpa perlu hardware fisik saat development.

---

## 3. Arsitektur 3-Layer — Mapping ke Tim

### Layer → Core Competency Mapping

```
┌─────────────────────────────────────────────────────┐
│  Layer 3: Anomaly Detection                         │
│  PIC: Ichwan (Data Scientist 11%, Server 5%)        │
│  • Statistik deviasi konsumsi                       │
│  • Profil 24-jam per pelanggan                      │
│  • Cross-layer validation                           │
├─────────────────────────────────────────────────────┤
│  Layer 2: Network Security                          │
│  PIC: Rafi (Cybersecurity 6%, SW Engineer 14%)      │
│  • ECDSA signing (micro-ecc)                        │
│  • SHA-256 hashing                                  │
│  • Secure packet protocol                           │
├─────────────────────────────────────────────────────┤
│  Layer 1: Physical Security                         │
│  PIC: Cesar (Electrical 6%, PCB 9%) + Rafi (FW 8%) │
│  • Tamper sensor (limit switch)                     │
│  • ISR debounce handler                             │
│  • Power-loss detection                             │
└─────────────────────────────────────────────────────┘
```

---

## 4. POC Scope — Apa yang Masuk & Tidak

### ✅ Masuk POC (Realistis untuk 3 Orang)

| Fitur | PIC | Deliverable |
|---|---|---|
| Firmware 3-layer security | Rafi | C++17 pada ESP32 via PlatformIO |
| Tamper detection (limit switch) | Cesar + Rafi | GPIO interrupt + ISR handler |
| ECC signing + SHA-256 | Rafi | micro-ecc + rweather/Crypto |
| Anomaly detection (statistik) | Ichwan | Deviasi threshold per jam |
| Backend sederhana | Ichwan | Python + FastAPI + SQLite |
| Dashboard monitoring | Ichwan | Web sederhana (status meter) |
| Simulasi hardware | All | Wokwi simulator via PlatformIO |

### ❌ TIDAK Masuk POC (Future Work)

| Fitur | Alasan |
|---|---|
| AES-GCM encryption | Kompleksitas tinggi, ECC signing sudah cukup untuk POC |
| MQTT broker (Mosquitto) | Serial/HTTP cukup untuk demo |
| Machine Learning | Statistik threshold sudah cukup untuk POC |
| Multi-platform (STM32, Arduino) | Fokus ESP32 dulu |
| Cloud deployment (AWS/Azure) | Localhost cukup untuk demo |
| TLS/mTLS | Overhead untuk POC |
| Secure element (ATECC608) | Hardware tambahan tidak diperlukan untuk POC |

---

## 5. Strategi Simulasi & Testing

### PlatformIO + Wokwi

```
Development Flow:
  VSCode + PlatformIO Extension
       │
       ├── Build: pio run -e esp32
       ├── Simulate: Wokwi (diagram.json)
       └── Upload: pio run -e esp32 -t upload (hardware fisik)
```

**Keuntungan Wokwi:**
- Tidak perlu beli ESP32 fisik untuk development awal
- Bisa simulasi GPIO, interrupt, serial monitor
- Integrasi langsung dengan PlatformIO
- Tim bisa develop secara paralel tanpa hardware sharing

---

## 6. Risiko & Mitigasi

| Risiko | Probabilitas | Mitigasi |
|---|---|---|
| Crypto terlalu berat di ESP32 | Rendah | ESP32 punya HW acceleration |
| Wokwi tidak support semua fitur | Sedang | Fallback ke hardware fisik |
| 3 orang tidak cukup | Sedang | POC scope sudah dipangkas |
| Interoperabilitas layer | Sedang | Integration testing di Week 4 |

---

**Document Information:**
- **Version:** 2.0.0
- **Last Updated:** February 2026
- **Purpose:** Ideation & Technology Justification (bukan Proposal)
- **Language:** Indonesian