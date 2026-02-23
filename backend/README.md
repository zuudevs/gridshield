# GridShield Backend

REST API for GridShield AMI Security System — meter data ingestion, tamper alerts, and anomaly monitoring.

## Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Run development server
uvicorn app.main:app --reload --port 8000
```

## API Endpoints

| Method  | Endpoint                          | Description                |
|---------|-----------------------------------|----------------------------|
| `GET`   | `/`                               | Health check               |
| `POST`  | `/api/meter-data`                 | Submit meter reading       |
| `GET`   | `/api/readings`                   | List recent readings       |
| `POST`  | `/api/tamper-alert`               | Submit tamper alert        |
| `GET`   | `/api/alerts`                     | List tamper alerts         |
| `PATCH` | `/api/alerts/{id}/acknowledge`    | Acknowledge an alert       |
| `POST`  | `/api/anomalies`                  | Log anomaly event          |
| `GET`   | `/api/anomalies`                  | List anomaly logs          |
| `GET`   | `/api/status`                     | System status summary      |

## Interactive Docs

After starting the server, visit:

- **Swagger UI:** [http://localhost:8000/docs](http://localhost:8000/docs)
- **ReDoc:** [http://localhost:8000/redoc](http://localhost:8000/redoc)

## Example: Submit Reading

```bash
curl -X POST http://localhost:8000/api/meter-data \
  -H "Content-Type: application/json" \
  -d '{
    "meter_id": 1311768467294899695,
    "energy_wh": 1200,
    "voltage_mv": 220000,
    "current_ma": 500,
    "power_factor": 950
  }'
```

## Tech Stack

- **Framework:** FastAPI
- **Database:** SQLite (SQLAlchemy ORM)
- **Validation:** Pydantic v2
- **Server:** Uvicorn (ASGI)
