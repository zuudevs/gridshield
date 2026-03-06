"""
Tests for Tamper Alert API endpoints.
"""


class TestCreateAlert:
    def test_create_alert_success(self, client):
        response = client.post("/api/tamper-alert", json={
            "meter_id": 1234567890,
            "tamper_type": "CasingOpened",
            "severity": 3,
        })
        assert response.status_code == 201
        data = response.json()
        assert data["meter_id"] == 1234567890
        assert data["tamper_type"] == "CasingOpened"
        assert data["severity"] == 3
        assert data["acknowledged"] is False

    def test_create_alert_default_severity(self, client):
        response = client.post("/api/tamper-alert", json={
            "meter_id": 999,
            "tamper_type": "MagneticInterference",
        })
        assert response.status_code == 201
        assert response.json()["severity"] == 0


class TestListAlerts:
    def test_list_alerts_empty(self, client):
        response = client.get("/api/alerts")
        assert response.status_code == 200
        assert response.json() == []

    def test_list_alerts_with_data(self, client):
        client.post("/api/tamper-alert", json={
            "meter_id": 100, "tamper_type": "CasingOpened", "severity": 2,
        })
        client.post("/api/tamper-alert", json={
            "meter_id": 200, "tamper_type": "PowerCutAttempt", "severity": 4,
        })
        response = client.get("/api/alerts")
        assert response.status_code == 200
        assert len(response.json()) == 2

    def test_list_alerts_filter_by_acknowledged(self, client):
        client.post("/api/tamper-alert", json={
            "meter_id": 100, "tamper_type": "CasingOpened",
        })
        response = client.get("/api/alerts?acknowledged=false")
        assert response.status_code == 200
        assert len(response.json()) == 1


class TestAcknowledgeAlert:
    def test_acknowledge_alert_success(self, client):
        # Create alert
        create_resp = client.post("/api/tamper-alert", json={
            "meter_id": 100, "tamper_type": "CasingOpened",
        })
        alert_id = create_resp.json()["id"]

        # Acknowledge it
        response = client.patch(f"/api/alerts/{alert_id}/acknowledge")
        assert response.status_code == 200
        assert response.json()["acknowledged"] is True

    def test_acknowledge_nonexistent_alert(self, client):
        response = client.patch("/api/alerts/99999/acknowledge")
        assert response.status_code == 404
