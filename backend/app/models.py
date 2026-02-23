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
