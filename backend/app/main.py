"""
GridShield Backend — FastAPI Application
REST API for AMI Security Data Ingestion & Monitoring
"""

from contextlib import asynccontextmanager

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .database import Base, engine
<<<<<<< HEAD
from .export import router as export_router
from .forensics import router as forensics_router
from .meters import router as meters_router
from .notifications import router as notifications_router
=======
<<<<<<< HEAD
from .export import router as export_router
=======
>>>>>>> 469b660da70c38354fe5127353f451559b605a7f
from .meters import router as meters_router
>>>>>>> origin/main
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
<<<<<<< HEAD
    version="3.3.0",
=======
<<<<<<< HEAD
    version="3.2.0",
=======
    version="3.1.0",
>>>>>>> 469b660da70c38354fe5127353f451559b605a7f
>>>>>>> origin/main
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
<<<<<<< HEAD
app.include_router(export_router)
app.include_router(notifications_router)
app.include_router(webhooks_router)
app.include_router(forensics_router)
=======
<<<<<<< HEAD
app.include_router(export_router)
=======
>>>>>>> 469b660da70c38354fe5127353f451559b605a7f
>>>>>>> origin/main


@app.get("/", tags=["Root"])
def root():
    """Health check."""
    return {
        "name": "GridShield API",
<<<<<<< HEAD
        "version": "3.3.0",
=======
<<<<<<< HEAD
        "version": "3.2.0",
=======
        "version": "3.1.0",
>>>>>>> 469b660da70c38354fe5127353f451559b605a7f
>>>>>>> origin/main
        "status": "running",
    }
