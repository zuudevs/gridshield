# RENCANA ANGGARAN BIAYA (RAB)
# GridShield — Multi-Layer Security Model for AMI

**Version:** 1.0.0  
**Last Updated:** Maret 2026  
**Tim:** Rafi, Ichwan, Cesar  
**Institusi:** Institut Teknologi PLN

---

## Ringkasan Proyek

GridShield adalah sistem keamanan multi-layer untuk Advanced Metering Infrastructure (AMI) yang mencakup:
- **Firmware** — C++17 pada ESP32 (ECC, tamper detection, anomaly detection)
- **Backend** — Python + FastAPI + SQLite
- **Frontend** — Vite + Chart.js dashboard
- **Hardware** — ESP32, sensor, PCB modul keamanan

---

## 1. Komponen Hardware

| No | Item | Spesifikasi | Qty | Harga Satuan (Rp) | Total (Rp) |
|:---:|:---|:---|:---:|---:|---:|
| 1.1 | ESP32 DevKit V1 | Xtensa LX6, 240 MHz, 4MB Flash, WiFi/BT | 2 | 75,000 | 150,000 |
| 1.2 | Limit Switch (Tamper Sensor) | Sensor buka casing, lever-type | 3 | 10,000 | 30,000 |
| 1.3 | Hall Effect Sensor (ACS712) | Sensor arus 5A/20A/30A | 2 | 25,000 | 50,000 |
| 1.4 | Voltage Sensor (ZMPT101B) | Sensor tegangan AC 220V | 2 | 30,000 | 60,000 |
| 1.5 | Temperature Sensor (DS18B20) | Sensor suhu digital, waterproof | 2 | 15,000 | 30,000 |
| 1.6 | Supercapacitor | 5.5V 1F, backup power saat mati listrik | 2 | 20,000 | 40,000 |
| 1.7 | Breadboard | 830 tie-point, full-size | 2 | 25,000 | 50,000 |
| 1.8 | Jumper Wire Kit | Male-Male, Male-Female, Female-Female | 3 set | 15,000 | 45,000 |
| 1.9 | Resistor Kit | 10Ω–1MΩ, 1/4W | 1 set | 20,000 | 20,000 |
| 1.10 | LED Indikator | Hijau, Merah, Kuning (indikator status) | 1 set | 5,000 | 5,000 |
| 1.11 | Push Button | Reset/test, tactile switch | 5 | 2,000 | 10,000 |
| 1.12 | Kabel USB Micro | Kabel data ESP32 | 2 | 15,000 | 30,000 |
| 1.13 | Power Adapter 5V/2A | Catu daya ESP32 | 2 | 25,000 | 50,000 |
| | | | | **Subtotal Hardware** | **570,000** |

---

## 2. PCB & Casing (Modul Keamanan)

| No | Item | Spesifikasi | Qty | Harga Satuan (Rp) | Total (Rp) |
|:---:|:---|:---|:---:|---:|---:|
| 2.1 | PCB Custom (Cetak) | 2-layer, 10×8 cm, jasa cetak PCB lokal | 5 | 30,000 | 150,000 |
| 2.2 | Solder Station Kit | Soldering iron + tin solder + flux | 1 set | 100,000 | 100,000 |
| 2.3 | Casing/Enclosure Meter | Dummy casing berbahan akrilik/3D print | 1 | 150,000 | 150,000 |
| 2.4 | Konektor & Terminal Block | Screw terminal, header pin | 1 set | 25,000 | 25,000 |
| | | | | **Subtotal PCB & Casing** | **425,000** |

---

## 3. Cloud & Server

| No | Item | Spesifikasi | Qty | Harga Satuan (Rp) | Total (Rp) |
|:---:|:---|:---|:---:|---:|---:|
| 3.1 | VPS Cloud (Backend) | 1 vCPU, 1GB RAM, 20GB SSD (IDCloudHost/DigitalOcean) | 3 bulan | 100,000/bln | 300,000 |
| 3.2 | Domain Name | `.com` atau `.id` untuk demo | 1 tahun | 150,000 | 150,000 |
| 3.3 | SSL Certificate | Let's Encrypt (gratis) | — | 0 | 0 |
| | | | | **Subtotal Cloud** | **450,000** |

---

## 4. Software & Development Tools

