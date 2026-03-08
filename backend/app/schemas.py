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


# ============================================================================
# Meter
# ============================================================================
class MeterCreate(BaseModel):
    meter_id: int = Field(..., description="Unique meter hardware identifier")
    name: str = Field(default="", max_length=100)
    location: str = Field(default="", max_length=200)
    firmware_version: str = Field(default="unknown", max_length=20)


class MeterUpdate(BaseModel):
    name: str | None = Field(default=None, max_length=100)
    location: str | None = Field(default=None, max_length=200)
    firmware_version: str | None = Field(default=None, max_length=20)
    status: str | None = Field(default=None, max_length=20)


class MeterResponse(BaseModel):
    id: int
    meter_id: int
    name: str
    location: str
    firmware_version: str
    status: str
    registered_at: datetime
    last_seen_at: datetime | None = None

    model_config = {"from_attributes": True}


class MeterStats(BaseModel):
    meter_id: int
    total_readings: int
    total_alerts: int
    total_anomalies: int
    avg_energy_wh: float
    avg_voltage_mv: float
    avg_current_ma: float
    last_reading_time: datetime | None = None
<<<<<<< HEAD


# ============================================================================
# Notification
# ============================================================================
class NotificationResponse(BaseModel):
    id: int
    timestamp: datetime
    notification_type: str
    severity: str
    message: str
    meter_id: int | None = None
    is_read: bool
    source_id: int | None = None

    model_config = {"from_attributes": True}


class NotificationSummary(BaseModel):
    unread_count: int
    recent: list[NotificationResponse] = []


# ============================================================================
# Webhook Config
# ============================================================================
class WebhookConfigCreate(BaseModel):
    url: str = Field(..., max_length=500)
    secret: str = Field(default="", max_length=100)
    enabled: bool = True
    event_types: str = Field(default="all", max_length=200)
    description: str = Field(default="", max_length=200)


class WebhookConfigResponse(BaseModel):
    id: int
    url: str
    secret: str
    enabled: bool
    event_types: str
    created_at: datetime
    description: str

    model_config = {"from_attributes": True}


class WebhookTestResult(BaseModel):
    success: bool
    status_code: int | None = None
    message: str = ""


# ============================================================================
# Forensics Report
# ============================================================================
class ForensicsReportCreate(BaseModel):
    meter_id: int
    report_type: str = Field(..., max_length=30)
    severity: str = Field(default="low", max_length=20)
    confidence: int = Field(default=0, ge=0, le=100)
    event_count: int = Field(default=0, ge=0)
    summary: str = Field(default="", max_length=1000)
    raw_payload: str = Field(default="{}", max_length=5000)


class ForensicsReportResponse(BaseModel):
    id: int
    meter_id: int
    timestamp: datetime
    report_type: str
    severity: str
    confidence: int
    event_count: int
    summary: str
    raw_payload: str

    model_config = {"from_attributes": True}
=======
>>>>>>> origin/main
