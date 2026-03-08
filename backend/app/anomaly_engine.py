"""
GridShield Backend — Server-Side Anomaly Detection Engine

Automatically analyzes incoming meter readings and flags anomalies
when consumption deviates significantly from recent patterns.
"""

from datetime import datetime

from sqlalchemy import func
from sqlalchemy.orm import Session

from . import models

# Default thresholds
ANOMALY_THRESHOLD_PERCENT = 60.0
CRITICAL_THRESHOLD_PERCENT = 80.0
RECENT_READINGS_WINDOW = 10  # compare against last N readings


def classify_severity(deviation: float) -> str:
    """Classify anomaly severity based on deviation percentage."""
    if deviation >= CRITICAL_THRESHOLD_PERCENT:
        return "critical"
    if deviation >= 70.0:
        return "high"
    if deviation >= ANOMALY_THRESHOLD_PERCENT:
        return "medium"
    return "low"


def classify_anomaly_type(current: float, expected: float) -> str:
    """Classify the type of anomaly based on direction of deviation."""
    if current < 1.0:
        return "ZeroConsumption"
    if current < expected:
        return "UnexpectedDrop"
    return "UnexpectedSpike"


def analyze_reading(reading: models.MeterReading, db: Session) -> models.AnomalyLog | None:
    """
    Analyze a newly ingested meter reading for anomalies.

    Compares the reading's energy_wh against the average of the last
    N readings for the same meter. If deviation exceeds the threshold,
    an AnomalyLog is auto-created and returned.

    Returns None if no anomaly is detected.
    """
    # Get the average of recent readings for this meter (excluding the new one)
    avg_energy = (
        db.query(func.avg(models.MeterReading.energy_wh))
        .filter(
            models.MeterReading.meter_id == reading.meter_id,
            models.MeterReading.id != reading.id,
        )
        .scalar()
    )

    # Need at least some historical data to compare
    if avg_energy is None or avg_energy < 1.0:
        return None

    reading_count = (
        db.query(func.count(models.MeterReading.id))
        .filter(
            models.MeterReading.meter_id == reading.meter_id,
            models.MeterReading.id != reading.id,
        )
        .scalar()
        or 0
    )

    # Need enough samples for meaningful comparison
    if reading_count < 3:
        return None

    # Calculate deviation
    current_value = float(reading.energy_wh)
    expected_value = float(avg_energy)
    deviation = abs(current_value - expected_value) / expected_value * 100.0

    if deviation < ANOMALY_THRESHOLD_PERCENT:
        return None

    # Anomaly detected — create log entry
    anomaly_type = classify_anomaly_type(current_value, expected_value)
    severity = classify_severity(deviation)
    confidence = min(100, int(deviation))

    anomaly = models.AnomalyLog(
        meter_id=reading.meter_id,
        timestamp=datetime.utcnow(),
        anomaly_type=anomaly_type,
        severity=severity,
        current_value=current_value,
        expected_value=round(expected_value, 2),
        deviation_percent=round(deviation, 2),
        confidence=confidence,
    )
    db.add(anomaly)
    db.commit()
    db.refresh(anomaly)

<<<<<<< HEAD
    # Auto-generate notification for detected anomaly
    from .notification_engine import on_anomaly
    on_anomaly(anomaly, db)

=======
>>>>>>> origin/main
    return anomaly
