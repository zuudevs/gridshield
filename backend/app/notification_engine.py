"""
GridShield Backend — Notification Engine

Automatically creates in-app notifications when tamper alerts,
anomalies, or forensics reports are ingested. Also dispatches
configured webhooks for external integrations.
"""

import logging
from datetime import datetime

from sqlalchemy.orm import Session

from . import models

logger = logging.getLogger("gridshield.notifications")


def create_notification(
    db: Session,
    notification_type: str,
    message: str,
    severity: str = "info",
    meter_id: int | None = None,
    source_id: int | None = None,
) -> models.Notification:
    """Create and persist an in-app notification."""
    notif = models.Notification(
        timestamp=datetime.utcnow(),
        notification_type=notification_type,
        severity=severity,
        message=message,
        meter_id=meter_id,
        is_read=False,
        source_id=source_id,
    )
    db.add(notif)
    db.commit()
    db.refresh(notif)
    return notif


def dispatch_webhooks(
    db: Session,
    event_type: str,
    payload: dict,
) -> int:
    """
    Send HTTP POST to all enabled webhooks matching event_type.

    Returns the number of webhooks successfully dispatched.
    Failures are logged but do not raise exceptions.
    """
    webhooks = db.query(models.WebhookConfig).filter(
        models.WebhookConfig.enabled == True  # noqa: E712
    ).all()

    dispatched = 0
    for wh in webhooks:
        # Check if webhook listens to this event type
        types = [t.strip() for t in wh.event_types.split(",")]
        if "all" not in types and event_type not in types:
            continue

        try:
            import httpx

            headers = {"Content-Type": "application/json"}
            if wh.secret:
                headers["X-Webhook-Secret"] = wh.secret

            with httpx.Client(timeout=5.0) as client:
                resp = client.post(wh.url, json=payload, headers=headers)
                if resp.status_code < 400:
                    dispatched += 1
                else:
                    logger.warning(
                        "Webhook %s returned %d", wh.url, resp.status_code
                    )
        except Exception as exc:
            logger.warning("Webhook dispatch to %s failed: %s", wh.url, exc)

    return dispatched


def on_alert(alert: models.TamperAlert, db: Session) -> models.Notification:
    """Handle a new tamper alert — create notification and dispatch webhooks."""
    severity_map = {0: "info", 1: "low", 2: "medium", 3: "high", 4: "critical"}
    sev = severity_map.get(alert.severity, "info")

    meter_hex = format(alert.meter_id, "X")[-8:] if alert.meter_id else "?"
    message = f"Tamper alert: {alert.tamper_type} on meter {meter_hex} (severity: {sev})"

    notif = create_notification(
        db,
        notification_type="tamper_alert",
        message=message,
        severity=sev,
        meter_id=alert.meter_id,
        source_id=alert.id,
    )

    dispatch_webhooks(db, "tamper_alert", {
        "event": "tamper_alert",
        "alert_id": alert.id,
        "meter_id": alert.meter_id,
        "tamper_type": alert.tamper_type,
        "severity": alert.severity,
        "timestamp": alert.timestamp.isoformat() if alert.timestamp else None,
    })

    return notif


def on_anomaly(anomaly: models.AnomalyLog, db: Session) -> models.Notification:
    """Handle a new anomaly — create notification and dispatch webhooks."""
    meter_hex = format(anomaly.meter_id, "X")[-8:] if anomaly.meter_id else "?"
    message = (
        f"Anomaly detected: {anomaly.anomaly_type} on meter {meter_hex} "
        f"(deviation: {anomaly.deviation_percent:.1f}%, severity: {anomaly.severity})"
    )

    notif = create_notification(
        db,
        notification_type="anomaly",
        message=message,
        severity=anomaly.severity,
        meter_id=anomaly.meter_id,
        source_id=anomaly.id,
    )

    dispatch_webhooks(db, "anomaly", {
        "event": "anomaly",
        "anomaly_id": anomaly.id,
        "meter_id": anomaly.meter_id,
        "anomaly_type": anomaly.anomaly_type,
        "severity": anomaly.severity,
        "deviation_percent": anomaly.deviation_percent,
        "timestamp": anomaly.timestamp.isoformat() if anomaly.timestamp else None,
    })

    return notif


def on_forensics_report(report: models.ForensicsReport, db: Session) -> models.Notification:
    """Handle a new forensics report — create notification and dispatch webhooks."""
    meter_hex = format(report.meter_id, "X")[-8:] if report.meter_id else "?"
    message = (
        f"Forensics report: {report.report_type} from meter {meter_hex} "
        f"({report.event_count} events, confidence: {report.confidence}%)"
    )

    notif = create_notification(
        db,
        notification_type="forensics",
        message=message,
        severity=report.severity,
        meter_id=report.meter_id,
        source_id=report.id,
    )

    dispatch_webhooks(db, "forensics", {
        "event": "forensics_report",
        "report_id": report.id,
        "meter_id": report.meter_id,
        "report_type": report.report_type,
        "severity": report.severity,
        "confidence": report.confidence,
        "event_count": report.event_count,
        "timestamp": report.timestamp.isoformat() if report.timestamp else None,
    })

    return notif
