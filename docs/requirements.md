# SOFTWARE REQUIREMENTS SPECIFICATION (SRS)
## Project: GridShield - Multi-layer Security Model for AMI
**Version:** 1.1.0  
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
3.  **Resource Constraints:** Smart Meter hardware (Arduino Mega, ESP32/STM32 planned) has limited power and memory, unable to run heavy encryption.

---

## 2. USERS & STAKEHOLDERS (User Personas)
| Actor | Description | Role in System |
| :--- | :--- | :--- |
| **Meter System (Edge)** | Smart Meter hardware at customer site. | Sends consumption data & tamper signals. |
| **Central Server (HES)** | Head-End System at utility/PLN. | Receives data, decrypts, & analyzes anomalies. |
| **Administrator/Operator** | P2TL or IT PLN personnel. | Monitors dashboard & receives theft alerts. |
| **Attacker/Thief (Threat)** | Malicious actor. | Attempts physical or network manipulation (mitigation target). |

---

## 3. FUNCTIONAL REQUIREMENTS (FR)

### 3.1 Layer 1: Physical Security
*Reference Code: FR-PHYS*
* **[FR-PHYS-01] Physical Tamper Detection:** System must be able to detect if meter casing is forcibly opened using integrated sensors.
* **[FR-PHYS-02] Priority Flagging:** When tamper is detected, microcontroller must immediately send "Priority Flag" signal to server, bypassing routine data transmission queue.
* **[FR-PHYS-03] Power Backup Operation:** Security module must be able to send tamper signal momentarily after main power is cut (using backup capacitor/battery).

### 3.2 Layer 2: Network Security
*Reference Code: FR-NET*
* **[FR-NET-01] Lightweight Encryption:** System must implement ECC (Elliptic Curve Cryptography) algorithm for encrypting measurement data before transmission.
    * *Note:* Must be optimized for C/C++ (Low-Level) for memory efficiency.
* **[FR-NET-02] Device Authentication:** Server must validate digital signatures from each data packet to ensure data originates from legitimate meters (Prevents *Man-in-the-Middle*).
* **[FR-NET-03] Data Integrity:** Data payload must have hashing mechanism to ensure data is unchanged during transmission.

### 3.3 Layer 3: Anomaly Analysis (Application Layer)
*Reference Code: FR-APP*
* **[FR-APP-01] Data Reception:** Server must be able to decrypt data received from thousands of meters *concurrently*.
* **[FR-APP-02] Load Anomaly Detection:** System must compare *real-time* data with customer historical patterns.
    * *Trigger:* If load drops drastically (e.g., 90%) without logical reason -> Flag Anomaly.
* **[FR-APP-03] Cross-Layer Validation:** System must verify physical data against digital patterns.
* **[FR-APP-04] Monitoring Dashboard:** Provides visual interface to display meter security status (Green: Safe, Red: Tamper/Anomaly).

---

## 4. NON-FUNCTIONAL REQUIREMENTS (NFR)

### 4.1 Efficiency & Performance
* **[NFR-PERF-01] Low Latency:** Encryption/decryption process on meter side must not impede primary measurement function (>100ms overhead considered failure).
* **[NFR-PERF-02] Power Efficient:** Security algorithm must not significantly drain power (Resource-constrained friendly).

### 4.2 Security
* **[NFR-SEC-01] Confidentiality:** Customer electricity consumption data must not be readable by third parties (sniffing).
* **[NFR-SEC-02] Availability:** Security system must not cause meter to *hang* or *crash* (Denial of Service).

### 4.3 Portability
* **[NFR-PORT-01] Hardware Agnostic:** C/C++ code for security module must be portable to various microcontroller types (Arduino Mega, ESP32, STM32) with minimal changes.

---

## 5. SYSTEM CONSTRAINTS
1.  **Programming Language:** Firmware side must use **C++17** (Per team expertise & hardware requirements). Server/Analytics side may use **Python**.
2.  **Hardware:** Current prototype uses Arduino Mega 2560. ESP32/STM32 support planned.
3.  **Connectivity:** Data transmission simulation using serial protocol or WiFi/LoRa (depending on available communication module).

---

## 6. TEST PLAN (Acceptance Criteria)
| Scenario ID | Test Description | Expected Result |
| :--- | :--- | :--- |
| **TEST-01** | Open Meter Casing | Alert appears on server dashboard within < 5 seconds. |
| **TEST-02** | Packet Data Sniffing | Captured data on network is unreadable (ciphertext). |
| **TEST-03** | False Data Injection | Server rejects manipulated data & logs attack. |
| **TEST-04** | Extreme Load Drop | System flags anomaly when electricity usage suddenly drops drastically. |

---

**Document Information:**
- **Version:** 1.1.0
- **Last Updated:** February 2026
- **Language:** Translated from Indonesian (original)