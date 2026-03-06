"""
Tests for Meter Reading API endpoints.
"""


class TestCreateReading:
    def test_create_reading_success(self, client):
        response = client.post("/api/meter-data", json={
            "meter_id": 1234567890,
            "energy_wh": 1000,
            "voltage_mv": 220000,
            "current_ma": 4545,
        })
        assert response.status_code == 201
        data = response.json()
        assert data["meter_id"] == 1234567890
        assert data["energy_wh"] == 1000
        assert data["voltage_mv"] == 220000
        assert "timestamp" in data

    def test_create_reading_with_optional_fields(self, client):
        response = client.post("/api/meter-data", json={
            "meter_id": 9999,
            "energy_wh": 500,
            "voltage_mv": 230000,
            "current_ma": 2000,
            "power_factor": 950,
            "phase": 1,
        })
        assert response.status_code == 201
        data = response.json()
        assert data["power_factor"] == 950
        assert data["phase"] == 1

    def test_create_reading_missing_required_field(self, client):
        response = client.post("/api/meter-data", json={
            "meter_id": 1234,
            "energy_wh": 1000,
            # missing voltage_mv and current_ma
        })
        assert response.status_code == 422

    def test_create_reading_negative_energy(self, client):
        response = client.post("/api/meter-data", json={
            "meter_id": 1234,
            "energy_wh": -100,
            "voltage_mv": 220000,
            "current_ma": 1000,
        })
        assert response.status_code == 422


class TestListReadings:
    def test_list_readings_empty(self, client):
        response = client.get("/api/readings")
        assert response.status_code == 200
        assert response.json() == []

    def test_list_readings_with_data(self, client):
        # Create two readings
        client.post("/api/meter-data", json={
            "meter_id": 100, "energy_wh": 500,
            "voltage_mv": 220000, "current_ma": 2000,
        })
        client.post("/api/meter-data", json={
            "meter_id": 200, "energy_wh": 1000,
            "voltage_mv": 230000, "current_ma": 4000,
        })
        response = client.get("/api/readings")
        assert response.status_code == 200
        data = response.json()
        assert len(data) == 2

    def test_list_readings_filter_by_meter(self, client):
        client.post("/api/meter-data", json={
            "meter_id": 100, "energy_wh": 500,
            "voltage_mv": 220000, "current_ma": 2000,
        })
        client.post("/api/meter-data", json={
            "meter_id": 200, "energy_wh": 1000,
            "voltage_mv": 230000, "current_ma": 4000,
        })
        response = client.get("/api/readings?meter_id=100")
        assert response.status_code == 200
        data = response.json()
        assert len(data) == 1
        assert data[0]["meter_id"] == 100

    def test_list_readings_limit(self, client):
        for i in range(5):
            client.post("/api/meter-data", json={
                "meter_id": 100, "energy_wh": i * 100,
                "voltage_mv": 220000, "current_ma": 2000,
            })
        response = client.get("/api/readings?limit=3")
        assert response.status_code == 200
        assert len(response.json()) == 3
