# PROJECT PROPOSAL
# GRIDSHIELD: MULTI-LAYER SECURITY MODEL FOR AMI SYSTEM PROTECTION

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

### 1.3 Competitive Advantages
Unlike closed vendor solutions (*proprietary*), GridShield is:
* **Resource-Constrained Friendly:** Algorithms optimized for affordable microcontrollers (Arduino Mega, with ESP32/STM32 support planned).
* **Cross-Layer Validation:** Verifies physical data against digital patterns.
* **Cost-Effective:** Implementation costs significantly lower than global vendor security licenses.

---

## CHAPTER II: TEAM IDENTITY AND QUALIFICATIONS

### 2.1 Team Composition
* **Muhammad Ichwan Fauzi (Lead/Network Security):** Focus on system architecture, *threat modeling*, and security protocol integration.
* **Rafi Indra Pramudhito Zuhayr (Software Engineer):** Specialist in *Low-Level Programming* (C/C++). Responsible for memory optimization, cryptographic algorithm implementation in *firmware*, and *embedded* system code efficiency.
* **Cesar Ardika Bhayangkara (Hardware Engineer):** Focus on electronic circuit design, physical sensor integration, and *hardware* communication (UART/SPI/I2C).

---

## CHAPTER III: PROBLEM ANALYSIS AND SOLUTIONS

### 3.1 Problem Description
* **Sophisticated Physical Manipulation:** Internal meter component modification.
* **False Data Injection (FDI):** Data packet manipulation during transmission.
* **Hardware Limitations:** Smart Meters have small CPU and RAM, unable to run standard PC encryption (RSA/AES-256 is too heavy).

### 3.2 Solution Architecture (GridShield)
*Defense-in-Depth* approach:
* **Layer 1 (Physical):** Tamper sensors detect casing opening -> Triggers *Priority Flag* -> Sends emergency signal via backup capacitor.
* **Layer 2 (Network - Firmware):** Uses C++17 for strict manual memory management. Replaces RSA with ECC (Elliptic Curve) which has shorter keys but equivalent security, suitable for narrow IoT *bandwidth*.
* **Layer 3 (Application):** *Anomaly Detection Engine* comparing *real-time* load with customer historical profiles.

### 3.3 Technology Readiness Level (TRL)
Currently at **TRL 4 (Validated in Lab Environment)**. Firmware implementation complete for Arduino Mega 2560, cryptographic algorithms validated, anomaly detection engine functional.

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

### 5.1 Work Plan (Sprint Program)
Targets during incubation period:
* **Week 1 (Design):** Finalization of *Threat Modeling* and *hardware* schematic.
* **Week 2 (Dev - Security):** ECC algorithm coding on Arduino Mega using C++17.
* **Week 3 (Dev - Analytic):** Backend anomaly detection creation (Python/Go).
* **Week 4 (Integration):** Combining Hardware + Firmware + Cloud modules for MVP Demo.

### 5.2 Support Requirements
* **Mentoring:** Cyber Security / Industrial IoT Practitioners.
* **Data Access:** Dummy electricity load profile dataset (anonymized).
* **Facilities:** Access to Power/Computer Systems Lab.

---

## CHAPTER VI: BUDGET ESTIMATE (RAB)

| No | Component | Description | Estimated Cost (Rp) |
| :--- | :--- | :--- | :--- |
| 1 | **Hardware Components** | Arduino Mega, Sensors (Hall/Limit Switch), PCB, Dummy Casing | 1,500,000 |
| 2 | **Cloud & Server** | VPS/Cloud rental for Backend (3 Months) | 600,000 |
| 3 | **Connectivity** | IoT Data Package / LoRa Module | 400,000 |
| 4 | **Research Operations** | Transportation, Report Printing, Team Consumables | 1,000,000 |
| **TOTAL** | | | **Rp 3,500,000** |

---

## CHAPTER VII: CONCLUSION

GridShield is not just a security tool, but a strategic solution to save national revenue from modern electricity theft. With a team combining *hardware*, low-level *software engineering* (C++17), and cybersecurity expertise, we are confident in delivering a valid prototype ready for further development.

---

**Document Information:**
- **Version:** 1.1.0
- **Last Updated:** February 2026
- **Language:** Translated from Indonesian (original)