"""
Tests for Meter Management API endpoints.
"""


class TestRegisterMeter:
    def test_register_meter_success(self, client):
        response = client.post("/api/meters", json={
            "meter_id": 1234567890,
            "name": "Meter A1",
            "location": "Substation Jakarta Selatan",
            "firmware_version": "3.1.0",
        })
        assert response.status_code == 201
        data = response.json()
        assert data["meter_id"] == 1234567890
        assert data["name"] == "Meter A1"
        assert data["status"] == "offline"
        assert "registered_at" in data

    def test_register_meter_minimal(self, client):
        response = client.post("/api/meters", json={
            "meter_id": 9999,
        })
        assert response.status_code == 201
        data = response.json()
        assert data["name"] == ""
        assert data["firmware_version"] == "unknown"

    def test_register_duplicate_meter(self, client):
        client.post("/api/meters", json={"meter_id": 1234})
        response = client.post("/api/meters", json={"meter_id": 1234})
        assert response.status_code == 409


class TestListMeters:
    def test_list_meters_empty(self, client):
        response = client.get("/api/meters")
        assert response.status_code == 200
        assert response.json() == []

    def test_list_meters_with_data(self, client):
        client.post("/api/meters", json={"meter_id": 100, "name": "A"})
        client.post("/api/meters", json={"meter_id": 200, "name": "B"})
        response = client.get("/api/meters")
        assert response.status_code == 200
        assert len(response.json()) == 2

    def test_list_meters_filter_by_status(self, client):
        client.post("/api/meters", json={"meter_id": 100})
        response = client.get("/api/meters?status=offline")
        assert response.status_code == 200
        assert len(response.json()) == 1
        response = client.get("/api/meters?status=online")
        assert response.status_code == 200
        assert len(response.json()) == 0


class TestGetMeter:
    def test_get_meter_success(self, client):
        client.post("/api/meters", json={"meter_id": 100, "name": "Test Meter"})
        response = client.get("/api/meters/100")
        assert response.status_code == 200
        assert response.json()["name"] == "Test Meter"

    def test_get_meter_not_found(self, client):
        response = client.get("/api/meters/99999")
        assert response.status_code == 404


class TestUpdateMeter:
    def test_update_meter_name(self, client):
        client.post("/api/meters", json={"meter_id": 100, "name": "Old Name"})
        response = client.patch("/api/meters/100", json={"name": "New Name"})
        assert response.status_code == 200
        assert response.json()["name"] == "New Name"

    def test_update_meter_status(self, client):
        client.post("/api/meters", json={"meter_id": 100})
        response = client.patch("/api/meters/100", json={"status": "tampered"})
        assert response.status_code == 200
        assert response.json()["status"] == "tampered"

    def test_update_nonexistent_meter(self, client):
        response = client.patch("/api/meters/99999", json={"name": "X"})
        assert response.status_code == 404


class TestDeleteMeter:
    def test_delete_meter_success(self, client):
        client.post("/api/meters", json={"meter_id": 100})
        response = client.delete("/api/meters/100")
        assert response.status_code == 204
        # Verify it's gone
        response = client.get("/api/meters/100")
        assert response.status_code == 404

    def test_delete_nonexistent_meter(self, client):
        response = client.delete("/api/meters/99999")
        assert response.status_code == 404


class TestMeterStats:
    def test_meter_stats_success(self, client):
        # Register meter
        client.post("/api/meters", json={"meter_id": 100})
        # Add some readings
        for energy in [1000, 1100, 1050, 1020]:
            client.post("/api/meter-data", json={
                "meter_id": 100,
                "energy_wh": energy,
                "voltage_mv": 220000,
                "current_ma": 4500,
            })
        # Add an alert
        client.post("/api/tamper-alert", json={
            "meter_id": 100, "tamper_type": "CasingOpened",
        })

        response = client.get("/api/meters/100/stats")
        assert response.status_code == 200
        data = response.json()
        assert data["meter_id"] == 100
        assert data["total_readings"] == 4
        assert data["total_alerts"] == 1
        assert data["avg_energy_wh"] > 0

    def test_meter_stats_not_found(self, client):
        response = client.get("/api/meters/99999/stats")
        assert response.status_code == 404
