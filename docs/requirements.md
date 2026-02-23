# SOFTWARE REQUIREMENTS SPECIFICATION (SRS)
## Project: GridShield - Multi-layer Security Model for AMI
**Version:** 2.0.0  
**Status:** Active  
**Analysis by:** Rafi  
**Last Updated:** February 2026

---

## 1. INTRODUCTION
### 1.1 Purpose
This document details the functional and non-functional requirements for **GridShield**, a multi-layer security system for smart meter infrastructure (AMI). The goal is to prevent data manipulation (tampering) and cyber attacks that harm utilities (PLN) and customers.

### 1.2 Problem Scope
As per the proposal, the main problems being solved are:
1.  **Physical Manipulation:** Illegal meter casing opening.
2.  **False Data Injection:** Cyber attacks on communication channels to manipulate billing.
3.  **Resource Constraints:** Smart Meter hardware (ESP32) has limited resources — requires lightweight cryptography (ECC, bukan RSA).

### 1.3 POC Scope
Dokumen ini mendefinisikan requirements untuk **Proof of Concept (POC)** dengan batasan:
- **Hardware Target:** ESP32 DevKit V1 (simulasi via Wokwi, deploy ke hardware fisik)
- **Build System:** PlatformIO (satu-satunya toolchain)
- **Backend:** Python + FastAPI + SQLite
- **Tim:** 3 orang (lihat section PIC mapping)

---

## 2. USERS & STAKEHOLDERS (User Personas)
| Actor | Description | Role in System |
| :--- | :--- | :--- |
| **Meter System (Edge)** | ESP32-based smart meter module at customer site. | Sends consumption data & tamper signals. |
| **Central Server (HES)** | Backend server (Python + FastAPI). | Receives data, validates signatures, & analyzes anomalies. |
| **Administrator/Operator** | P2TL or IT PLN personnel. | Monitors dashboard & receives theft alerts. |
| **Attacker/Thief (Threat)** | Malicious actor. | Attempts physical or network manipulation (mitigation target). |

---

## 3. FUNCTIONAL REQUIREMENTS (FR)

### 3.1 Layer 1: Physical Security
*Reference Code: FR-PHYS — **PIC: Cesar (Hardware) + Rafi (Firmware ISR)***

| ID | Requirement | POC Deliverable |
| :--- | :--- | :--- |
| **FR-PHYS-01** | System must detect if meter casing is forcibly opened using integrated sensors. | Limit switch → GPIO interrupt pada ESP32 |
| **FR-PHYS-02** | When tamper is detected, microcontroller must immediately send "Priority Flag" signal to server, bypassing routine queue. | ISR handler + priority packet via Serial/WiFi |
| **FR-PHYS-03** | Security module must be able to send tamper signal momentarily after main power is cut (using backup capacitor). | Supercapacitor circuit (Cesar) |

### 3.2 Layer 2: Network Security
*Reference Code: FR-NET — **PIC: Rafi (Cybersecurity + Software Engineer)***

| ID | Requirement | POC Deliverable |
| :--- | :--- | :--- |
| **FR-NET-01** | System must implement ECC (Elliptic Curve Cryptography) for signing measurement data before transmission. | ECDSA secp256r1 via micro-ecc pada ESP32 |
| **FR-NET-02** | Server must validate digital signatures from each data packet to ensure data originates from legitimate meters. | Signature verification di Python backend |
| **FR-NET-03** | Data payload must have hashing mechanism to ensure data is unchanged during transmission. | SHA-256 via rweather/Crypto |

> [!NOTE]
> AES-GCM encryption (FR-NET-01 original) diubah menjadi ECC signing saja untuk POC. Encryption akan ditambahkan di fase berikutnya.

### 3.3 Layer 3: Anomaly Analysis (Application Layer)
*Reference Code: FR-APP — **PIC: Ichwan (Data Scientist + Server Engineer + UI/UX)***

| ID | Requirement | POC Deliverable |
| :--- | :--- | :--- |
| **FR-APP-01** | Server must be able to receive and validate data from meters. | FastAPI endpoint + signature validation |
| **FR-APP-02** | System must compare real-time data with customer historical patterns. Trigger: if load drops >60% without logical reason → Flag Anomaly. | Statistical deviation algorithm (Python) |
| **FR-APP-03** | System must verify physical data against digital patterns (Cross-Layer Validation). | Combined tamper + anomaly alert logic |
| **FR-APP-04** | Provides visual interface to display meter security status (Green: Safe, Red: Tamper/Anomaly). | Simple web dashboard (HTML + Chart.js) |

---

## 4. NON-FUNCTIONAL REQUIREMENTS (NFR)

### 4.1 Efficiency & Performance
| ID | Requirement | PIC | POC Target |
| :--- | :--- | :--- | :--- |
| **NFR-PERF-01** | ECC signing on meter side must complete within 100ms. | Rafi | ESP32 HW accel: ~60ms ✅ |
| **NFR-PERF-02** | Security algorithm must not significantly drain power. | Rafi + Cesar | Deep sleep between readings |

### 4.2 Security
| ID | Requirement | PIC | POC Target |
| :--- | :--- | :--- | :--- |
| **NFR-SEC-01** | Meter data must not be readable by third parties (signed + integrity-checked). | Rafi | ECDSA + SHA-256 |
| **NFR-SEC-02** | Security system must not cause meter to hang or crash. | Rafi | Watchdog timer + no heap alloc |

### 4.3 Portability
| ID | Requirement | PIC | POC Target |
| :--- | :--- | :--- | :--- |
| **NFR-PORT-01** | C++ code for security module must be portable with minimal changes. | Rafi | HAL abstraction layer |

> [!NOTE]
> Untuk POC, portability difokuskan pada ESP32 saja. Multi-platform support (STM32, dll.) adalah future work.

---

## 5. SYSTEM CONSTRAINTS
1.  **Programming Language:** Firmware side must use **C++17**. Server/Analytics side uses **Python 3.11+**.
2.  **Hardware:** ESP32 DevKit V1 sebagai target utama. Development/testing via **Wokwi simulator**.
3.  **Build System:** **PlatformIO** sebagai satu-satunya toolchain (tanpa CMake native build).
4.  **Connectivity:** Serial (UART) untuk POC demo, WiFi HTTP untuk production-like demo.

---

## 6. TEST PLAN (Acceptance Criteria)

| Scenario ID | Test Description | Expected Result | PIC | Method |
| :--- | :--- | :--- | :--- | :--- |
| **TEST-01** | Open Meter Casing (trigger limit switch) | Alert appears on dashboard within < 5 seconds. | Ichwan (QA) | Wokwi simulation |
| **TEST-02** | Packet Data Sniffing (capture serial data) | Captured data contains valid ECDSA signature, raw payload without encryption context is meaningless without key. | Ichwan (QA) | Serial monitor analysis |
| **TEST-03** | False Data Injection (send malformed packet) | Server rejects manipulated data & logs attack. | Ichwan (QA) | Python test script |
| **TEST-04** | Extreme Load Drop (simulate 90% drop) | System flags anomaly as CRITICAL severity. | Ichwan (QA) | Backend unit test |

---

**Document Information:**
- **Version:** 2.0.0
- **Last Updated:** February 2026
- **Language:** Indonesian + English