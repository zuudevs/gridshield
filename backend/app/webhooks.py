"""
GridShield Backend — Webhook Configuration Routes
CRUD for webhook endpoints and test delivery.
"""

from datetime import datetime

from fastapi import APIRouter, Depends, HTTPException
from sqlalchemy.orm import Session

from . import models, schemas
from .database import get_db

router = APIRouter(prefix="/api", tags=["Webhooks"])


@router.post("/webhooks", response_model=schemas.WebhookConfigResponse, status_code=201)
def create_webhook(webhook: schemas.WebhookConfigCreate, db: Session = Depends(get_db)):
    """Register a new webhook endpoint."""
    db_wh = models.WebhookConfig(**webhook.model_dump())
    db.add(db_wh)
    db.commit()
    db.refresh(db_wh)
    return db_wh


@router.get("/webhooks", response_model=list[schemas.WebhookConfigResponse])
def list_webhooks(db: Session = Depends(get_db)):
    """List all configured webhooks."""
    return db.query(models.WebhookConfig).order_by(
        models.WebhookConfig.created_at.desc()
    ).all()


@router.delete("/webhooks/{webhook_id}", status_code=204)
def delete_webhook(webhook_id: int, db: Session = Depends(get_db)):
    """Remove a webhook configuration."""
    wh = db.query(models.WebhookConfig).filter(
        models.WebhookConfig.id == webhook_id
    ).first()
    if wh is None:
        raise HTTPException(status_code=404, detail="Webhook not found")
    db.delete(wh)
    db.commit()


@router.post("/webhooks/{webhook_id}/test", response_model=schemas.WebhookTestResult)
def test_webhook(webhook_id: int, db: Session = Depends(get_db)):
    """Send a test payload to a webhook endpoint."""
    wh = db.query(models.WebhookConfig).filter(
        models.WebhookConfig.id == webhook_id
    ).first()
    if wh is None:
        raise HTTPException(status_code=404, detail="Webhook not found")

    payload = {
        "event": "test",
        "message": "GridShield webhook test",
        "timestamp": datetime.utcnow().isoformat(),
    }

    try:
        import httpx

        headers = {"Content-Type": "application/json"}
        if wh.secret:
            headers["X-Webhook-Secret"] = wh.secret

        with httpx.Client(timeout=5.0) as client:
            resp = client.post(wh.url, json=payload, headers=headers)
            return schemas.WebhookTestResult(
                success=resp.status_code < 400,
                status_code=resp.status_code,
                message=f"Delivered to {wh.url}",
            )
    except Exception as exc:
        return schemas.WebhookTestResult(
            success=False,
            status_code=None,
            message=str(exc),
        )
