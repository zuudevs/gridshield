"""
GridShield Backend — Pydantic Schemas
Request/Response validation
"""

from datetime import datetime

from pydantic import BaseModel, Field


# ============================================================================
# Meter Reading
# ============================================================================
class MeterReadingCreate(BaseModel):
    meter_id: int = Field(..., description="Unique meter identifier")
    energy_wh: int = Field(..., ge=0, description="Energy in watt-hours")
    voltage_mv: int = Field(..., ge=0, description="Voltage in millivolts")
    current_ma: int = Field(..., ge=0, description="Current in milliamps")
    power_factor: int = Field(default=0, ge=0, le=1000, description="Power factor (0-1000)")
    phase: int = Field(default=0, ge=0, le=3)


class MeterReadingResponse(BaseModel):
    id: int
    meter_id: int
    timestamp: datetime
    energy_wh: int
    voltage_mv: int
    current_ma: int
    power_factor: int
    phase: int

    model_config = {"from_attributes": True}


# ============================================================================
# Tamper Alert
# ============================================================================
class TamperAlertCreate(BaseModel):
    meter_id: int
    tamper_type: str = Field(..., max_length=50)
    severity: int = Field(default=0, ge=0, le=4)


class TamperAlertResponse(BaseModel):
    id: int
    meter_id: int
    timestamp: datetime
    tamper_type: str
    severity: int
    acknowledged: bool

    model_config = {"from_attributes": True}


# ============================================================================
# Anomaly Log
# ============================================================================
class AnomalyLogCreate(BaseModel):
    meter_id: int
    anomaly_type: str = Field(..., max_length=50)
    severity: str = Field(..., max_length=20)
    current_value: float = 0.0
    expected_value: float = 0.0
    deviation_percent: float = 0.0
    confidence: int = Field(default=0, ge=0, le=100)


class AnomalyLogResponse(BaseModel):
    id: int
    meter_id: int
    timestamp: datetime
    anomaly_type: str
    severity: str
    current_value: float
    expected_value: float
    deviation_percent: float
    confidence: int

    model_config = {"from_attributes": True}


# ============================================================================
# Status
# ============================================================================
class SystemStatus(BaseModel):
    total_readings: int
    total_alerts: int
    total_anomalies: int
    active_meters: int
    unacknowledged_alerts: int
    latest_reading_time: datetime | None = None
