"""Small SQLite connection and transactional migration layer."""

import sqlite3
from collections.abc import Iterator
from contextlib import contextmanager
from pathlib import Path
from time import monotonic, sleep


def normalize_label(value: str | None) -> str:
    """Match coffee types without case or whitespace differences."""
    return " ".join((value or "").split()).casefold()


class Database:
    def __init__(self, path: str | Path):
        self.path = Path(path)

    @contextmanager
    def connect(self) -> Iterator[sqlite3.Connection]:
        connection = sqlite3.connect(self.path, timeout=5, check_same_thread=False)
        connection.row_factory = sqlite3.Row
        connection.execute("PRAGMA foreign_keys = ON")
        connection.create_function("normalize_label", 1, normalize_label, deterministic=True)
        try:
            yield connection
        finally:
            connection.close()

    def initialize(self) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        migrations = sorted(Path(__file__).with_name("migrations").glob("*.sql"))
        with self.connect() as connection:
            # Changing journal mode may return BUSY without honoring SQLite's busy
            # timeout when two processes first open a new database. Retry at startup.
            deadline = monotonic() + 5
            while True:
                try:
                    connection.execute("PRAGMA journal_mode = WAL")
                    break
                except sqlite3.OperationalError as exc:
                    if exc.sqlite_errorcode != sqlite3.SQLITE_BUSY or monotonic() >= deadline:
                        raise
                    sleep(0.05)
            # Hold the write lock while checking and applying schema versions.
            with connection:
                connection.execute("BEGIN IMMEDIATE")
                version = connection.execute("PRAGMA user_version").fetchone()[0]
                if version > len(migrations):
                    raise RuntimeError("Database schema is newer than this server")
                for expected, migration in enumerate(migrations, start=1):
                    if int(migration.stem.split("_")[0]) != expected:
                        raise RuntimeError("Migration versions must be consecutive")
                    if expected <= version:
                        continue
                    statement = ""
                    for line in migration.read_text().splitlines(keepends=True):
                        statement += line
                        if sqlite3.complete_statement(statement):
                            connection.execute(statement)
                            statement = ""
                    if statement.strip():
                        raise RuntimeError(f"Incomplete SQL in {migration.name}")
                    connection.execute(f"PRAGMA user_version = {expected}")
