"""
GridShield Backend — Data Export Routes
CSV export endpoints for readings, alerts, and anomalies.
"""

import csv
import io
from datetime import datetime

from fastapi import APIRouter, Depends, Query
from fastapi.responses import StreamingResponse
from sqlalchemy.orm import Session

from . import models
from .database import get_db

router = APIRouter(prefix="/api/export", tags=["Export"])


def _csv_response(rows: list[dict], filename: str) -> StreamingResponse:
    """Build a StreamingResponse with CSV content."""
    if not rows:
        output = io.StringIO()
        output.write("")
        output.seek(0)
        return StreamingResponse(
            iter([output.getvalue()]),
            media_type="text/csv",
            headers={"Content-Disposition": f'attachment; filename="{filename}"'},
        )

    output = io.StringIO()
    writer = csv.DictWriter(output, fieldnames=rows[0].keys())
    writer.writeheader()
    writer.writerows(rows)
    output.seek(0)
    return StreamingResponse(
        iter([output.getvalue()]),
        media_type="text/csv",
        headers={"Content-Disposition": f'attachment; filename="{filename}"'},
    )


# ============================================================================
# Export Readings
# ============================================================================
@router.get("/readings")
def export_readings(
    meter_id: int | None = None,
    limit: int = Query(default=1000, le=10000),
    db: Session = Depends(get_db),
):
    """Export meter readings as CSV."""
    query = db.query(models.MeterReading)
    if meter_id is not None:
        query = query.filter(models.MeterReading.meter_id == meter_id)
    results = query.order_by(models.MeterReading.timestamp.desc()).limit(limit).all()

    rows = [
        {
            "id": r.id,
            "meter_id": r.meter_id,
            "timestamp": r.timestamp.isoformat() if r.timestamp else "",
            "energy_wh": r.energy_wh,
            "voltage_mv": r.voltage_mv,
            "current_ma": r.current_ma,
            "power_factor": r.power_factor,
            "phase": r.phase,
        }
        for r in results
    ]

    ts = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
    return _csv_response(rows, f"gridshield_readings_{ts}.csv")


# ============================================================================
# Export Alerts
# ============================================================================
@router.get("/alerts")
def export_alerts(
    meter_id: int | None = None,
    limit: int = Query(default=1000, le=10000),
    db: Session = Depends(get_db),
):
    """Export tamper alerts as CSV."""
    query = db.query(models.TamperAlert)
    if meter_id is not None:
        query = query.filter(models.TamperAlert.meter_id == meter_id)
    results = query.order_by(models.TamperAlert.timestamp.desc()).limit(limit).all()

    rows = [
        {
            "id": a.id,
            "meter_id": a.meter_id,
            "timestamp": a.timestamp.isoformat() if a.timestamp else "",
            "tamper_type": a.tamper_type,
            "severity": a.severity,
            "acknowledged": a.acknowledged,
        }
        for a in results
    ]

    ts = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
    return _csv_response(rows, f"gridshield_alerts_{ts}.csv")


# ============================================================================
# Export Anomalies
# ============================================================================
@router.get("/anomalies")
def export_anomalies(
    meter_id: int | None = None,
    limit: int = Query(default=1000, le=10000),
    db: Session = Depends(get_db),
):
    """Export anomaly logs as CSV."""
    query = db.query(models.AnomalyLog)
    if meter_id is not None:
        query = query.filter(models.AnomalyLog.meter_id == meter_id)
    results = query.order_by(models.AnomalyLog.timestamp.desc()).limit(limit).all()

    rows = [
        {
            "id": a.id,
            "meter_id": a.meter_id,
            "timestamp": a.timestamp.isoformat() if a.timestamp else "",
            "anomaly_type": a.anomaly_type,
            "severity": a.severity,
            "current_value": a.current_value,
            "expected_value": a.expected_value,
            "deviation_percent": a.deviation_percent,
            "confidence": a.confidence,
        }
        for a in results
    ]

    ts = datetime.utcnow().strftime("%Y%m%d_%H%M%S")
    return _csv_response(rows, f"gridshield_anomalies_{ts}.csv")
