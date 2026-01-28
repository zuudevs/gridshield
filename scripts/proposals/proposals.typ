
#set page(
  paper: "a4",
  margin: (top: 19mm, bottom: 43mm, left: 13mm, right: 13mm), 
  numbering: "1",
)

#set text(
  font: "Times New Roman",
  size: 12pt,
  lang: "id"
)

#set par(
  justify: true,
  leading: 1.15em, 
  first-line-indent: 0pt, 
  spacing: 1.2em 
)

#set list(indent: 1em)
#set enum(indent: 1em)

#set heading(numbering: "1.1")

#show heading.where(level: 1): it => [
  #pagebreak(weak: true)
  #set align(center)
  #set text(weight: "bold", size: 14pt)
  #v(2em)
  BAB #counter(heading).display("I") \
  #it.body
  #v(1em)
]

#show heading.where(level: 2): it => [
  #v(1em)
  #text(weight: "bold", size: 12pt)[
    #counter(heading).display() #it.body
  ]
  #v(0.5em)
]

#page(numbering: none)[
  #align(center + horizon)[
    #text(size: 14pt, weight: "bold")[PROPOSAL PROYEK]
    
    #v(2cm)
    
    #align(center + horizon)[
      #image("assets/images/logo_itpln.png", width: 4cm)
    ]
    
    #v(2cm)
    
    #text(size: 16pt, weight: "bold")[
      GRIDSHIELD: MODEL KEAMANAN MULTI-LAYER \
      UNTUK PERLINDUNGAN SISTEM AMI \
      (ADVANCED METERING INFRASTRUCTURE)
    ]
    
    #v(3cm)
    
    *Disusun Oleh:* \
    #v(0.5em)
    
    #align(center)[
      #table(
        columns: (auto, auto),
        align: left,
        gutter: 2em, 
        row-gutter: 1em, 
        stroke: none,
        [Muhammad Ichwan Fauzi], [202331227],
        [Rafi Indra Pramudhito Zuhayr], [202331291],
        [Cesar Ardika Bhayangkara], [202311240]
      )
    ]
    
    #v(1fr)
    
    *INSTITUT TEKNOLOGI PLN* \
    *JAKARTA* \
    *2025*
  ]
]



= RINGKASAN EKSEKUTIF

== Latar Belakang dan Masalah
Modernisasi kelistrikan melalui _Advanced Metering Infrastructure_ (AMI) adalah langkah strategis PLN. Namun, sistem ini membawa risiko keamanan siber baru. Masalah utama yang dihadapi adalah kerentanan terhadap manipulasi data pengukuran (_meter tampering_) baik secara fisik maupun melalui serangan siber (_cyber-attacks_) pada jalur komunikasi. Hal ini berpotensi menyebabkan kerugian finansial akibat pencurian listrik digital (_Non-Technical Losses_) dan ketidakakuratan data tagihan.

== Solusi Teknologi
GridShield menawarkan arsitektur keamanan _Multi-Layer_ khusus ekosistem AMI:
+ *Keamanan Fisik:* Sensor tamper terintegrasi dengan mekanisme _Power-Loss Alert_.
+ *Keamanan Jaringan:* Implementasi _Lightweight Cryptography_ berbasis ECC (Elliptic Curve Cryptography) yang dioptimalkan menggunakan *Modern C++ (C++23)* untuk efisiensi memori pada _embedded system_.
+ *Analitik Cerdas:* Deteksi anomali pola konsumsi secara _real-time_ di sisi server.

== Keunggulan Kompetitif
Berbeda dengan solusi vendor yang tertutup (_proprietary_), GridShield bersifat:
- *Resource-Constrained Friendly:* Algoritma dioptimalkan untuk mikrokontroler murah (ESP32/STM32).
- *Cross-Layer Validation:* Memverifikasi data fisik dengan pola digital.
- *Cost-Effective:* Biaya implementasi jauh lebih rendah dibandingkan lisensi keamanan vendor global.

= IDENTITAS TIM DAN KUALIFIKASI

== Komposisi Tim
Tim kami terdiri dari spesialis yang saling melengkapi:

- *Muhammad Ichwan Fauzi (Ketua/Network Security)* \
  Fokus pada arsitektur sistem, _threat modeling_, dan integrasi protokol keamanan. Bertanggung jawab memastikan seluruh sistem aman dari ujung ke ujung.

- *Rafi Indra Pramudhito Zuhayr (Software Engineer)* \
  Spesialis _Low-Level Programming_ (C/C++). Bertanggung jawab atas optimasi memori, implementasi algoritma kriptografi pada _firmware_, dan efisiensi kode sistem _embedded_. Penggunaan C++23 diperlukan untuk manajemen resource yang ketat.

- *Cesar Ardika Bhayangkara (Hardware Engineer)* \
  Fokus pada perancangan rangkaian elektronika, integrasi sensor fisik, dan komunikasi _hardware_ (UART/SPI/I2C). Bertanggung jawab atas realisasi fisik alat.

= ANALISIS MASALAH DAN SOLUSI

