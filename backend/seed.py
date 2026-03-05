"""
GridShield Backend — Database Seed Script
Populates the SQLite database with realistic demo data.

Usage:
    cd backend
    python seed.py
"""

import random
import sys
from datetime import datetime, timedelta
from pathlib import Path

# Ensure we can import the app module
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from backend.app.database import Base, engine, SessionLocal
from backend.app.models import MeterReading, TamperAlert, AnomalyLog


# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------
NUM_METERS = 5
READINGS_PER_METER = 100
ALERTS_COUNT = 20
ANOMALIES_COUNT = 15
TIME_SPAN_HOURS = 72  # 3 days of data

METER_IDS = [
    1001,
    1002,
    1003,
    1004,
    1005,
]

TAMPER_TYPES = [
    "CasingOpened",
    "MagneticInterference",
    "PowerCutAttempt",
    "PhysicalShock",
    "VibrationDetected",
    "TemperatureAnomaly",
]

ANOMALY_TYPES = [
    "UnexpectedDrop",
    "UnexpectedSpike",
    "ZeroConsumption",
    "PatternDeviation",
]

SEVERITY_LEVELS = ["Low", "Medium", "High", "Critical"]


def generate_readings(session, now):
    """Generate realistic meter readings with daily patterns."""
    print(f"  Generating {READINGS_PER_METER * NUM_METERS} meter readings...")

    for meter_id in METER_IDS:
        base_energy = random.randint(800, 2000)
        for i in range(READINGS_PER_METER):
            ts = now - timedelta(
                hours=TIME_SPAN_HOURS * (1 - i / READINGS_PER_METER)
            )
            hour = ts.hour

            # Simulate daily pattern: higher during day, lower at night
            if 6 <= hour <= 9:
                factor = random.uniform(1.2, 1.5)   # morning peak
            elif 10 <= hour <= 17:
                factor = random.uniform(0.8, 1.1)    # daytime
            elif 18 <= hour <= 22:
                factor = random.uniform(1.3, 1.8)    # evening peak
            else:
                factor = random.uniform(0.3, 0.6)    # night

            energy = int(base_energy * factor + random.gauss(0, 50))
            voltage = 220000 + random.randint(-5000, 5000)
            current = int(energy / 0.22) + random.randint(-100, 100)
            pf = random.randint(850, 1000)

            session.add(MeterReading(
                meter_id=meter_id,
                timestamp=ts,
                energy_wh=max(0, energy),
                voltage_mv=voltage,
                current_ma=max(0, current),
                power_factor=pf,
                phase=random.choice([0, 1]),
            ))


def generate_alerts(session, now):
    """Generate tamper alerts across meters."""
    print(f"  Generating {ALERTS_COUNT} tamper alerts...")

    for _ in range(ALERTS_COUNT):
        ts = now - timedelta(hours=random.uniform(0, TIME_SPAN_HOURS))
        session.add(TamperAlert(
            meter_id=random.choice(METER_IDS),
            timestamp=ts,
            tamper_type=random.choice(TAMPER_TYPES),
            severity=random.randint(0, 4),
            acknowledged=random.random() < 0.3,  # 30% acknowledged
        ))


def generate_anomalies(session, now):
    """Generate anomaly detection events."""
    print(f"  Generating {ANOMALIES_COUNT} anomaly logs...")

    for _ in range(ANOMALIES_COUNT):
        ts = now - timedelta(hours=random.uniform(0, TIME_SPAN_HOURS))
        expected = random.uniform(800, 2000)
        deviation_pct = random.uniform(15, 95)
        direction = random.choice([-1, 1])
        current = expected * (1 + direction * deviation_pct / 100)

        session.add(AnomalyLog(
            meter_id=random.choice(METER_IDS),
            timestamp=ts,
            anomaly_type=random.choice(ANOMALY_TYPES),
            severity=random.choice(SEVERITY_LEVELS),
            current_value=round(current, 1),
            expected_value=round(expected, 1),
            deviation_percent=round(deviation_pct, 1),
            confidence=random.randint(40, 99),
        ))


def main():
    print("GridShield — Seeding Database")
    print("=" * 40)

    # Create tables
    Base.metadata.create_all(bind=engine)
    session = SessionLocal()

    try:
        # Clear existing data
        session.query(MeterReading).delete()
        session.query(TamperAlert).delete()
        session.query(AnomalyLog).delete()
        session.commit()
        print("  Cleared existing data.")

        now = datetime.now()
        generate_readings(session, now)
        generate_alerts(session, now)
        generate_anomalies(session, now)

        session.commit()
        print("=" * 40)
        print("[OK] Done! Seeded:")
        print(f"   - {READINGS_PER_METER * NUM_METERS} meter readings")
        print(f"   - {ALERTS_COUNT} tamper alerts")
        print(f"   - {ANOMALIES_COUNT} anomaly logs")
        print(f"   - {NUM_METERS} meters")
    except Exception as e:
        session.rollback()
        print(f"[ERROR] {e}")
        raise
    finally:
        session.close()


if __name__ == "__main__":
    main()
