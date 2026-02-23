"""
GridShield Backend — API Routes
"""

from fastapi import APIRouter, Depends, Query
from sqlalchemy import func
from sqlalchemy.orm import Session

from . import models, schemas
from .database import get_db

router = APIRouter(prefix="/api", tags=["GridShield API"])


# ============================================================================
# Meter Readings
# ============================================================================
@router.post("/meter-data", response_model=schemas.MeterReadingResponse, status_code=201)
def create_meter_reading(reading: schemas.MeterReadingCreate, db: Session = Depends(get_db)):
    """Receive a meter reading from the ESP32 device."""
    db_reading = models.MeterReading(**reading.model_dump())
    db.add(db_reading)
    db.commit()
    db.refresh(db_reading)
    return db_reading


@router.get("/readings", response_model=list[schemas.MeterReadingResponse])
def list_readings(
    meter_id: int | None = None,
    limit: int = Query(default=50, le=200),
    db: Session = Depends(get_db),
):
    """List recent meter readings, optionally filtered by meter_id."""
    query = db.query(models.MeterReading)
    if meter_id is not None:
        query = query.filter(models.MeterReading.meter_id == meter_id)
    return query.order_by(models.MeterReading.timestamp.desc()).limit(limit).all()


# ============================================================================
# Tamper Alerts
# ============================================================================
@router.post("/tamper-alert", response_model=schemas.TamperAlertResponse, status_code=201)
def create_tamper_alert(alert: schemas.TamperAlertCreate, db: Session = Depends(get_db)):
    """Receive a tamper alert from the ESP32 device."""
    db_alert = models.TamperAlert(**alert.model_dump())
    db.add(db_alert)
    db.commit()
    db.refresh(db_alert)
    return db_alert


@router.get("/alerts", response_model=list[schemas.TamperAlertResponse])
def list_alerts(
    meter_id: int | None = None,
    acknowledged: bool | None = None,
    limit: int = Query(default=50, le=200),
    db: Session = Depends(get_db),
):
    """List tamper alerts with optional filters."""
    query = db.query(models.TamperAlert)
    if meter_id is not None:
        query = query.filter(models.TamperAlert.meter_id == meter_id)
    if acknowledged is not None:
        query = query.filter(models.TamperAlert.acknowledged == acknowledged)
    return query.order_by(models.TamperAlert.timestamp.desc()).limit(limit).all()


@router.patch("/alerts/{alert_id}/acknowledge", response_model=schemas.TamperAlertResponse)
def acknowledge_alert(alert_id: int, db: Session = Depends(get_db)):
    """Acknowledge a tamper alert."""
    alert = db.query(models.TamperAlert).filter(models.TamperAlert.id == alert_id).first()
    if alert is None:
        from fastapi import HTTPException
        raise HTTPException(status_code=404, detail="Alert not found")
    alert.acknowledged = True
    db.commit()
    db.refresh(alert)
    return alert


# ============================================================================
# Anomaly Logs
# ============================================================================
@router.post("/anomalies", response_model=schemas.AnomalyLogResponse, status_code=201)
def create_anomaly_log(anomaly: schemas.AnomalyLogCreate, db: Session = Depends(get_db)):
    """Log an anomaly detection event."""
    db_anomaly = models.AnomalyLog(**anomaly.model_dump())
    db.add(db_anomaly)
    db.commit()
    db.refresh(db_anomaly)
    return db_anomaly


@router.get("/anomalies", response_model=list[schemas.AnomalyLogResponse])
def list_anomalies(
    meter_id: int | None = None,
    limit: int = Query(default=50, le=200),
    db: Session = Depends(get_db),
):
    """List anomaly logs."""
    query = db.query(models.AnomalyLog)
    if meter_id is not None:
        query = query.filter(models.AnomalyLog.meter_id == meter_id)
    return query.order_by(models.AnomalyLog.timestamp.desc()).limit(limit).all()


# ============================================================================
# System Status
# ============================================================================
@router.get("/status", response_model=schemas.SystemStatus)
def get_status(db: Session = Depends(get_db)):
    """Get system-wide status summary."""
    total_readings = db.query(func.count(models.MeterReading.id)).scalar() or 0
    total_alerts = db.query(func.count(models.TamperAlert.id)).scalar() or 0
    total_anomalies = db.query(func.count(models.AnomalyLog.id)).scalar() or 0
    active_meters = db.query(func.count(func.distinct(models.MeterReading.meter_id))).scalar() or 0
    unack = db.query(func.count(models.TamperAlert.id)).filter(
        models.TamperAlert.acknowledged == False  # noqa: E712
    ).scalar() or 0

    latest = db.query(func.max(models.MeterReading.timestamp)).scalar()

    return schemas.SystemStatus(
        total_readings=total_readings,
        total_alerts=total_alerts,
        total_anomalies=total_anomalies,
        active_meters=active_meters,
        unacknowledged_alerts=unack,
        latest_reading_time=latest,
    )
