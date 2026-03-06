"""
Tests for server-side anomaly detection engine.
"""


class TestAnomalyDetection:
    def _seed_normal_readings(self, client, meter_id, count=5, energy=1000):
        """Helper: create multiple normal readings for a meter."""
        for _ in range(count):
            client.post("/api/meter-data", json={
                "meter_id": meter_id,
                "energy_wh": energy,
                "voltage_mv": 220000,
                "current_ma": 4545,
            })

    def test_no_anomaly_on_normal_readings(self, client):
        """Normal readings should not trigger anomaly detection."""
        self._seed_normal_readings(client, 100, count=5, energy=1000)

        response = client.get("/api/anomalies?meter_id=100")
        assert response.status_code == 200
        assert len(response.json()) == 0

    def test_anomaly_on_sudden_drop(self, client):
        """A 90% energy drop should trigger an UnexpectedDrop anomaly."""
        # Establish baseline: 5 readings at 1000 Wh
        self._seed_normal_readings(client, 200, count=5, energy=1000)

        # Send anomalous reading: 100 Wh (90% drop)
        client.post("/api/meter-data", json={
            "meter_id": 200,
            "energy_wh": 100,
            "voltage_mv": 220000,
            "current_ma": 500,
        })

        response = client.get("/api/anomalies?meter_id=200")
        assert response.status_code == 200
        anomalies = response.json()
        assert len(anomalies) >= 1
        anomaly = anomalies[0]
        assert anomaly["anomaly_type"] == "UnexpectedDrop"
        assert anomaly["severity"] in ("critical", "high")
        assert anomaly["deviation_percent"] > 60.0

    def test_anomaly_on_sudden_spike(self, client):
        """A 200% energy spike should trigger an UnexpectedSpike anomaly."""
        self._seed_normal_readings(client, 300, count=5, energy=1000)

        # Send spike: 3000 Wh (200% above average)
        client.post("/api/meter-data", json={
            "meter_id": 300,
            "energy_wh": 3000,
            "voltage_mv": 220000,
            "current_ma": 13000,
        })

        response = client.get("/api/anomalies?meter_id=300")
        assert response.status_code == 200
        anomalies = response.json()
        assert len(anomalies) >= 1
        assert anomalies[0]["anomaly_type"] == "UnexpectedSpike"

    def test_no_anomaly_with_insufficient_data(self, client):
        """Should not trigger anomaly if fewer than 3 historical readings exist."""
        # Only 2 readings — too few for comparison
        client.post("/api/meter-data", json={
            "meter_id": 400, "energy_wh": 1000,
            "voltage_mv": 220000, "current_ma": 4545,
        })
        client.post("/api/meter-data", json={
            "meter_id": 400, "energy_wh": 100,  # big drop but insufficient history
            "voltage_mv": 220000, "current_ma": 500,
        })

        response = client.get("/api/anomalies?meter_id=400")
        assert response.status_code == 200
        assert len(response.json()) == 0

    def test_anomaly_severity_classification(self, client):
        """Critical severity for >= 80% deviation."""
        self._seed_normal_readings(client, 500, count=5, energy=1000)

        # 95% drop: energy 50
        client.post("/api/meter-data", json={
            "meter_id": 500, "energy_wh": 50,
            "voltage_mv": 220000, "current_ma": 200,
        })

        response = client.get("/api/anomalies?meter_id=500")
        anomalies = response.json()
        assert len(anomalies) >= 1
        assert anomalies[0]["severity"] == "critical"
        assert anomalies[0]["confidence"] >= 80
