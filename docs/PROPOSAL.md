# PROPOSAL PROYEK
# GRIDSHIELD: MODEL KEAMANAN MULTI-LAYER UNTUK PERLINDUNGAN SISTEM AMI

**Disusun Oleh:**
* Muhammad Ichwan Fauzi (202331227)
* Rafi Indra Pramudhito Zuhayr (202331291)
* Cesar Ardika Bhayangkara (202311240)

**Institut Teknologi PLN - 2025**

---

## BAB I: RINGKASAN EKSEKUTIF

### 1.1 Latar Belakang dan Masalah
Modernisasi kelistrikan melalui *Advanced Metering Infrastructure* (AMI) adalah langkah strategis PLN. Namun, sistem ini membawa risiko keamanan siber baru. Masalah utama yang dihadapi adalah kerentanan terhadap manipulasi data pengukuran (*meter tampering*) baik secara fisik maupun melalui serangan siber (*cyber-attacks*) pada jalur komunikasi. Hal ini berpotensi menyebabkan kerugian finansial akibat pencurian listrik digital (*Non-Technical Losses*) dan ketidakakuratan data tagihan.

### 1.2 Solusi Teknologi
GridShield menawarkan arsitektur keamanan *Multi-Layer* khusus ekosistem AMI:
1.  **Keamanan Fisik:** Sensor tamper terintegrasi dengan mekanisme *Power-Loss Alert*.
2.  **Keamanan Jaringan:** Implementasi *Lightweight Cryptography* berbasis ECC (Elliptic Curve Cryptography) yang dioptimalkan menggunakan **Modern C++ (C++23)** untuk efisiensi memori pada *embedded system*.
3.  **Analitik Cerdas:** Deteksi anomali pola konsumsi secara *real-time* di sisi server.

### 1.3 Keunggulan Kompetitif
Berbeda dengan solusi vendor yang tertutup (*proprietary*), GridShield bersifat:
* **Resource-Constrained Friendly:** Algoritma dioptimalkan untuk mikrokontroler murah (ESP32/STM32).
* **Cross-Layer Validation:** Memverifikasi data fisik dengan pola digital.
* **Cost-Effective:** Biaya implementasi jauh lebih rendah dibandingkan lisensi keamanan vendor global.

---

## BAB II: IDENTITAS TIM DAN KUALIFIKASI

### 2.1 Komposisi Tim
* **Muhammad Ichwan Fauzi (Ketua/Network Security):** Fokus pada arsitektur sistem, *threat modeling*, dan integrasi protokol keamanan.
* **Rafi Indra Pramudhito Zuhayr (Software Engineer):** Spesialis *Low-Level Programming* (C/C++). Bertanggung jawab atas optimasi memori, implementasi algoritma kriptografi pada *firmware*, dan efisiensi kode sistem *embedded*.
* **Cesar Ardika Bhayangkara (Hardware Engineer):** Fokus pada perancangan rangkaian elektronika, integrasi sensor fisik, dan komunikasi *hardware* (UART/SPI/I2C).

---

## BAB III: ANALISIS MASALAH DAN SOLUSI

### 3.1 Deskripsi Masalah
* **Manipulasi Fisik Canggih:** Modifikasi komponen internal meteran.
* **False Data Injection (FDI):** Manipulasi paket data di tengah transmisi.
* **Keterbatasan Hardware:** Smart Meter memiliki CPU dan RAM kecil, tidak kuat menjalankan enkripsi standar PC (RSA/AES-256 berat).

### 3.2 Arsitektur Solusi (GridShield)
Pendekatan *Defense-in-Depth*:
* **Layer 1 (Physical):** Sensor tamper mendeteksi pembukaan casing -> Memicu *Priority Flag* -> Kirim sinyal darurat via kapasitor cadangan.
* **Layer 2 (Network - Firmware):** Menggunakan C++23 untuk manajemen memori manual yang ketat. Menggantikan RSA dengan ECC (Elliptic Curve) yang memiliki kunci lebih pendek namun setara keamanannya, cocok untuk *bandwidth* IoT yang sempit.
* **Layer 3 (Application):** *Anomaly Detection Engine* yang membandingkan beban *real-time* dengan profil historis pelanggan.

### 3.3 Tingkat Kesiapan Teknologi (TRL)
Saat ini berada pada **TRL 3 (Proof of Concept Experimental)**. Algoritma telah dipilih, arsitektur didefinisikan, dan simulasi kode awal sedang berjalan.

---

## BAB IV: MODEL BISNIS DAN PASAR

### 4.1 Target Pasar
* **B2G (PLN/Utilitas):** Unit P2TL dan Divisi AMI.
* **B2B (Kawasan Industri):** Pengelola kawasan yang memiliki pembangkit/distribusi mandiri.
* **OEM (Manufaktur):** Produsen meter lokal (TKDN) yang butuh modul keamanan tambahan.

### 4.2 Model Pendapatan
* **Lisensi Per-Perangkat:** One-time fee per meter yang diinstal GridShield.
* **SaaS Dashboard:** Biaya langganan bulanan untuk monitoring anomali.

---

## BAB V: RENCANA IMPLEMENTASI DAN KEBUTUHAN (REVISI)

### 5.1 Rencana Kerja (Sprint Program)
Target selama masa inkubasi:
* **Minggu 1 (Desain):** Finalisasi *Threat Modeling* dan skema *hardware*.
* **Minggu 2 (Dev - Security):** Coding algoritma ECC pada ESP32 menggunakan C++.
* **Minggu 3 (Dev - Analytic):** Pembuatan *backend* deteksi anomali (Python/Go).
* **Minggu 4 (Integrasi):** Penggabungan modul Hardware + Firmware + Cloud untuk Demo MVP.

### 5.2 Kebutuhan Dukungan
* **Mentoring:** Praktisi Cyber Security / IoT Industri.
* **Akses Data:** Dataset dummy profil beban listrik (anonim).
* **Fasilitas:** Akses Lab Sistem Tenaga/Komputer.

---

## BAB VI: ESTIMASI ANGGARAN BIAYA (RAB)

*(Bagian ini ditambahkan karena proposal proyek wajib memiliki estimasi dana)*

| No | Komponen | Deskripsi | Estimasi Biaya (Rp) |
| :--- | :--- | :--- | :--- |
| 1 | **Komponen Hardware** | ESP32 DevKit, Sensor (Hall/Limit Switch), PCB, Casing Dummy | 1.500.000 |
| 2 | **Cloud & Server** | Sewa VPS/Cloud untuk Backend (3 Bulan) | 600.000 |
| 3 | **Konektivitas** | Paket Data IoT / Modul LoRa | 400.000 |
| 4 | **Operasional Riset** | Transportasi, Cetak Laporan, Konsumsi Tim | 1.000.000 |
| **TOTAL** | | | **Rp 3.500.000** |

---

## BAB VII: PENUTUP

GridShield bukan hanya sekadar alat pengaman, melainkan solusi strategis untuk menyelamatkan pendapatan negara dari pencurian listrik modern. Dengan tim yang menggabungkan keahlian *hardware*, *software engineering* tingkat rendah (C++), dan keamanan siber, kami yakin dapat menghadirkan purwarupa yang valid dan siap dikembangkan lebih lanjut.