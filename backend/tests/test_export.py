"""
Tests for CSV export endpoints.
"""

from fastapi.testclient import TestClient


def test_export_readings_empty(client: TestClient):
    """Export readings CSV when no data exists — returns empty CSV."""
    resp = client.get("/api/export/readings")
    assert resp.status_code == 200
    assert resp.headers["content-type"].startswith("text/csv")


def test_export_readings_with_data(client: TestClient):
    """Export readings CSV with data — returns proper CSV rows."""
    # Insert a reading
    client.post("/api/meter-data", json={
        "meter_id": 99001,
        "energy_wh": 500,
        "voltage_mv": 220000,
        "current_ma": 2000,
    })
    resp = client.get("/api/export/readings")
    assert resp.status_code == 200
    lines = resp.text.strip().split("\n")
    assert len(lines) >= 2  # header + at least 1 row
    assert "meter_id" in lines[0]
    assert "energy_wh" in lines[0]
    assert "99001" in lines[1]


def test_export_alerts_empty(client: TestClient):
    """Export alerts CSV when no data exists."""
    resp = client.get("/api/export/alerts")
    assert resp.status_code == 200
    assert resp.headers["content-type"].startswith("text/csv")


def test_export_alerts_with_data(client: TestClient):
    """Export alerts CSV with data."""
    client.post("/api/tamper-alert", json={
        "meter_id": 99002,
        "tamper_type": "CasingOpened",
        "severity": 3,
    })
    resp = client.get("/api/export/alerts")
    assert resp.status_code == 200
    lines = resp.text.strip().split("\n")
    assert len(lines) >= 2
    assert "tamper_type" in lines[0]
    assert "CasingOpened" in lines[1]


def test_export_anomalies_empty(client: TestClient):
    """Export anomalies CSV when no data exists."""
    resp = client.get("/api/export/anomalies")
    assert resp.status_code == 200
    assert resp.headers["content-type"].startswith("text/csv")


def test_export_anomalies_with_data(client: TestClient):
    """Export anomalies CSV with data."""
    client.post("/api/anomalies", json={
        "meter_id": 99003,
        "anomaly_type": "UnexpectedSpike",
        "severity": "high",
        "current_value": 1500.0,
        "expected_value": 500.0,
        "deviation_percent": 200.0,
        "confidence": 95,
    })
    resp = client.get("/api/export/anomalies")
    assert resp.status_code == 200
    lines = resp.text.strip().split("\n")
    assert len(lines) >= 2
    assert "anomaly_type" in lines[0]
    assert "UnexpectedSpike" in lines[1]
