# GridShield v3.3.1 — Panduan Wiring

## Daftar Komponen

| # | Komponen | Spesifikasi | Fungsi |
|---|----------|-------------|--------|
| 1 | **ESP32 DevKit V1** | Xtensa LX6 Dual-Core 240MHz | Mikrokontroler utama |
| 2 | **PZEM-004T v3.0** | Energy Meter Modbus RTU | Ukur V, I, P, E, Hz, PF |
| 3 | **PZKHCT** | CT Clamp (Current Transformer) | Sensor arus untuk PZEM |
| 4 | **DHT11** | Sensor Suhu & Kelembaban | Monitor suhu lingkungan |
| 5 | **JQC-3FF-S-Z** | Relay 10A 250VAC | Kontrol ON/OFF beban AC |
| 6 | **MCB IC60N** | Miniature Circuit Breaker | Proteksi arus lebih |
| 7 | **MEAN WELL IRM-10-3.3** | AC-DC PSU 3.3V 3A (10W) | Catu daya utama dari AC 220V |
| 8 | **TP4056** | Li-ion Charger Module | Charging baterai dari IRM |
| 9 | **MT3608** | DC-DC Boost Converter | Boost 3.3V → 5V untuk TP4056 |
| 10 | **Li-ion 18650** | 2500mAh 3.7V | Baterai cadangan (UPS) |
| 11 | **Buzzer Aktif** | Piezo Buzzer 3.3V | Alarm suara (tamper/overheat) |
| 12 | **Tamper Switch** | Switch + Pull-up | Deteksi pembukaan casing |
| 13 | **Terminal Block** | Screw Terminal | Koneksi kabel AC 220V |
| 14 | **Breadboard** | — | Prototyping koneksi sensor |

---

## Arsitektur Daya (Power Architecture)

ESP32 **selalu** powered dari baterai melalui TP4056. Saat AC hidup, IRM-10-3.3 mengisi baterai. Saat AC mati, baterai tetap menyuplai ESP32 → kirim alert **POWER LOSS**.

```
                                    ┌─────────────────────────┐
                                    │   MEAN WELL IRM-10-3.3  │
PLN 220V AC ──► MCB IC60N ──┬──────►│ AC IN (L,N)             │
                            │       │              DC OUT 3.3V ├──┐
                            │       │              GND ────────├──┤
                            │       └─────────────────────────┘  │
                            │                                    │
                            │       ┌──────────────┐             │
                            │       │   MT3608     │◄────────────┘
                            │       │ IN 3.3V      │    (boost 3.3V → 5V)
                            │       │ OUT 5V ──────├──┐
                            │       └──────────────┘  │
                            │                         │
                            │       ┌─────────────────┴──────┐
                            │       │      TP4056            │
                            │       │  IN+ (5V)              │
                            │       │          BAT+/BAT- ────├── Li-ion 2500mAh
                            │       │  OUT+ ─────────────────├──► ESP32 VIN
                            │       │  OUT- ─────────────────├──► ESP32 GND
                            │       └────────────────────────┘
                            │
                            ├──────► PZEM-004T (AC L,N + CT Clamp)
                            │
                            └──────► Relay COM ──► Beban AC ──► Neutral
```

### Alur Daya

| Kondisi | Alur | ESP32 Status |
|---------|------|--------------|
| **AC ON** | PLN → MCB → IRM → MT3608 → TP4056 → charge battery + power ESP32 | **Hidup** (dari battery+charger) |
| **AC OFF** | Battery → TP4056 OUT → ESP32 VIN | **Tetap hidup!** → kirim alert |
| **Battery habis** | Tidak ada sumber daya | ESP32 mati |

### Estimasi Backup Time

| Parameter | Nilai |
|-----------|-------|
| Kapasitas baterai | 2500 mAh |
| Konsumsi ESP32 (WiFi aktif) | ~200 mA |
| Efisiensi AMS1117 | ~85% |
| **Estimasi backup** | **~10 jam** |

> ⚠️ **TP4056 output = tegangan baterai (3.0–4.2V).** Masuk ke ESP32 VIN, lalu diturunkan oleh AMS1117 onboard ke 3.3V. Saat baterai di bawah ~3.5V, WiFi mungkin tidak stabil.

> ⚠️ **Saat AMS1117 dropout**: Pada arus rendah (~200mA), dropout AMS1117 sekitar 0.5V. Jadi baterai 3.7V → output 3.2V (masih di atas minimal ESP32: 3.0V).

