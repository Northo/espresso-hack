import sqlite3
from concurrent.futures import ThreadPoolExecutor

import pytest

from espresso_api.db import Database


def test_concurrent_initialization_preserves_single_initial_setup(tmp_path):
    path = tmp_path / "shots.sqlite3"
    with ThreadPoolExecutor(max_workers=4) as executor:
        list(executor.map(lambda _: Database(path).initialize(), range(4)))
    with Database(path).connect() as db:
        assert db.execute("SELECT COUNT(*) FROM setup_revisions").fetchone()[0] == 1
        assert db.execute("PRAGMA user_version").fetchone()[0] == 1


def test_newer_schema_is_rejected_without_modification(tmp_path):
    database = Database(tmp_path / "shots.sqlite3")
    database.initialize()
    with database.connect() as db:
        db.execute("PRAGMA user_version = 999")
    with pytest.raises(RuntimeError, match="newer"):
        database.initialize()
    with database.connect() as db:
        assert db.execute("PRAGMA user_version").fetchone()[0] == 999
        assert db.execute("SELECT COUNT(*) FROM setup_revisions").fetchone()[0] == 1


def test_failed_migration_rolls_back_schema_and_version(tmp_path):
    database = Database(tmp_path / "shots.sqlite3")
    # Simulate a partial/unversioned database that conflicts halfway through v1.
    with database.connect() as db:
        db.execute("CREATE TABLE setup_revisions (existing_column TEXT)")
        db.commit()
    with pytest.raises(sqlite3.OperationalError, match="already exists"):
        database.initialize()
    with database.connect() as db:
        assert db.execute("PRAGMA user_version").fetchone()[0] == 0
        assert (
            db.execute(
                "SELECT name FROM sqlite_master WHERE type = 'table' AND name = 'coffees'"
            ).fetchone()
            is None
        )
        assert db.execute("SELECT existing_column FROM setup_revisions").fetchall() == []
