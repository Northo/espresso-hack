"""FastAPI application. Run with uvicorn espresso_api.app:app."""

import json
import os
import sqlite3
from collections.abc import AsyncIterator, Iterator
from contextlib import asynccontextmanager
from datetime import UTC, datetime
from pathlib import Path
from typing import Annotated
from uuid import UUID, uuid4

from fastapi import Depends, FastAPI, HTTPException, Query, Request, Response
from fastapi.exceptions import RequestValidationError
from fastapi.responses import JSONResponse
from pydantic import AwareDatetime

from .db import Database
from .middleware import BodyLimitMiddleware
from .models import (
    Coffee,
    CoffeeCreate,
    CoffeePage,
    CoffeePatch,
    CoffeeSnapshot,
    Setup,
    SetupWrite,
    Shot,
    ShotCreate,
    ShotPage,
    ShotPatch,
)


def timestamp(value: datetime | None = None) -> str:
    return (
        (value or datetime.now(UTC))
        .astimezone(UTC)
        .isoformat(timespec="microseconds")
        .replace("+00:00", "Z")
    )


def connection(request: Request) -> Iterator[sqlite3.Connection]:
    with request.app.state.database.connect() as db:
        yield db


DB = Annotated[sqlite3.Connection, Depends(connection)]
Limit = Annotated[int, Query(ge=1, le=100)]
Offset = Annotated[int, Query(ge=0)]


def require_row(db: sqlite3.Connection, table: str, key: str, value: str | int) -> sqlite3.Row:
    # Table/key are internal constants, never request parameters.
    row = db.execute(f"SELECT * FROM {table} WHERE {key} = ?", (value,)).fetchone()
    if row is None:
        raise HTTPException(404, f"{table.rstrip('s').replace('_', ' ')} not found")
    return row


def read_setup(row: sqlite3.Row) -> Setup:
    return Setup(
        revision=row["revision"],
        coffee=json.loads(row["coffee_snapshot"]) if row["coffee_snapshot"] else None,
        grind_setting=row["grind_setting"],
        dose_g=row["dose_g"],
        target_yield_g=row["target_yield_g"],
        updated_at=row["updated_at"],
    )


def read_shot(db: sqlite3.Connection, row: sqlite3.Row) -> Shot:
    values = dict(row)
    revision = values.pop("setup_revision")
    values.pop("creation_request")
    setup = read_setup(require_row(db, "setup_revisions", "revision", revision))
    return Shot(setup=setup, **values)


