# DOKUMEN SPESIFIKASI KEBUTUHAN PERANGKAT LUNAK (SRS)
## Proyek: GridShield - Multi-layer Security Model for AMI
**Versi:** 1.0.0
**Status:** Draft Awal
**Analisis oleh:** Rafi

---

## 1. PENDAHULUAN
### 1.1 Tujuan
Dokumen ini merinci kebutuhan fungsional dan non-fungsional untuk **GridShield**, sebuah sistem keamanan berlapis untuk infrastruktur meteran pintar (AMI). Tujuannya adalah mencegah manipulasi data (tampering) dan serangan siber yang merugikan utilitas (PLN) dan pelanggan.

### 1.2 Lingkup Masalah
Sesuai proposal, masalah utama yang diselesaikan adalah:
1.  **Manipulasi Fisik:** Pembukaan casing meteran secara ilegal.
2.  **Injeksi Data Palsu:** Serangan siber pada jalur komunikasi untuk memanipulasi tagihan.
3.  **Keterbatasan Resource:** Perangkat keras Smart Meter (ESP32/STM32) memiliki daya dan memori terbatas, tidak kuat menjalankan enkripsi berat.

---

## 2. PENGGUNA & STAKEHOLDER (User Persona)
| Aktor | Deskripsi | Peran dalam Sistem |
| :--- | :--- | :--- |
| **Sistem Meter (Edge)** | Perangkat keras Smart Meter di pelanggan. | Mengirim data konsumsi & sinyal tamper. |
| **Server Pusat (HES)** | Head-End System di utilitas/PLN. | Menerima data, mendekripsi, & analisis anomali. |
| **Administrator/Operator** | Petugas P2TL atau IT PLN. | Memantau dashboard & menerima alert pencurian. |
| **Peretas/Pencuri (Threat)** | Aktor jahat. | Mencoba memanipulasi fisik atau jaringan (objek mitigasi). |

---

## 3. KEBUTUHAN FUNGSIONAL (Functional Requirements - FR)

### 3.1 Layer 1: Keamanan Fisik (Physical Security)
*Kode Referensi: FR-PHYS*
* **[FR-PHYS-01] Deteksi Tamper Fisik:** Sistem harus mampu mendeteksi jika casing meteran dibuka secara paksa menggunakan sensor terintegrasi.
* **[FR-PHYS-02] Priority Flagging:** Saat tamper terdeteksi, mikrokontroler harus segera mengirimkan sinyal "Priority Flag" ke server, mengabaikan antrian pengiriman data rutin.
* **[FR-PHYS-03] Power Backup Operation:** Modul keamanan harus tetap dapat mengirimkan sinyal tamper sesaat setelah daya utama diputus (menggunakan kapasitor/baterai cadangan).

### 3.2 Layer 2: Keamanan Jaringan (Network Security)
*Kode Referensi: FR-NET*
* **[FR-NET-01] Enkripsi Ringan (Lightweight Crypto):** Sistem harus mengimplementasikan algoritma ECC (Elliptic Curve Cryptography) untuk enkripsi data pengukuran sebelum dikirim.
    * *Catatan:* Harus dioptimalkan untuk C/C++ (Low-Level) agar hemat memori.
* **[FR-NET-02] Otentikasi Perangkat:** Server harus memvalidasi tanda tangan digital dari setiap paket data untuk memastikan data berasal dari meteran yang sah (Mencegah *Man-in-the-Middle*).
* **[FR-NET-03] Integritas Data:** Payload data harus memiliki mekanisme hashing untuk memastikan data tidak berubah selama transmisi.

### 3.3 Layer 3: Analisis Anomali (Application Layer)
*Kode Referensi: FR-APP*
* **[FR-APP-01] Penerimaan Data:** Server harus mampu mendekripsi data yang diterima dari ribuan meteran secara *concurrent*.
* **[FR-APP-02] Deteksi Anomali Beban:** Sistem harus membandingkan data *real-time* dengan pola historis pelanggan.
    * *Trigger:* Jika beban turun drastis (misal: 90%) tanpa alasan logis -> Flag Anomali.
* **[FR-APP-03] Cross-Layer Validation:** Sistem harus memverifikasi data fisik dengan pola digital.
* **[FR-APP-04] Dashboard Monitoring:** Menyediakan antarmuka visual untuk menampilkan status keamanan meteran (Hijau: Aman, Merah: Tamper/Anomali).

---

## 4. KEBUTUHAN NON-FUNGSIONAL (Non-Functional Requirements - NFR)

### 4.1 Efisiensi & Kinerja (Performance)
* **[NFR-PERF-01] Latensi Rendah:** Proses enkripsi/dekripsi di sisi meteran tidak boleh menghambat fungsi utama pengukuran listrik (> 100ms overhead dianggap gagal).
* **[NFR-PERF-02] Hemat Daya:** Algoritma keamanan tidak boleh menguras daya secara signifikan (Resource-constrained friendly).

### 4.2 Keamanan (Security)
* **[NFR-SEC-01] Confidentiality:** Data konsumsi listrik pelanggan tidak boleh bisa dibaca oleh pihak ketiga (sniffing).
* **[NFR-SEC-02] Availability:** Sistem keamanan tidak boleh menyebabkan meteran *hang* atau *crash* (Denial of Service).

### 4.3 Portabilitas (Portability)
* **[NFR-PORT-01] Hardware Agnostic:** Kode C/C++ untuk modul keamanan harus dapat di-porting ke berbagai jenis mikrokontroler (ESP32, STM32) dengan perubahan minimal.

---

## 5. BATASAN SISTEM (Constraints)
1.  **Bahasa Pemrograman:** Sisi firmware wajib menggunakan **C/C++** (Sesuai keahlian Rafi & kebutuhan hardware). Sisi Server/Analitik boleh menggunakan **Python**.
2.  **Hardware:** Prototype menggunakan ESP32/STM32.
3.  **Konektivitas:** Simulasi pengiriman data menggunakan protokol serial atau WiFi/LoRa (tergantung modul komunikasi yang tersedia).

---

## 6. RENCANA PENGUJIAN (Acceptance Criteria)
| ID Skenario | Deskripsi Pengujian | Hasil yang Diharapkan |
| :--- | :--- | :--- |
| **TEST-01** | Buka Casing Meteran | Alert muncul di dashboard server dalam waktu < 5 detik. |
| **TEST-02** | Sniffing Data Paket | Data yang ditangkap di jaringan tidak terbaca (ciphertext). |
| **TEST-03** | Injeksi Data Palsu | Server menolak data yang dimanipulasi & mencatat log serangan. |
| **TEST-04** | Drop Beban Ekstrem | Sistem menandai anomali saat penggunaan listrik turun drastis tiba-tiba. |