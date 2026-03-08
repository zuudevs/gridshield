"""
GridShield Backend — FastAPI Application
REST API for AMI Security Data Ingestion & Monitoring
"""

from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .database import Base, engine
from .export import router as export_router
from .forensics import router as forensics_router
from .meters import router as meters_router
from .notifications import router as notifications_router
from .routes import router
from .webhooks import router as webhooks_router


@asynccontextmanager
async def lifespan(app: FastAPI):
    """Create database tables on startup."""
    Base.metadata.create_all(bind=engine)
    yield


app = FastAPI(
    title="GridShield API",
    description="REST API for GridShield AMI Security System — "
                "Meter data ingestion, tamper alerts, anomaly monitoring, "
                "and fleet management.",
    version="3.3.0",
    lifespan=lifespan,
)

# CORS — allow frontend dev server
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)

# Routes
app.include_router(router)
app.include_router(meters_router)
app.include_router(export_router)
app.include_router(notifications_router)
app.include_router(webhooks_router)
app.include_router(forensics_router)


@app.get("/", tags=["Root"])
def root():
    """Health check."""
    return {
        "name": "GridShield API",
        "version": "3.3.0",
        "status": "running",
    }
