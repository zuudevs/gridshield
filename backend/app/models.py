"""
GridShield Backend — Database Models
"""

from datetime import datetime

from sqlalchemy import Column, Integer, BigInteger, String, Float, DateTime, Boolean

from .database import Base


class MeterReading(Base):
    """Meter reading received from ESP32 device."""

    __tablename__ = "meter_readings"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    meter_id = Column(BigInteger, nullable=False, index=True)
    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False)
    energy_wh = Column(Integer, nullable=False)
    voltage_mv = Column(Integer, nullable=False)
    current_ma = Column(Integer, nullable=False)
    power_factor = Column(Integer, default=0)
    phase = Column(Integer, default=0)


class TamperAlert(Base):
    """Tamper alert received from ESP32 device."""

    __tablename__ = "tamper_alerts"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    meter_id = Column(BigInteger, nullable=False, index=True)
    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False)
    tamper_type = Column(String(50), nullable=False)
    severity = Column(Integer, default=0)
    acknowledged = Column(Boolean, default=False)


class AnomalyLog(Base):
    """Anomaly detection log."""

    __tablename__ = "anomaly_logs"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    meter_id = Column(BigInteger, nullable=False, index=True)
    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False)
    anomaly_type = Column(String(50), nullable=False)
    severity = Column(String(20), nullable=False)
    current_value = Column(Float, default=0.0)
    expected_value = Column(Float, default=0.0)
    deviation_percent = Column(Float, default=0.0)
    confidence = Column(Integer, default=0)


class Meter(Base):
    """Registered meter device."""

    __tablename__ = "meters"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    meter_id = Column(BigInteger, unique=True, nullable=False, index=True)
    name = Column(String(100), default="")
    location = Column(String(200), default="")
    firmware_version = Column(String(20), default="unknown")
    status = Column(String(20), default="offline")  # online / offline / tampered
    registered_at = Column(DateTime, default=datetime.utcnow, nullable=False)
    last_seen_at = Column(DateTime, default=None, nullable=True)


class Notification(Base):
    """In-app notification generated from alerts and anomalies."""

    __tablename__ = "notifications"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False)
    notification_type = Column(String(30), nullable=False)  # tamper_alert / anomaly / forensics
    severity = Column(String(20), default="info")  # info / low / medium / high / critical
    message = Column(String(500), nullable=False)
    meter_id = Column(BigInteger, nullable=True, index=True)
    is_read = Column(Boolean, default=False)
    source_id = Column(Integer, nullable=True)  # FK-like reference to alert/anomaly/report id


class WebhookConfig(Base):
    """External webhook endpoint configuration."""

    __tablename__ = "webhook_configs"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    url = Column(String(500), nullable=False)
    secret = Column(String(100), default="")
    enabled = Column(Boolean, default=True)
    event_types = Column(String(200), default="all")  # comma-separated: all, tamper_alert, anomaly, forensics
    created_at = Column(DateTime, default=datetime.utcnow, nullable=False)
    description = Column(String(200), default="")


class ForensicsReport(Base):
    """Forensic incident report from firmware."""

    __tablename__ = "forensics_reports"

    id = Column(Integer, primary_key=True, index=True, autoincrement=True)
    meter_id = Column(BigInteger, nullable=False, index=True)
    timestamp = Column(DateTime, default=datetime.utcnow, nullable=False)
    report_type = Column(String(30), nullable=False)  # physical / network / fraud / hybrid
    severity = Column(String(20), default="low")
    confidence = Column(Integer, default=0)
    event_count = Column(Integer, default=0)
    summary = Column(String(1000), default="")
    raw_payload = Column(String(5000), default="{}")
