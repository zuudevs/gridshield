"""
Tests for forensics report endpoints.
"""

from fastapi.testclient import TestClient


SAMPLE_REPORT = {
    "meter_id": 77001,
    "report_type": "physical",
    "severity": "high",
    "confidence": 85,
    "event_count": 5,
    "summary": "Multiple casing tamper events detected over 24h period",
    "raw_payload": '{"events": [{"type": "CasingOpened", "count": 3}]}',
}


def test_create_forensics_report(client: TestClient):
    """Create a forensics report."""
    resp = client.post("/api/forensics/reports", json=SAMPLE_REPORT)
    assert resp.status_code == 201
    data = resp.json()
    assert data["meter_id"] == 77001
    assert data["report_type"] == "physical"
    assert data["confidence"] == 85


def test_list_forensics_reports(client: TestClient):
    """List forensics reports."""
    client.post("/api/forensics/reports", json=SAMPLE_REPORT)
    resp = client.get("/api/forensics/reports")
    assert resp.status_code == 200
    assert len(resp.json()) >= 1


def test_get_forensics_report_by_id(client: TestClient):
    """Get a single forensics report by ID."""
    create_resp = client.post("/api/forensics/reports", json=SAMPLE_REPORT)
    report_id = create_resp.json()["id"]

    resp = client.get(f"/api/forensics/reports/{report_id}")
    assert resp.status_code == 200
    assert resp.json()["id"] == report_id
    assert resp.json()["summary"] == SAMPLE_REPORT["summary"]


def test_get_forensics_report_not_found(client: TestClient):
    """Get non-existent report returns 404."""
    resp = client.get("/api/forensics/reports/9999")
    assert resp.status_code == 404


def test_filter_forensics_reports_by_meter(client: TestClient):
    """Filter forensics reports by meter_id."""
    client.post("/api/forensics/reports", json=SAMPLE_REPORT)
    client.post("/api/forensics/reports", json={
        **SAMPLE_REPORT,
        "meter_id": 77002,
    })

    resp = client.get("/api/forensics/reports?meter_id=77001")
    assert resp.status_code == 200
    reports = resp.json()
    assert all(r["meter_id"] == 77001 for r in reports)


def test_filter_forensics_reports_by_type(client: TestClient):
    """Filter forensics reports by report_type."""
    client.post("/api/forensics/reports", json=SAMPLE_REPORT)
    client.post("/api/forensics/reports", json={
        **SAMPLE_REPORT,
        "report_type": "network",
    })

    resp = client.get("/api/forensics/reports?report_type=physical")
    assert resp.status_code == 200
    reports = resp.json()
    assert all(r["report_type"] == "physical" for r in reports)
