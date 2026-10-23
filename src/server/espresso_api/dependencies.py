"""Request-scoped SQLite connection, shared by JSON and HTML routes."""

import sqlite3
from collections.abc import Iterator
from typing import Annotated

from fastapi import Depends, Request


def connection(request: Request) -> Iterator[sqlite3.Connection]:
    with request.app.state.database.connect() as db:
        yield db


DB = Annotated[sqlite3.Connection, Depends(connection)]
