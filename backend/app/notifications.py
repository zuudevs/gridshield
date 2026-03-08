"""
GridShield Backend — Notification Routes
In-app notification listing, read/unread management, and summary.
"""

from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import func
from sqlalchemy.orm import Session

from . import models, schemas
from .database import get_db

router = APIRouter(prefix="/api", tags=["Notifications"])


@router.get("/notifications", response_model=list[schemas.NotificationResponse])
def list_notifications(
    is_read: bool | None = None,
    limit: int = Query(default=50, le=200),
    db: Session = Depends(get_db),
):
    """List notifications with optional read/unread filter."""
    query = db.query(models.Notification)
    if is_read is not None:
        query = query.filter(models.Notification.is_read == is_read)
    return query.order_by(models.Notification.timestamp.desc()).limit(limit).all()


@router.get("/notifications/summary", response_model=schemas.NotificationSummary)
def notification_summary(db: Session = Depends(get_db)):
    """Get unread count and recent notifications."""
    unread_count = (
        db.query(func.count(models.Notification.id))
        .filter(models.Notification.is_read == False)  # noqa: E712
        .scalar()
        or 0
    )

    recent = (
        db.query(models.Notification)
        .order_by(models.Notification.timestamp.desc())
        .limit(5)
        .all()
    )

    return schemas.NotificationSummary(unread_count=unread_count, recent=recent)


@router.patch("/notifications/{notification_id}/read", response_model=schemas.NotificationResponse)
def mark_notification_read(notification_id: int, db: Session = Depends(get_db)):
    """Mark a single notification as read."""
    notif = db.query(models.Notification).filter(
        models.Notification.id == notification_id
    ).first()
    if notif is None:
        raise HTTPException(status_code=404, detail="Notification not found")
    notif.is_read = True
    db.commit()
    db.refresh(notif)
    return notif


@router.post("/notifications/read-all", status_code=200)
def mark_all_notifications_read(db: Session = Depends(get_db)):
    """Mark all unread notifications as read."""
    updated = (
        db.query(models.Notification)
        .filter(models.Notification.is_read == False)  # noqa: E712
        .update({"is_read": True})
    )
    db.commit()
    return {"updated": updated}