### Catatan Power Supply

| Komponen | Sumber Daya | Tegangan |
|----------|-------------|----------|
| **ESP32 DevKit V1** | TP4056 OUT+ → pin **VIN** | 3.0–4.2V → AMS1117 → 3.3V |
| **PZEM-004T** | Langsung dari AC 220V (terminal L, N) | 220V AC |
| **Relay JQC-3FF** | ESP32 **3V3** pin output | 3.3V DC* |
| **DHT11** | ESP32 pin **3V3** | 3.3V DC |
| **Buzzer** | ESP32 pin **GPIO25** (PWM) | 3.3V |
| **MT3608** | IRM-10-3.3 DC OUT | 3.3V IN → 5V OUT |
| **TP4056** | MT3608 OUT (5V) | 5V IN → charges Li-ion |

> ⚠️ **JQC-3FF-S-Z biasanya versi 5V.** Jika relay kamu versi 5V, kamu perlu power USB terpisah ke VIN. Jika versi 3.3V, bisa langsung dari ESP32 3V3. Cek label di relay: **"SRA-05VDC" = 5V**, **"SRA-03V3DC" = 3.3V**.

---

## Pin Mapping — ESP32 DevKit V1

```
                    ┌──────────────────────┐
                    │    ESP32 DevKit V1    │
                    │                      │
  TP4056 OUT+ ────►┤ VIN            GND  ├─ GND (common)
  TP4056 OUT- ────►┤ GND            3V3  ├─ (output ke sensor)
                    ├ VP (GPIO36)    GPIO23├─
                    ├ VN (GPIO39)    GPIO22├─
                    ├ GPIO34         GPIO21├─
                    ├ GPIO35         GPIO19├─
                    ├ GPIO32         GPIO18├─
                    ├ GPIO33          GPIO5├─
 Buzzer + ────────►┤ GPIO25         GPIO17├──── PZEM RX (← ESP TX)
 Relay IN ─────────►┤ GPIO26         GPIO16├──── PZEM TX (→ ESP RX)
                    ├ GPIO27          GPIO4├──── Tamper Switch
 DHT11 DATA ──────►─┤ GPIO13          GPIO2├──── LED (built-in)
                    ├ GPIO14                ├─
                    ├ GPIO12          GND  ├─
                    ├ GPIO15          3V3  ├─
                    │        [USB-C]       │
                    └──────────────────────┘
```

### Tabel Pin Assignment

| Komponen | Interface | ESP32 GPIO | Keterangan |
|----------|-----------|------------|------------|
| PZEM-004T TX | UART2 RX | **GPIO16** | PZEM TX → ESP32 RX (cross!) |
| PZEM-004T RX | UART2 TX | **GPIO17** | ESP32 TX → PZEM RX (cross!) |
| DHT11 DATA | Digital | **GPIO13** | Pull-up internal + 4.7kΩ external recommended |
| JQC-3FF Relay IN | Digital OUT | **GPIO26** | HIGH = ON, LOW = OFF |
| Buzzer (+) | PWM (LEDC) | **GPIO25** | Piezo buzzer aktif 3.3V |
| Tamper Switch | Digital IN | **GPIO4** | Pull-up, GND = tamper |
| LED Indicator | Digital OUT | **GPIO2** | Built-in LED (heartbeat) |

---

## Wiring Detail per Komponen

### 1. MEAN WELL IRM-10-3.3 — AC-DC Power Supply

PSU ini mengubah **220V AC → 3.3V DC**. Output-nya **tidak langsung** ke ESP32, tapi ke MT3608 untuk charging baterai.

| IRM-10-3.3 Pin | Sambung ke |
|----------------|------------|
| **L (AC Input)** | Terminal Block → dari MCB IC60N (Live) |
| **N (AC Input)** | Terminal Block → Neutral |
| **+Vo (DC 3.3V)** | MT3608 **IN+** |
| **-Vo (DC GND)** | MT3608 **IN-** (GND common) |

```
                ┌─────────────────────┐
MCB ──► Live ──►│ L        +Vo (3.3V) ├─────► MT3608 IN+
      Neutral ──►│ N        -Vo (GND)  ├─────► GND (common)
                └─────────────────────┘
                    MEAN WELL IRM-10-3.3

  Output: 3.3V DC, 3A max (10W)
  Input:  85-264V AC, 50/60Hz
```

