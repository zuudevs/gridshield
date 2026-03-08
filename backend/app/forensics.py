"""
GridShield Backend — Forensics Report Routes
Ingest and query firmware forensic incident reports.
"""

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy.orm import Session

from . import models, schemas
from .database import get_db
from .notification_engine import on_forensics_report

router = APIRouter(prefix="/api/forensics", tags=["Forensics"])


@router.post("/reports", response_model=schemas.ForensicsReportResponse, status_code=201)
def create_forensics_report(
    report: schemas.ForensicsReportCreate,
    db: Session = Depends(get_db),
):
    """Ingest a forensic incident report from firmware."""
    db_report = models.ForensicsReport(**report.model_dump())
    db.add(db_report)
    db.commit()
    db.refresh(db_report)

    # Auto-generate notification for critical/high severity reports
    if db_report.severity in ("critical", "high"):
        on_forensics_report(db_report, db)

    return db_report


@router.get("/reports", response_model=list[schemas.ForensicsReportResponse])
def list_forensics_reports(
    meter_id: int | None = None,
    report_type: str | None = None,
    limit: int = Query(default=50, le=200),
    db: Session = Depends(get_db),
):
    """List forensics reports with optional filters."""
    query = db.query(models.ForensicsReport)
    if meter_id is not None:
        query = query.filter(models.ForensicsReport.meter_id == meter_id)
    if report_type is not None:
        query = query.filter(models.ForensicsReport.report_type == report_type)
    return query.order_by(models.ForensicsReport.timestamp.desc()).limit(limit).all()


@router.get("/reports/{report_id}", response_model=schemas.ForensicsReportResponse)
def get_forensics_report(report_id: int, db: Session = Depends(get_db)):
    """Get a single forensics report by ID."""
    report = db.query(models.ForensicsReport).filter(
        models.ForensicsReport.id == report_id
    ).first()
    if report is None:
        raise HTTPException(status_code=404, detail="Report not found")
    return report
