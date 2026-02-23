# PROJECT PROPOSAL
# GRIDSHIELD: MULTI-LAYER SECURITY MODEL FOR AMI SYSTEM PROTECTION

**Version:** 2.0.0  
**Last Updated:** February 2026  

**Prepared By:**
* Muhammad Ichwan Fauzi (202331227)
* Rafi Indra Pramudhito Zuhayr (202331291)
* Cesar Ardika Bhayangkara (202311240)

**Institut Teknologi PLN - 2025**

---

## CHAPTER I: EXECUTIVE SUMMARY

### 1.1 Background and Problem Statement
Grid modernization through *Advanced Metering Infrastructure* (AMI) is a strategic initiative for PLN (Indonesian state electricity company). However, this system introduces new cybersecurity risks. The main challenges are vulnerabilities to measurement data manipulation (*meter tampering*) both physically and through cyber-attacks on communication channels. This potentially causes financial losses due to digital electricity theft (*Non-Technical Losses*) and billing data inaccuracies.

### 1.2 Technology Solution
GridShield offers a *Multi-Layer* security architecture specifically designed for the AMI ecosystem:
1.  **Physical Security:** Integrated tamper sensors with *Power-Loss Alert* mechanism.
2.  **Network Security:** Implementation of *Lightweight Cryptography* based on ECC (Elliptic Curve Cryptography) optimized using **Modern C++ (C++17)** for memory efficiency on *embedded systems*.
3.  **Smart Analytics:** Real-time anomaly detection of consumption patterns on the server side.

**Target Platform:** ESP32 (Xtensa LX6 @ 240 MHz) — development via **PlatformIO** + simulasi **Wokwi**.

### 1.3 Competitive Advantages
Unlike closed vendor solutions (*proprietary*), GridShield is:
* **Resource-Constrained Friendly:** Algorithms optimized for ESP32 microcontroller with hardware crypto acceleration.
* **Simulation-Ready:** Full development cycle via PlatformIO + Wokwi tanpa perlu hardware fisik.
* **Cross-Layer Validation:** Verifies physical data against digital patterns.
* **Cost-Effective:** Implementation costs significantly lower than global vendor security licenses.

---

## CHAPTER II: TEAM IDENTITY AND QUALIFICATIONS

### 2.1 Team Composition & Core Roles

| PIC | Core Domain | Key Roles (Weight) |
|---|---|---|
| **M. Ichwan Fauzi** | Data & Server | Data Scientist (11%), Server Engineer (5%), QA (5%), UI/UX (4%), DBA (3%) |
| **Rafi Indra P. Zuhayr** | Firmware & Security | Software Engineer (14%), Embedded Firmware (8%), Cybersecurity (6%), System Architect (3%) |
| **Cesar Ardika B.** | Hardware & Power | PCB Design (9%), Electrical Engineer (6%), Power Systems (5%), Supply Chain (3%) |

**Role Details:**
* **Ichwan (Data & Server — 28%):** Mengembangkan algoritma deteksi anomali Layer 3, mengelola backend server (Python + SQLite), merancang dashboard monitoring, dan melakukan QA/testing.
* **Rafi (Firmware & Security — 37%):** Membangun logika inti sistem C++17 pada ESP32, mengimplementasikan kriptografi ECC (Layer 2), menangani firmware ISR/GPIO (Layer 1), dan mengelola arsitektur sistem.
* **Cesar (Hardware & Power — 23%):** Merancang PCB modul keamanan, mengintegrasikan sensor fisik (tamper detection), mengelola sistem daya cadangan, dan mengatur pengadaan komponen.

---

## CHAPTER III: PROBLEM ANALYSIS AND SOLUTIONS

### 3.1 Problem Description
* **Sophisticated Physical Manipulation:** Internal meter component modification.
* **False Data Injection (FDI):** Data packet manipulation during transmission.
* **Hardware Limitations:** Smart Meters have limited resources, requiring lightweight cryptography (RSA terlalu berat, ECC lebih cocok).

