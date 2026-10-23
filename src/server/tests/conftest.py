import pytest
from fastapi.testclient import TestClient

from espresso_api.app import create_app


@pytest.fixture
def client(tmp_path):
    with TestClient(create_app(tmp_path / "shots.sqlite3")) as test_client:
        yield test_client