> ⚠️ **PERINGATAN:**
> - IRM-10-3.3 terhubung langsung ke **220V AC**!
> - Pastikan sambungan AC terisolasi dengan baik
> - **JANGAN** sentuh terminal AC saat dalam keadaan hidup
> - Gunakan casing tertutup untuk keselamatan
> - Jika ESP32 juga terhubung USB (untuk programming), **cabut dulu IRM** sebelum colok USB!

---

### 2. MT3608 + TP4056 + Li-ion — Backup Power System (UPS)

Ini adalah sistem daya cadangan. Saat AC mati, baterai tetap menyuplai ESP32.

**Urutan sambungan:**

```
IRM-10-3.3 (3.3V) ──► MT3608 IN+
                      MT3608 OUT (5V) ──► TP4056 IN+
                                         TP4056 BAT ◄──► Li-ion 2500mAh
                                         TP4056 OUT+ ──► ESP32 VIN
                      GND (common) ──────────────────── ESP32 GND
```

#### MT3608 — Boost Converter

| MT3608 Pin | Sambung ke |
|------------|------------|
| **IN+** | IRM-10-3.3 **+Vo (3.3V)** |
| **IN-** | GND (common) |
| **OUT+** | TP4056 **IN+** |
| **OUT-** | GND (common) |

> 📌 **Putar trimpot MT3608** hingga output = **5.0V** (ukur pakai multimeter sebelum sambung ke TP4056!)

#### TP4056 — Li-ion Charger + UPS

| TP4056 Pin | Sambung ke |
|------------|------------|
| **IN+** | MT3608 **OUT+ (5V)** |
| **IN-** | GND (common) |
| **BAT+** | Li-ion **positif (+)** |
| **BAT-** | Li-ion **negatif (-)** |
| **OUT+** | ESP32 **VIN** pin |
| **OUT-** | ESP32 **GND** |

> 📌 TP4056 modul biasanya sudah include proteksi overcharge (DW01A + 8205A).
> 📌 LED merah = charging, LED biru = full charge.
> ⚠️ **JANGAN** sambungkan IRM langsung ke ESP32 3V3 saat menggunakan sistem UPS ini!

---

### 3. PZEM-004T + PZKHCT CT Clamp

| PZEM-004T Pin | Sambung ke |
|---------------|------------|
| **L (AC)** | Terminal Block → Live (dari MCB) |
| **N (AC)** | Terminal Block → Neutral |
| **TX (TTL)** | ESP32 **GPIO16** (cross-connect!) |
| **RX (TTL)** | ESP32 **GPIO17** (cross-connect!) |

```
                 ┌──────────────────────────┐
Terminal         │      PZEM-004T v3.0       │
Block    ──L───►│ [L]                       │
         ──N───►│ [N]    [CT]◄──────────────┤── PZKHCT CT Clamp
                 │                           │   (klem di kabel Live)
                 │  [TX]──────► GPIO16       │
                 │  [RX]◄────── GPIO17       │
                 └──────────────────────────┘
```

> 📌 **TX PZEM → GPIO16 (RX ESP32), RX PZEM → GPIO17 (TX ESP32) — CROSS!**
> 📌 CT Clamp diklem melingkari kabel **LIVE saja** (jangan potong kabel!)

---

### 4. DHT11 — Sensor Suhu & Kelembaban

| DHT11 Pin | Sambung ke |
|-----------|------------|
| **VCC** (pin 1) | ESP32 **3V3** |
| **DATA** (pin 2) | ESP32 **GPIO13** |
| **NC** (pin 3) | Tidak dipakai |
| **GND** (pin 4) | ESP32 **GND** |

```
DHT11 (tampak depan, kisi-kisi menghadap kamu)
┌─────────┐
│ ░░░░░░░ │
│         │
└─┬─┬─┬─┬┘
  1 2 3 4
  │ │   │
 3V3│  GND
    │
  GPIO13
```

> 📌 Jika pembacaan tidak stabil, tambahkan resistor **4.7kΩ** antara DATA dan 3V3.
> 📌 **JANGAN** gunakan GPIO15 — itu strapping pin yang mempengaruhi boot ESP32.

---

### 5. JQC-3FF-S-Z Relay — Kontrol Beban AC

| Relay Module Pin | Sambung ke |
|------------------|------------|
| **VCC** | ESP32 **3V3** (atau 5V jika relay versi 5V) |
| **GND** | ESP32 **GND** |
| **IN** (signal) | ESP32 **GPIO26** |

