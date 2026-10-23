"""Shared journal operations used by both the JSON API and HTML routes."""

import json
import sqlite3
from datetime import UTC, datetime
from uuid import UUID, uuid4

from fastapi import HTTPException

from .db import normalize_label
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


def create_coffee(db: sqlite3.Connection, payload: CoffeeCreate) -> Coffee:
    values = payload.model_dump(mode="json")
    values.update(id=str(uuid4()), archived=False, created_at=timestamp())
    values["updated_at"] = values["created_at"]
    with db:
        db.execute(
            f"INSERT INTO coffees ({', '.join(values)}) VALUES ({', '.join('?' for _ in values)})",
            tuple(values.values()),
        )
    return Coffee(**values)


def list_coffees(
    db: sqlite3.Connection,
    limit: int = 50,
    offset: int = 0,
    include_archived: bool = False,
    search: str = "",
) -> CoffeePage:
    clauses, parameters = [], []
    if not include_archived:
        clauses.append("archived = 0")
    if search.strip():
        clauses.append("instr(normalize_label(name || ' ' || coalesce(roaster, '')), ?) > 0")
        parameters.append(normalize_label(search))
    where = "WHERE " + " AND ".join(clauses) if clauses else ""
    with db:
        db.execute("BEGIN")
        total = db.execute(f"SELECT COUNT(*) FROM coffees {where}", parameters).fetchone()[0]
        rows = db.execute(
            f"SELECT * FROM coffees {where} ORDER BY name COLLATE NOCASE, id LIMIT ? OFFSET ?",
            (*parameters, limit, offset),
        ).fetchall()
    return CoffeePage(
        items=[Coffee(**dict(row)) for row in rows],
        total=total,
        limit=limit,
        offset=offset,
    )


def coffee_choices(db: sqlite3.Connection, include_archived: bool = False) -> list[Coffee]:
    where = "" if include_archived else "WHERE archived = 0"
    return [
        Coffee(**dict(row))
        for row in db.execute(
            f"SELECT * FROM coffees {where} ORDER BY name COLLATE NOCASE, roast_date DESC, id"
        )
    ]


def get_coffee(db: sqlite3.Connection, coffee_id: UUID) -> Coffee:
    return Coffee(**dict(require_row(db, "coffees", "id", str(coffee_id))))


def patch_coffee(db: sqlite3.Connection, coffee_id: UUID, payload: CoffeePatch) -> Coffee:
    with db:
        db.execute("BEGIN IMMEDIATE")
        require_row(db, "coffees", "id", str(coffee_id))
        values = payload.model_dump(mode="json", exclude_unset=True)
        if values:
            values["updated_at"] = timestamp()
            update_row(db, "coffees", str(coffee_id), values)
        row = require_row(db, "coffees", "id", str(coffee_id))
    return Coffee(**dict(row))


def get_setup(db: sqlite3.Connection) -> Setup:
    return read_setup(
        db.execute("SELECT * FROM setup_revisions ORDER BY revision DESC LIMIT 1").fetchone()
    )


def set_setup(db: sqlite3.Connection, payload: SetupWrite) -> Setup:
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


def create_shot(db: sqlite3.Connection, payload: ShotCreate) -> tuple[Shot, bool]:
    """Return the shot and whether it was newly created (rather than a retry)."""
    values = payload.model_dump(mode="json")
    original_request = json.dumps(values, sort_keys=True, separators=(",", ":"))
    created = False
    with db:
        db.execute("BEGIN IMMEDIATE")
        existing = db.execute("SELECT * FROM shots WHERE id = ?", (str(payload.id),)).fetchone()
        if existing is not None:
            if existing["creation_request"] != original_request:
                raise HTTPException(409, "Shot ID already exists with a different payload")
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
            created = True
    return result, created


def list_shots(
    db: sqlite3.Connection,
    limit: int = 50,
    offset: int = 0,
    coffee_id: UUID | None = None,
    since: datetime | None = None,
    until: datetime | None = None,
    name: str | None = None,
    roaster: str | None = None,
) -> ShotPage:
    if since and until and since >= until:
        raise HTTPException(422, "since must be earlier than until")
    clauses, parameters = [], []
    if coffee_id is not None:
        clauses.append("setup_revisions.coffee_id = ?")
        parameters.append(str(coffee_id))
    if name is not None:
        clauses.append("normalize_label(coffees.name) = ?")
        parameters.append(normalize_label(name))
    if roaster is not None:
        clauses.append("normalize_label(coffees.roaster) = ?")
        parameters.append(normalize_label(roaster))
    if since is not None:
        clauses.append("shots.received_at >= ?")
        parameters.append(timestamp(since))
    if until is not None:
        clauses.append("shots.received_at < ?")
        parameters.append(timestamp(until))
    where = "WHERE " + " AND ".join(clauses) if clauses else ""
    source = (
        "FROM shots JOIN setup_revisions ON shots.setup_revision = setup_revisions.revision "
        "LEFT JOIN coffees ON setup_revisions.coffee_id = coffees.id"
    )
    with db:
        db.execute("BEGIN")
        total = db.execute(f"SELECT COUNT(*) {source} {where}", parameters).fetchone()[0]
        rows = db.execute(
            f"SELECT shots.* {source} {where} "
            "ORDER BY shots.received_at DESC, shots.id DESC LIMIT ? OFFSET ?",
            (*parameters, limit, offset),
        ).fetchall()
        items = [read_shot(db, row) for row in rows]
    return ShotPage(items=items, total=total, limit=limit, offset=offset)


def get_shot(db: sqlite3.Connection, shot_id: UUID) -> Shot:
    return read_shot(db, require_row(db, "shots", "id", str(shot_id)))


def patch_shot(db: sqlite3.Connection, shot_id: UUID, payload: ShotPatch) -> Shot:
    with db:
        db.execute("BEGIN IMMEDIATE")
        require_row(db, "shots", "id", str(shot_id))
        update_row(db, "shots", str(shot_id), payload.model_dump(exclude_unset=True))
        result = read_shot(db, require_row(db, "shots", "id", str(shot_id)))
    return result