def update_row(db: sqlite3.Connection, table: str, row_id: str, values: dict) -> None:
    if not values:
        return
    # Keys originate only from validated model fields / server-generated fields.
    assignments = ", ".join(f"{name} = ?" for name in values)
    db.execute(f"UPDATE {table} SET {assignments} WHERE id = ?", (*values.values(), row_id))


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

    @application.exception_handler(RequestValidationError)
    async def validation_error(request: Request, exc: RequestValidationError) -> JSONResponse:
        # Do not echo raw input/context: these can contain NaN/Infinity or exceptions,
        # which the default JSON error renderer cannot serialize.
        details = [{key: error[key] for key in ("type", "loc", "msg")} for error in exc.errors()]
        return JSONResponse({"detail": details}, status_code=422)

    @application.get("/api/v1/health", tags=["health"])
    def health(db: DB) -> dict[str, str]:
        db.execute("SELECT 1").fetchone()
        return {"status": "ok"}

    @application.post("/api/v1/coffees", response_model=Coffee, status_code=201, tags=["coffees"])
    def create_coffee(payload: CoffeeCreate, response: Response, db: DB) -> Coffee:
        values = payload.model_dump(mode="json")
        values.update(id=str(uuid4()), archived=False, created_at=timestamp())
        values["updated_at"] = values["created_at"]
        with db:
            db.execute(
                f"INSERT INTO coffees ({', '.join(values)}) "
                f"VALUES ({', '.join('?' for _ in values)})",
                tuple(values.values()),
            )
        response.headers["Location"] = f"/api/v1/coffees/{values['id']}"
        return Coffee(**values)

    @application.get("/api/v1/coffees", response_model=CoffeePage, tags=["coffees"])
    def list_coffees(
        db: DB,
        limit: Limit = 50,
        offset: Offset = 0,
        include_archived: bool = False,
    ) -> CoffeePage:
        where = "" if include_archived else "WHERE archived = 0"
        with db:
            db.execute("BEGIN")  # Count and page share a read snapshot.
            total = db.execute(f"SELECT COUNT(*) FROM coffees {where}").fetchone()[0]
            rows = db.execute(
                f"SELECT * FROM coffees {where} ORDER BY name COLLATE NOCASE, id LIMIT ? OFFSET ?",
                (limit, offset),
            ).fetchall()
        return CoffeePage(
            items=[Coffee(**dict(row)) for row in rows],
            total=total,
            limit=limit,
            offset=offset,
        )

    @application.get("/api/v1/coffees/{coffee_id}", response_model=Coffee, tags=["coffees"])
    def get_coffee(coffee_id: UUID, db: DB) -> Coffee:
        return Coffee(**dict(require_row(db, "coffees", "id", str(coffee_id))))

    @application.patch("/api/v1/coffees/{coffee_id}", response_model=Coffee, tags=["coffees"])
    def patch_coffee(coffee_id: UUID, payload: CoffeePatch, db: DB) -> Coffee:
        with db:
            db.execute("BEGIN IMMEDIATE")
            require_row(db, "coffees", "id", str(coffee_id))
            values = payload.model_dump(mode="json", exclude_unset=True)
            if values:
                values["updated_at"] = timestamp()
                update_row(db, "coffees", str(coffee_id), values)
            row = require_row(db, "coffees", "id", str(coffee_id))
        return Coffee(**dict(row))

    @application.get("/api/v1/setup", response_model=Setup, tags=["setup"])
    def get_setup(db: DB) -> Setup:
        return read_setup(
            db.execute("SELECT * FROM setup_revisions ORDER BY revision DESC LIMIT 1").fetchone()
        )

    @application.put("/api/v1/setup", response_model=Setup, tags=["setup"])
    def set_setup(payload: SetupWrite, db: DB) -> Setup:
        """Replace the current selection and save an immutable revision."""
        with db:
            db.execute("BEGIN IMMEDIATE")
            coffee_snapshot = None
            if payload.coffee_id is not None:
                coffee = require_row(db, "coffees", "id", str(payload.coffee_id))
                if coffee["archived"]:
                    raise HTTPException(409, "Archived coffee cannot be selected")
                coffee_snapshot = CoffeeSnapshot(
                    **{name: coffee[name] for name in CoffeeSnapshot.model_fields}
                ).model_dump_json()
            cursor = db.execute(
                """INSERT INTO setup_revisions
                (coffee_id, coffee_snapshot, grind_setting, dose_g, target_yield_g, updated_at)
                VALUES (?, ?, ?, ?, ?, ?)""",
                (
                    str(payload.coffee_id) if payload.coffee_id else None,
                    coffee_snapshot,
                    payload.grind_setting,
                    payload.dose_g,
                    payload.target_yield_g,
                    timestamp(),
                ),
            )
            row = require_row(db, "setup_revisions", "revision", cursor.lastrowid)
        return read_setup(row)

    @application.post("/api/v1/shots", response_model=Shot, status_code=201, tags=["shots"])
    def create_shot(payload: ShotCreate, response: Response, db: DB) -> Shot:
        """Submit a completed shot using the setup revision captured at brew start."""
        values = payload.model_dump(mode="json")
        original_request = json.dumps(values, sort_keys=True, separators=(",", ":"))
        with db:
            # Serialize duplicate checks and inserts (including concurrent retries).
            db.execute("BEGIN IMMEDIATE")
            existing = db.execute("SELECT * FROM shots WHERE id = ?", (str(payload.id),)).fetchone()
            if existing is not None:
                if existing["creation_request"] != original_request:
                    raise HTTPException(409, "Shot ID already exists with a different payload")
                response.status_code = 200
                result = read_shot(db, existing)
            else:
                setup = read_setup(
                    require_row(db, "setup_revisions", "revision", payload.setup_revision)
                )
                if values["dose_g"] is None:
                    values["dose_g"] = setup.dose_g
                values.update(received_at=timestamp(), creation_request=original_request)
                db.execute(
                    f"INSERT INTO shots ({', '.join(values)}) "
                    f"VALUES ({', '.join('?' for _ in values)})",
                    tuple(values.values()),
                )
                result = read_shot(db, require_row(db, "shots", "id", str(payload.id)))
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
        if since and until and since >= until:
            raise HTTPException(422, "since must be earlier than until")
        clauses, parameters = [], []
        if coffee_id is not None:
            clauses.append("setup_revisions.coffee_id = ?")
            parameters.append(str(coffee_id))
        if since is not None:
            clauses.append("shots.received_at >= ?")
            parameters.append(timestamp(since))
        if until is not None:
            clauses.append("shots.received_at < ?")
            parameters.append(timestamp(until))
        where = "WHERE " + " AND ".join(clauses) if clauses else ""
        source = (
            "FROM shots JOIN setup_revisions ON shots.setup_revision = setup_revisions.revision"
        )
        with db:
            db.execute("BEGIN")
            total = db.execute(
                f"SELECT COUNT(*) {source} {where}",
                parameters,
            ).fetchone()[0]
            rows = db.execute(
                f"SELECT shots.* {source} {where} "
                "ORDER BY shots.received_at DESC, shots.id DESC LIMIT ? OFFSET ?",
                (*parameters, limit, offset),
            ).fetchall()
            items = [read_shot(db, row) for row in rows]
        return ShotPage(items=items, total=total, limit=limit, offset=offset)

    @application.get("/api/v1/shots/{shot_id}", response_model=Shot, tags=["shots"])
    def get_shot(shot_id: UUID, db: DB) -> Shot:
        return read_shot(db, require_row(db, "shots", "id", str(shot_id)))

    @application.patch("/api/v1/shots/{shot_id}", response_model=Shot, tags=["shots"])
    def patch_shot(shot_id: UUID, payload: ShotPatch, db: DB) -> Shot:
        with db:
            db.execute("BEGIN IMMEDIATE")
            require_row(db, "shots", "id", str(shot_id))
            update_row(db, "shots", str(shot_id), payload.model_dump(exclude_unset=True))
            result = read_shot(db, require_row(db, "shots", "id", str(shot_id)))
        return result

    return application


app = create_app()