**Koneksi sisi AC (terminal screw relay):**
```
MCB ──► COM (Common) ──┐
                        │ JQC-3FF Relay
        NO ─────────────┤──► Beban AC ──► Neutral
                        │
        NC ─────────────┘ (tidak dipakai)
```

> 📌 **GPIO26 HIGH = Relay ON** (beban tersambung)
> ⚠️ Firmware otomatis **matikan relay** jika tamper terdeteksi atau suhu > 60°C

---

### 6. Buzzer — Alarm Suara

| Buzzer Pin | Sambung ke |
|------------|------------|
| **+ (positif)** | ESP32 **GPIO25** |
| **- (GND)** | ESP32 **GND** |

```
Buzzer (piezo aktif)
┌───────┐
│  (+) (-)│
└─┬───┬─┘
  │     │
GPIO25  GND
```

**Pola bunyi:**
| Event | Pola | Frekuensi |
|-------|------|-----------|
| Boot OK | 2x chirp cepat | 2000 Hz |
| PZEM Fail | 1x beep pendek | 1500 Hz |
| Suhu Tinggi | 2x beep sedang | 2500 Hz |
| Tamper | 3x beep panjang | 3000 Hz |

> 📌 Gunakan **buzzer aktif 3.3V** (sudah ada oscillator internal). Buzzer pasif juga bisa karena firmware menggunakan PWM.

---

### 7. MCB IC60N — Circuit Breaker

| Terminal | Sambung ke |
|----------|------------|
| **Input (atas)** | PLN 220V Live |
| **Output (bawah)** | Terminal Block → distribusi |

```
PLN 220V Live ──► [MCB IC60N] ──► Terminal Block ──┬── IRM-10-3.3 (L)
                                                    ├── PZEM-004T (L)
PLN Neutral ──────────────────── Terminal Block ──┬── IRM-10-3.3 (N)
                                                  ├── PZEM-004T (N)
                                                  └── Beban (N)
```

---

### 8. Tamper Switch — Deteksi Pembukaan Casing

| Switch Pin | Sambung ke |
|------------|------------|
| **Pin 1** | ESP32 **GPIO4** |
| **Pin 2** | ESP32 **GND** |

> Casing tertutup = LOW (normal), Casing terbuka = HIGH (**TAMPER!** → relay mati, buzzer 3x, alert dikirim)

---

## Diagram Wiring Lengkap

```
 PLN 220V AC
   │     │
   L     N
   │     │
┌──┴──┐  │
│ MCB │  │
│IC60N│  │
└──┬──┘  │
   │     │
┌──┴─────┴──── Terminal Block ────────────────────┐
│                                                  │
│  ┌──────────────┐   ┌──────────┐   ┌──────────┐ │
├─►│ IRM-10-3.3   │   │  MT3608  │   │  TP4056  │ │
│  │ L    +Vo 3.3V├──►│IN    OUT ├──►│IN    OUT+├──► ESP32 VIN
│  │ N    -Vo GND ├──►│GND   GND │   │     OUT-├──► ESP32 GND
│  └──────────────┘   └──────────┘   │BAT ◄──► │Li-ion 2500mAh│
│                                    └──────────┘ │
│                                                  │
│  ┌──────────────┐                                │
├─►│ PZEM-004T    │                                │
│  │ L            │  TX ──► GPIO16 (ESP32 RX)      │
│  │ N            │  RX ◄── GPIO17 (ESP32 TX)      │
│  │ CT ◄── PZKHCT│  (Clamp di Live)               │
│  └──────────────┘                                │
│                                                  │
│  ┌──────────────┐       ┌────────────────┐       │
├─►│ Relay COM    ├──►NO──┤ Beban AC       │──► N ─┤
│  │ JQC-3FF      │       └────────────────┘       │
│  │ IN ◄── GP26  │                                │
│  │ VCC ◄── 3V3  │                                │
│  │ GND ◄── GND  │                                │
│  └──────────────┘                                │
│                                                  │
└──────────────────────────────────────────────────┘

   ESP32 DevKit V1 (powered by TP4056 → VIN)
   ├── GPIO13 ◄── DHT11 DATA (+ 3V3, GND)
   ├── GPIO25 ──► Buzzer (+, GND)
   ├── GPIO26 ──► Relay IN
   ├── GPIO4  ◄── Tamper Switch (+ GND)
   ├── GPIO2  ──► LED built-in (heartbeat)
   └── GPIO16/17 ◄──► PZEM-004T (UART2)
```

