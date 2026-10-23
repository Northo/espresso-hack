"""FastAPI application. Run with uvicorn espresso_api.app:app."""

import os
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager
from pathlib import Path
from typing import Annotated
from uuid import UUID

from fastapi import FastAPI, Query, Request, Response
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from fastapi.staticfiles import StaticFiles
from pydantic import AwareDatetime
from starlette.exceptions import HTTPException as StarletteHTTPException

from . import store, web
from .db import Database
from .dependencies import DB
from .middleware import BodyLimitMiddleware
from .models import (
    Coffee,
    CoffeeCreate,
    CoffeePage,
    CoffeePatch,
    Setup,
    SetupWrite,
    Shot,
    ShotCreate,
    ShotPage,
    ShotPatch,
)

Limit = Annotated[int, Query(ge=1, le=100)]
Offset = Annotated[int, Query(ge=0)]


def create_app(database_path: str | Path | None = None) -> FastAPI:
    database = Database(
        database_path
        if database_path is not None
        else os.environ.get("ESPRESSO_DB_PATH", "data/espresso.sqlite3")
    )

    @asynccontextmanager
    async def lifespan(application: FastAPI) -> AsyncIterator[None]:
        database.initialize()
        yield

    application = FastAPI(
        title="Espresso Shot Journal",
        version="0.1.0",
        description="Trusted-LAN API for coffee, the current setup, and shot history.",
        lifespan=lifespan,
    )
    application.state.database = database
    application.add_middleware(BodyLimitMiddleware)
    application.mount(
        "/static", StaticFiles(directory=Path(__file__).with_name("static")), name="static"
    )
    application.include_router(web.router)

    @application.exception_handler(RequestValidationError)
    async def validation_error(request: Request, exc: RequestValidationError) -> Response:
        if web.is_web_path(request.url.path):
            return web.error_page(
                request, 422, "Check the address and filter values, then try again."
            )
        details = [{key: error[key] for key in ("type", "loc", "msg")} for error in exc.errors()]
        return JSONResponse({"detail": details}, status_code=422)

    @application.exception_handler(StarletteHTTPException)
    async def http_error(request: Request, exc: StarletteHTTPException) -> Response:
        if web.is_web_path(request.url.path):
            return web.error_page(request, exc.status_code, str(exc.detail))
        return JSONResponse(
            {"detail": exc.detail}, status_code=exc.status_code, headers=exc.headers
        )

    @application.get("/api/v1/health", tags=["health"])
    def health(db: DB) -> dict[str, str]:
        db.execute("SELECT 1").fetchone()
        return {"status": "ok"}

    @application.post("/api/v1/coffees", response_model=Coffee, status_code=201, tags=["coffees"])
    def create_coffee(payload: CoffeeCreate, response: Response, db: DB) -> Coffee:
        result = store.create_coffee(db, payload)
        response.headers["Location"] = f"/api/v1/coffees/{result.id}"
        return result

    @application.get("/api/v1/coffees", response_model=CoffeePage, tags=["coffees"])
    def list_coffees(
        db: DB,
        limit: Limit = 50,
        offset: Offset = 0,
        include_archived: bool = False,
    ) -> CoffeePage:
        return store.list_coffees(db, limit, offset, include_archived)

    @application.get("/api/v1/coffees/{coffee_id}", response_model=Coffee, tags=["coffees"])
    def get_coffee(coffee_id: UUID, db: DB) -> Coffee:
        return store.get_coffee(db, coffee_id)

    @application.patch("/api/v1/coffees/{coffee_id}", response_model=Coffee, tags=["coffees"])
    def patch_coffee(coffee_id: UUID, payload: CoffeePatch, db: DB) -> Coffee:
        return store.patch_coffee(db, coffee_id, payload)

    @application.get("/api/v1/setup", response_model=Setup, tags=["setup"])
    def get_setup(db: DB) -> Setup:
        return store.get_setup(db)

    @application.put("/api/v1/setup", response_model=Setup, tags=["setup"])
    def set_setup(payload: SetupWrite, db: DB) -> Setup:
        """Replace the current selection and save an immutable revision."""
        return store.set_setup(db, payload)

    @application.post("/api/v1/shots", response_model=Shot, status_code=201, tags=["shots"])
    def create_shot(payload: ShotCreate, response: Response, db: DB) -> Shot:
        """Submit a completed shot using the setup revision captured at brew start."""
        result, created = store.create_shot(db, payload)
        if not created:
            response.status_code = 200
        response.headers["Location"] = f"/api/v1/shots/{payload.id}"
        return result

    @application.get("/api/v1/shots", response_model=ShotPage, tags=["shots"])
    def list_shots(
        db: DB,
        limit: Limit = 50,
        offset: Offset = 0,
        coffee_id: UUID | None = None,
        since: Annotated[
            AwareDatetime | None,
            Query(description="Inclusive lower bound on server receipt time (timezone required)."),
        ] = None,
        until: Annotated[
            AwareDatetime | None,
            Query(description="Exclusive upper bound on server receipt time (timezone required)."),
        ] = None,
    ) -> ShotPage:
        return store.list_shots(db, limit, offset, coffee_id, since, until)

    @application.get("/api/v1/shots/{shot_id}", response_model=Shot, tags=["shots"])
    def get_shot(shot_id: UUID, db: DB) -> Shot:
        return store.get_shot(db, shot_id)

    @application.patch("/api/v1/shots/{shot_id}", response_model=Shot, tags=["shots"])
    def patch_shot(shot_id: UUID, payload: ShotPatch, db: DB) -> Shot:
        return store.patch_shot(db, shot_id, payload)

    return application


app = create_app()