== Deskripsi Masalah
Tiga masalah utama yang diidentifikasi:
+ *Manipulasi Fisik Canggih:* Modifikasi komponen internal meteran untuk memperlambat putaran atau bypass.
+ *False Data Injection (FDI):* Manipulasi paket data di tengah transmisi untuk memalsukan laporan konsumsi.
+ *Keterbatasan Hardware:* Smart Meter memiliki CPU dan RAM kecil, tidak kuat menjalankan enkripsi standar PC (seperti RSA/AES-256 yang berat).

== Arsitektur Solusi (GridShield)
Kami menerapkan pendekatan _Defense-in-Depth_:
- *Layer 1 (Physical):* Sensor tamper mendeteksi pembukaan casing. Sinyal ini memicu _Priority Flag_ dan mengirim sinyal darurat via kapasitor cadangan meskipun listrik diputus.
- *Layer 2 (Network - Firmware):* Menggunakan C++23 untuk manajemen memori manual yang ketat. Menggantikan RSA dengan ECC (Elliptic Curve) yang memiliki kunci lebih pendek namun setara keamanannya, sangat cocok untuk _bandwidth_ IoT yang sempit.
- *Layer 3 (Application):* _Anomaly Detection Engine_ yang membandingkan beban _real-time_ dengan profil historis pelanggan untuk mendeteksi ketidakwajaran.

== Tingkat Kesiapan Teknologi (TRL)
Saat ini proyek berada pada *TRL 3 (Proof of Concept Experimental)*. Algoritma telah dipilih, arsitektur didefinisikan, dan simulasi kode awal sedang berjalan di lingkungan laboratorium terbatas.

= MODEL BISNIS DAN PASAR

== Target Pasar
Target implementasi GridShield mencakup:
- *B2G (PLN/Utilitas):* Unit P2TL (Penertiban Pemakaian Tenaga Listrik) dan Divisi AMI Pusat.
- *B2B (Kawasan Industri):* Pengelola kawasan industri terpadu yang memiliki pembangkit atau distribusi listrik mandiri.
- *OEM (Manufaktur):* Produsen meter listrik lokal (TKDN) yang membutuhkan modul keamanan tambahan untuk meningkatkan nilai jual.

== Model Pendapatan
- *Lisensi Per-Perangkat:* Biaya satu kali (_one-time fee_) per meter yang diinstal firmware GridShield.
- *SaaS Dashboard:* Biaya langganan bulanan untuk penggunaan sistem monitoring dan deteksi anomali berbasis cloud.

= RENCANA IMPLEMENTASI

== Rencana Kerja (Sprint Program)
Target pencapaian selama masa inkubasi 4 minggu:
/ Minggu 1 (Desain): Finalisasi _Threat Modeling_ dan skema _hardware_.
/ Minggu 2 (Dev - Security): Coding algoritma ECC pada ESP32 menggunakan C++.
/ Minggu 3 (Dev - Analytic): Pembuatan _backend_ deteksi anomali (Python/Go).
/ Minggu 4 (Integrasi): Penggabungan modul Hardware + Firmware + Cloud untuk Demo MVP.

== Kebutuhan Dukungan
Untuk keberhasilan proyek, kami membutuhkan:
- *Mentoring:* Bimbingan dari praktisi Cyber Security atau IoT Industri.
- *Akses Data:* Dataset dummy profil beban listrik (anonim) untuk melatih model deteksi.
- *Fasilitas:* Akses ke Laboratorium Sistem Tenaga atau Komputer untuk pengujian.

= ESTIMASI ANGGARAN BIAYA (RAB)

Berikut adalah estimasi biaya yang dibutuhkan untuk pengembangan purwarupa (MVP):

#figure(
  table(
    columns: (auto, 2fr, 4fr, 2fr),
    align: (center, left, left, right),
    fill: (col, row) => if row == 0 { luma(230) } else { none },
    inset: 10pt,
    [*No*], [*Komponen*], [*Deskripsi*], [*Estimasi Biaya (Rp)*],
    [1], [Komponen Hardware], [ESP32 DevKit, Sensor (Hall/Limit Switch), PCB, Casing Dummy], [1.500.000],
    [2], [Cloud & Server], [Sewa VPS/Cloud untuk Backend (3 Bulan)], [600.000],
    [3], [Konektivitas], [Paket Data IoT / Modul LoRa], [400.000],
    [4], [Operasional Riset], [Transportasi, Cetak Laporan, Konsumsi Tim], [1.000.000],
    table.cell(colspan: 3, align: right)[*TOTAL*], [*Rp 3.500.000*]
  ),
  caption: [Rencana Anggaran Biaya GridShield]
)

= PENUTUP

GridShield bukan hanya sekadar alat pengaman, melainkan solusi strategis untuk menyelamatkan pendapatan negara dari pencurian listrik modern. Dengan tim yang menggabungkan keahlian _hardware_, _software engineering_ tingkat rendah (C++), dan keamanan siber, kami yakin dapat menghadirkan purwarupa yang valid dan siap dikembangkan lebih lanjut menjadi produk industri.