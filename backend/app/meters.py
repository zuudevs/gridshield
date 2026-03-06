"""
GridShield Backend — Meter Management Routes
CRUD operations for meter registration and fleet management.
"""

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import func
from sqlalchemy.orm import Session

from . import models, schemas
from .database import get_db

router = APIRouter(prefix="/api", tags=["Meters"])


# ============================================================================
# Meter CRUD
# ============================================================================
@router.post("/meters", response_model=schemas.MeterResponse, status_code=201)
def register_meter(meter: schemas.MeterCreate, db: Session = Depends(get_db)):
    """Register a new meter device."""
    existing = db.query(models.Meter).filter(
        models.Meter.meter_id == meter.meter_id
    ).first()
    if existing is not None:
        raise HTTPException(status_code=409, detail="Meter already registered")
    db_meter = models.Meter(**meter.model_dump())
    db.add(db_meter)
    db.commit()
    db.refresh(db_meter)
    return db_meter


@router.get("/meters", response_model=list[schemas.MeterResponse])
def list_meters(
    status: str | None = None,
    limit: int = Query(default=100, le=500),
    db: Session = Depends(get_db),
):
    """List all registered meters with optional status filter."""
    query = db.query(models.Meter)
    if status is not None:
        query = query.filter(models.Meter.status == status)
    return query.order_by(models.Meter.registered_at.desc()).limit(limit).all()


@router.get("/meters/{meter_id}", response_model=schemas.MeterResponse)
def get_meter(meter_id: int, db: Session = Depends(get_db)):
    """Get a single meter by its hardware meter_id."""
    meter = db.query(models.Meter).filter(models.Meter.meter_id == meter_id).first()
    if meter is None:
        raise HTTPException(status_code=404, detail="Meter not found")
    return meter


@router.patch("/meters/{meter_id}", response_model=schemas.MeterResponse)
def update_meter(
    meter_id: int,
    update: schemas.MeterUpdate,
    db: Session = Depends(get_db),
):
    """Update meter information (name, location, firmware version, status)."""
    meter = db.query(models.Meter).filter(models.Meter.meter_id == meter_id).first()
    if meter is None:
        raise HTTPException(status_code=404, detail="Meter not found")

    update_data = update.model_dump(exclude_unset=True)
    for key, value in update_data.items():
        setattr(meter, key, value)

    db.commit()
    db.refresh(meter)
    return meter


@router.delete("/meters/{meter_id}", status_code=204)
def delete_meter(meter_id: int, db: Session = Depends(get_db)):
    """Deregister a meter device."""
    meter = db.query(models.Meter).filter(models.Meter.meter_id == meter_id).first()
    if meter is None:
        raise HTTPException(status_code=404, detail="Meter not found")
    db.delete(meter)
    db.commit()


# ============================================================================
# Meter Statistics
# ============================================================================
@router.get("/meters/{meter_id}/stats", response_model=schemas.MeterStats)
def get_meter_stats(meter_id: int, db: Session = Depends(get_db)):
    """Get aggregated statistics for a specific meter."""
    meter = db.query(models.Meter).filter(models.Meter.meter_id == meter_id).first()
    if meter is None:
        raise HTTPException(status_code=404, detail="Meter not found")

    total_readings = (
        db.query(func.count(models.MeterReading.id))
        .filter(models.MeterReading.meter_id == meter_id)
        .scalar()
        or 0
    )
    total_alerts = (
        db.query(func.count(models.TamperAlert.id))
        .filter(models.TamperAlert.meter_id == meter_id)
        .scalar()
        or 0
    )
    total_anomalies = (
        db.query(func.count(models.AnomalyLog.id))
        .filter(models.AnomalyLog.meter_id == meter_id)
        .scalar()
        or 0
    )

    avg_energy = (
        db.query(func.avg(models.MeterReading.energy_wh))
        .filter(models.MeterReading.meter_id == meter_id)
        .scalar()
        or 0.0
    )
    avg_voltage = (
        db.query(func.avg(models.MeterReading.voltage_mv))
        .filter(models.MeterReading.meter_id == meter_id)
        .scalar()
        or 0.0
    )
    avg_current = (
        db.query(func.avg(models.MeterReading.current_ma))
        .filter(models.MeterReading.meter_id == meter_id)
        .scalar()
        or 0.0
    )
    last_reading = (
        db.query(func.max(models.MeterReading.timestamp))
        .filter(models.MeterReading.meter_id == meter_id)
        .scalar()
    )

    return schemas.MeterStats(
        meter_id=meter_id,
        total_readings=total_readings,
        total_alerts=total_alerts,
        total_anomalies=total_anomalies,
        avg_energy_wh=round(avg_energy, 2),
        avg_voltage_mv=round(avg_voltage, 2),
        avg_current_ma=round(avg_current, 2),
        last_reading_time=last_reading,
    )