---

## Checklist Sebelum Power On

### Sisi Backup Power (DC)
- [ ] MT3608: IN+ dari IRM +Vo, putar trimpot hingga OUT = **5.0V**
- [ ] TP4056: IN+ dari MT3608 OUT, BAT ke Li-ion, OUT+ ke ESP32 VIN
- [ ] Li-ion terpasang benar di TP4056 (cek polaritas!)
- [ ] ESP32 VIN dari TP4056 OUT+, GND dari TP4056 OUT-

### Sisi Sensor (DC)
- [ ] DHT11: VCC=3V3, DATA=GPIO13, GND=GND
- [ ] Relay: VCC=3V3, GND=GND, IN=GPIO26
- [ ] Buzzer: (+)=GPIO25, (-)=GND
- [ ] Tamper: satu kaki GPIO4, satu kaki GND
- [ ] PZEM TX → GPIO16 (ESP RX), PZEM RX → GPIO17 (ESP TX) — **CROSS!**
- [ ] Semua GND common (terhubung satu sama lain)

### Sisi AC (220V)
- [ ] MCB IC60N terpasang dan **OFF** dulu
- [ ] Terminal block mengamankan semua kabel AC
- [ ] IRM-10-3.3 AC input L dan N tersambung benar
- [ ] PZEM-004T L dan N tersambung benar
- [ ] CT Clamp PZKHCT diklem di kabel **Live** (bukan Neutral!)
- [ ] Relay COM dari MCB, NO ke beban, beban ke Neutral
- [ ] Semua kabel AC terisolasi — **tidak ada yang terbuka!**

### Urutan Power On
1. **Cabut USB** dari ESP32 (jika ada)
2. Flash firmware via USB terlebih dahulu (sebelum pasang ke sistem)
3. Pastikan MCB **OFF**
4. Pastikan MT3608 output = 5V (ukur dulu!)
5. Pasang Li-ion ke TP4056
6. Periksa semua koneksi sekali lagi
7. Nyalakan MCB → IRM → MT3608 → TP4056 → ESP32 boot
8. Amati: LED 3x blink (starting), 2x blink + chirp (WiFi connected)
9. Cek TP4056 LED: merah = charging, biru = full
10. Tes MCB OFF → ESP32 **tetap hidup** (battery), buzzer beep, alert terkirim

---

## Troubleshooting

| Masalah | Solusi |
|---------|--------|
| ESP32 tidak menyala | Cek TP4056 OUT dengan multimeter (harus 3.0–4.2V); cek baterai |
| MT3608 output salah | Putar trimpot pelan-pelan, ukur dengan multimeter (target 5.0V) |
| TP4056 LED merah terus | Baterai belum penuh (normal); jika >4 jam, cek koneksi baterai |
| TP4056 tidak charging | Cek MT3608 output (harus 5V); cek polaritas input TP4056 |
| PZEM tidak merespon | Cek TX/RX cross-connect, pastikan AC L/N terhubung |
| DHT11 selalu error | Pastikan pakai GPIO13 (bukan GPIO15!), tambah resistor 4.7kΩ pull-up |
| Relay tidak ON | Cek apakah relay versi 3.3V atau 5V, sesuaikan catu daya |
| Tamper selalu trigger | Pastikan switch normally-open, pull-up aktif di firmware |
| Buzzer tidak bunyi | Cek polaritas (+/−), cek apakah buzzer 3.3V atau 5V |
| "POST failed" | Backend harus jalan: `uvicorn --host 0.0.0.0 --port 8000` |
| ESP32 restart terus | Watchdog timeout — periksa koneksi WiFi dan backend |
| MCB off tapi ESP32 mati | Cek baterai (mungkin habis); cek koneksi TP4056 → VIN |

---

## Catatan Keselamatan

> ⚠️ **PERINGATAN — Proyek ini melibatkan tegangan AC 220V yang BERBAHAYA!**
>
> - Selalu **matikan MCB** sebelum menyentuh kabel atau komponen AC
> - Gunakan **casing tertutup** untuk deployment final
> - Jangan bekerja sendiri — minta orang lain standby
> - Li-ion bisa meledak jika short circuit — **jangan** biarkan terminal terbuka
> - Jika ragu, konsultasikan dengan dosen/teknisi listrik
