"""
Tests for webhook configuration endpoints.
"""

from fastapi.testclient import TestClient


SAMPLE_WEBHOOK = {
    "url": "https://example.com/webhook",
    "secret": "test-secret-123",
    "enabled": True,
    "event_types": "tamper_alert,anomaly",
    "description": "Test webhook",
}


def test_create_webhook(client: TestClient):
    """Create a webhook configuration."""
    resp = client.post("/api/webhooks", json=SAMPLE_WEBHOOK)
    assert resp.status_code == 201
    data = resp.json()
    assert data["url"] == SAMPLE_WEBHOOK["url"]
    assert data["enabled"] is True
    assert data["event_types"] == "tamper_alert,anomaly"


def test_list_webhooks(client: TestClient):
    """List configured webhooks."""
    client.post("/api/webhooks", json=SAMPLE_WEBHOOK)
    resp = client.get("/api/webhooks")
    assert resp.status_code == 200
    assert len(resp.json()) >= 1


def test_delete_webhook(client: TestClient):
    """Delete a webhook configuration."""
    create_resp = client.post("/api/webhooks", json=SAMPLE_WEBHOOK)
    wh_id = create_resp.json()["id"]

    del_resp = client.delete(f"/api/webhooks/{wh_id}")
    assert del_resp.status_code == 204

    # Verify it's gone
    webhooks = client.get("/api/webhooks").json()
    assert not any(w["id"] == wh_id for w in webhooks)


def test_delete_webhook_not_found(client: TestClient):
    """Delete a non-existent webhook returns 404."""
    resp = client.delete("/api/webhooks/9999")
    assert resp.status_code == 404


def test_test_webhook_not_found(client: TestClient):
    """Test a non-existent webhook returns 404."""
    resp = client.post("/api/webhooks/9999/test")
    assert resp.status_code == 404


def test_test_webhook_unreachable(client: TestClient):
    """Test webhook to unreachable URL returns failure result."""
    create_resp = client.post("/api/webhooks", json={
        "url": "http://localhost:1/nonexistent",
        "secret": "",
        "enabled": True,
        "event_types": "all",
        "description": "Unreachable test",
    })
    wh_id = create_resp.json()["id"]

    resp = client.post(f"/api/webhooks/{wh_id}/test")
    assert resp.status_code == 200
    data = resp.json()
    assert data["success"] is False