### 3.2 Solution Architecture (GridShield)
*Defense-in-Depth* approach with **ESP32** as target platform:
* **Layer 1 (Physical) — PIC: Cesar + Rafi:** Tamper sensors detect casing opening → Triggers *Priority Flag* → ISR handler dengan debounce logic.
* **Layer 2 (Network - Firmware) — PIC: Rafi:** C++17 firmware pada ESP32. ECDSA signing (micro-ecc) + SHA-256 hashing untuk integritas data.
* **Layer 3 (Application) — PIC: Ichwan:** Statistical anomaly detection engine comparing real-time consumption with 24-hour historical profile.

**Development & Simulation:**
* **Build System:** PlatformIO (unified toolchain)
* **Simulator:** Wokwi (GPIO, interrupt, serial — tanpa hardware fisik)
* **Backend:** Python + FastAPI + SQLite (lightweight, sesuai POC scope)

### 3.3 Technology Readiness Level (TRL)
Currently at **TRL 4 (Validated in Lab/Simulation Environment)**. Firmware implementation for ESP32 via PlatformIO, cryptographic algorithms validated dengan micro-ecc, anomaly detection engine functional.

---

## CHAPTER IV: BUSINESS MODEL AND MARKET

### 4.1 Target Market
* **B2G (PLN/Utilities):** P2TL Units and AMI Division.
* **B2B (Industrial Areas):** Area managers with independent generation/distribution.
* **OEM (Manufacturing):** Local meter manufacturers (TKDN) requiring additional security modules.

### 4.2 Revenue Model
* **Per-Device License:** One-time fee per meter with GridShield installed.
* **SaaS Dashboard:** Monthly subscription for anomaly monitoring.

---

## CHAPTER V: IMPLEMENTATION PLAN AND REQUIREMENTS

### 5.1 Work Plan (Sprint Program by PIC)

Targets during incubation period:

| Week | Rafi (Firmware & Security) | Ichwan (Data & Server) | Cesar (Hardware & Power) |
|---|---|---|---|
| **1** | Setup PlatformIO + Wokwi, arsitektur core system | Setup Python backend + SQLite | Desain schematic + sensor selection |
| **2** | Implementasi Layer 2 (ECC + packet protocol) | Setup MQTT/serial receiver + DB schema | Integrasi sensor tamper di Wokwi |
| **3** | Implementasi Layer 1 (ISR + debounce) | Implementasi Layer 3 (anomaly engine) + dashboard | PCB layout + power backup circuit |
| **4** | Integrasi 3 layer + QA firmware | QA backend + demo preparation | Hardware assembly + final testing |

### 5.2 Support Requirements
* **Mentoring:** Cyber Security / Industrial IoT Practitioners.
* **Data Access:** Dummy electricity load profile dataset (anonymized).
* **Tools:** PlatformIO + Wokwi (free tier cukup untuk POC).
* **Facilities:** Access to Power/Computer Systems Lab (untuk hardware testing final).

---

## CHAPTER VI: BUDGET ESTIMATE (RAB)

| No | Component | Description | Estimated Cost (Rp) |
| :--- | :--- | :--- | :--- |
| 1 | **Hardware Components** | ESP32 DevKit V1 (×2), Sensors (Hall/Limit Switch), PCB, Dummy Casing | 1,200,000 |
| 2 | **Cloud & Server** | VPS/Cloud rental for Backend (3 Months) | 600,000 |
| 3 | **Connectivity** | WiFi (built-in ESP32) — no additional module needed | 0 |
| 4 | **Development Tools** | PlatformIO + Wokwi (free tier) | 0 |
| 5 | **Research Operations** | Transportation, Report Printing, Team Consumables | 1,000,000 |
| **TOTAL** | | | **Rp 2,800,000** |

---

## CHAPTER VII: CONCLUSION

GridShield is not just a security tool, but a strategic solution to save national revenue from modern electricity theft. With a team combining *hardware engineering* (Cesar), low-level *software engineering* C++17 (Rafi), and *data science* anomaly detection (Ichwan) — all targeting **ESP32** via **PlatformIO + Wokwi** — we are confident in delivering a valid, simulation-ready prototype that can be deployed to real hardware with minimal changes.

---

**Document Information:**
- **Version:** 2.0.0
- **Last Updated:** February 2026
- **Language:** Translated from Indonesian (original)