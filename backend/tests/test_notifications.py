"""
Tests for notification endpoints.
"""

from fastapi.testclient import TestClient


def test_list_notifications_empty(client: TestClient):
    """List notifications when none exist."""
    resp = client.get("/api/notifications")
    assert resp.status_code == 200
    assert resp.json() == []


def test_notification_auto_created_on_alert(client: TestClient):
    """Creating a tamper alert auto-generates a notification."""
    client.post("/api/tamper-alert", json={
        "meter_id": 88001,
        "tamper_type": "CasingOpened",
        "severity": 3,
    })
    resp = client.get("/api/notifications")
    assert resp.status_code == 200
    notifs = resp.json()
    assert len(notifs) >= 1
    assert notifs[0]["notification_type"] == "tamper_alert"
    assert notifs[0]["is_read"] is False


def test_notification_auto_created_on_anomaly(client: TestClient):
    """Creating an anomaly auto-generates a notification."""
    client.post("/api/anomalies", json={
        "meter_id": 88002,
        "anomaly_type": "UnexpectedSpike",
        "severity": "high",
        "current_value": 1500.0,
        "expected_value": 500.0,
        "deviation_percent": 200.0,
        "confidence": 90,
    })
    resp = client.get("/api/notifications")
    assert resp.status_code == 200
    notifs = resp.json()
    assert len(notifs) >= 1
    assert notifs[0]["notification_type"] == "anomaly"


def test_notification_summary(client: TestClient):
    """Notification summary returns unread count and recent items."""
    # Create an alert to trigger a notification
    client.post("/api/tamper-alert", json={
        "meter_id": 88003,
        "tamper_type": "MagneticField",
        "severity": 2,
    })
    resp = client.get("/api/notifications/summary")
    assert resp.status_code == 200
    data = resp.json()
    assert data["unread_count"] >= 1
    assert len(data["recent"]) >= 1


def test_mark_notification_read(client: TestClient):
    """Mark a single notification as read."""
    client.post("/api/tamper-alert", json={
        "meter_id": 88004,
        "tamper_type": "CasingOpened",
        "severity": 1,
    })
    notifs = client.get("/api/notifications").json()
    notif_id = notifs[0]["id"]

    resp = client.patch(f"/api/notifications/{notif_id}/read")
    assert resp.status_code == 200
    assert resp.json()["is_read"] is True


def test_mark_all_notifications_read(client: TestClient):
    """Mark all notifications as read."""
    # Create two alerts to generate two notifications
    for i in range(2):
        client.post("/api/tamper-alert", json={
            "meter_id": 88005 + i,
            "tamper_type": "PowerCut",
            "severity": 1,
        })

    resp = client.post("/api/notifications/read-all")
    assert resp.status_code == 200
    assert resp.json()["updated"] >= 2

    summary = client.get("/api/notifications/summary").json()
    assert summary["unread_count"] == 0