| No | Item | Spesifikasi | Biaya (Rp) |
|:---:|:---|:---|---:|
| 4.1 | ESP-IDF v5.5 | Framework resmi ESP32 (open-source) | 0 |
| 4.2 | QEMU Simulator | Emulasi ESP32 (open-source) | 0 |
| 4.3 | Python 3.11+ | Backend runtime (open-source) | 0 |
| 4.4 | FastAPI + Uvicorn | Web framework (open-source) | 0 |
| 4.5 | SQLite | Database (open-source) | 0 |
| 4.6 | Vite + Chart.js | Frontend build tool + charting (open-source) | 0 |
| 4.7 | micro-ecc | Library kriptografi ECC (open-source) | 0 |
| 4.8 | VSCode + ESP-IDF Extension | IDE (free) | 0 |
| 4.9 | Git + GitHub | Version control + hosting (free tier) | 0 |
| 4.10 | GitHub Actions CI/CD | Free tier (2000 min/bulan) | 0 |
| 4.11 | clang-format + clang-tidy | Code quality tools (open-source) | 0 |
| | | **Subtotal Software** | **0** |

> [!NOTE]
> Seluruh stack software menggunakan open-source/free tier sehingga tidak ada biaya lisensi.

---

## 5. Operasional & Penelitian

| No | Item | Spesifikasi | Qty | Harga Satuan (Rp) | Total (Rp) |
|:---:|:---|:---|:---:|---:|---:|
| 5.1 | Transportasi | Pertemuan tim, lab visit, bimbingan | 12 kali | 30,000 | 360,000 |
| 5.2 | Cetak Laporan & Proposal | Hardcopy + jilid | 3 set | 50,000 | 150,000 |
| 5.3 | Konsumsi Tim | Meeting, kerja bersama | 12 kali | 30,000 | 360,000 |
| 5.4 | Internet / Data | Kuota internet untuk development | 3 bulan | 50,000/bln | 150,000 |
| | | | | **Subtotal Operasional** | **1,020,000** |

---

## 6. Testing & Demo

| No | Item | Spesifikasi | Qty | Harga Satuan (Rp) | Total (Rp) |
|:---:|:---|:---|:---:|---:|---:|
| 6.1 | Logic Analyzer | USB logic analyzer 8-channel (debugging) | 1 | 80,000 | 80,000 |
| 6.2 | Multimeter Digital | Pengukuran tegangan/arus untuk validasi | 1 | 100,000 | 100,000 |
| 6.3 | Power Bank | Pengujian portabel di lapangan | 1 | 75,000 | 75,000 |
| | | | | **Subtotal Testing** | **255,000** |

---

## REKAPITULASI TOTAL

| No | Kategori | Total (Rp) |
|:---:|:---|---:|
| 1 | Komponen Hardware | 570,000 |
| 2 | PCB & Casing | 425,000 |
| 3 | Cloud & Server | 450,000 |
| 4 | Software & Development Tools | 0 |
| 5 | Operasional & Penelitian | 1,020,000 |
| 6 | Testing & Demo | 255,000 |
| | **GRAND TOTAL** | **Rp 2,720,000** |

---

## Catatan

1. **Harga bersifat estimasi** berdasarkan harga pasar Maret 2026 (Tokopedia/Shopee/lokal).
2. **Contingency 10%** disarankan → Total + contingency = **Rp 2,992,000 ≈ Rp 3,000,000**.
3. Beberapa komponen (multimeter, solder station) bisa dipinjam dari lab jika tersedia, sehingga biaya bisa lebih rendah.
4. Biaya cloud bisa dikurangi jika menggunakan laptop sebagai server lokal saat demo.
5. Seluruh software yang digunakan bersifat **open-source / free tier**, sehingga zero software cost.

---

## Perbandingan dengan RAB di Proposal

| Item | RAB Proposal (v2.0) | RAB Detail Ini |
|:---|---:|---:|
| Hardware Components | 1,200,000 | 995,000 |
| Cloud & Server | 600,000 | 450,000 |
| Connectivity | 0 | 0 |
| Development Tools | 0 | 0 |
| Research Operations | 1,000,000 | 1,275,000 |
| **TOTAL** | **2,800,000** | **2,720,000** |

> [!TIP]
> RAB ini lebih detail dari versi proposal dan menunjukkan bahwa estimasi awal **Rp 2,800,000** cukup akurat. Selisih ~Rp 80,000 karena rincian per-komponen yang lebih presisi.

---

**Document Version:** 1.0.0  
**Created:** Maret 2026  
**Author:** GridShield Team
