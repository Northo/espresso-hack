from concurrent.futures import ThreadPoolExecutor
from datetime import UTC, datetime, timedelta
from uuid import uuid4

import pytest
from fastapi.testclient import TestClient

from espresso_api.app import create_app


def coffee(client, **overrides):
    payload = {"name": "House blend", "roaster": "Local roaster", **overrides}
    response = client.post("/api/v1/coffees", json=payload)
    assert response.status_code == 201, response.text
    return response.json()


def shot_payload(client, **overrides):
    return {
        "id": str(uuid4()),
        "setup_revision": client.get("/api/v1/setup").json()["revision"],
        "duration_s": 32.4,
        **overrides,
    }


def shot(client, **overrides):
    response = client.post("/api/v1/shots", json=shot_payload(client, **overrides))
    assert response.status_code == 201, response.text
    return response.json()


def test_health_and_openapi(client):
    assert client.get("/api/v1/health").json() == {"status": "ok"}
    assert client.get("/docs").status_code == 200
    schema = client.get("/openapi.json").json()
    assert "/api/v1/shots" in schema["paths"]
    assert schema["components"]["schemas"]["SetupWrite"]["properties"]["grind_setting"]


def test_coffee_lifecycle(client):
    created = coffee(client, roast_date="2026-10-01", notes="Chocolate")
    assert created["archived"] is False
    assert created["created_at"].endswith("Z")
    url = f"/api/v1/coffees/{created['id']}"
    assert client.get(url).json() == created
    patched = client.patch(url, json={"name": "New name", "notes": None}).json()
    assert patched["name"] == "New name"
    assert patched["notes"] is None
    assert patched["roast_date"] == "2026-10-01"
    assert client.patch(url, json={}).json() == patched
    assert client.patch(url, json={"archived": True}).status_code == 200
    assert client.get("/api/v1/coffees").json()["total"] == 0
    page = client.get("/api/v1/coffees", params={"include_archived": True}).json()
    assert page["total"] == 1
    assert page["items"][0]["archived"] is True
    assert client.patch(url, json={"archived": False}).json()["archived"] is False


def test_coffee_pagination_and_sorting(client):
    coffee(client, name="Zulu")
    first = coffee(client, name="alpha")
    coffee(client, name="Beta")
    page = client.get("/api/v1/coffees", params={"limit": 1}).json()
    assert page["items"][0]["id"] == first["id"]
    assert page["total"] == 3
    assert page["limit"] == 1
    page = client.get("/api/v1/coffees", params={"limit": 1, "offset": 1}).json()
    assert page["items"][0]["name"] == "Beta"
    assert page["offset"] == 1


@pytest.mark.parametrize(
    "payload",
    [
        {"name": ""},
        {"name": "   "},
        {"name": "x" * 201},
        {"name": "Valid", "unknown": "ignored?"},
        {"name": "Valid", "roast_date": "not-a-date"},
        {"name": "Valid", "notes": "x" * 4001},
    ],
)
def test_invalid_coffee(client, payload):
    assert client.post("/api/v1/coffees", json=payload).status_code == 422


@pytest.mark.parametrize("payload", [{"name": None}, {"archived": None}, {"id": str(uuid4())}])
def test_invalid_coffee_patch(client, payload):
    created = coffee(client)
    assert client.patch(f"/api/v1/coffees/{created['id']}", json=payload).status_code == 422


def test_setup_and_historical_snapshot(client):
    initial = client.get("/api/v1/setup").json()
    assert initial["revision"] == 1
    assert initial["coffee"] is None
    assert initial["grind_setting"] is None
    selected = coffee(client)
    response = client.put(
        "/api/v1/setup",
        json={
            "coffee_id": selected["id"],
            "grind_setting": 15.5,
            "dose_g": 18,
            "target_yield_g": 36,
        },
    )
    assert response.status_code == 200
    setup = response.json()
    assert setup["revision"] > initial["revision"]
    assert setup["grind_setting"] == 15.5
    assert setup["coffee"]["name"] == selected["name"]

    # Both setup changes and coffee edits after brew start must not affect this shot.
    client.put("/api/v1/setup", json={"grind_setting": 20})
    client.patch(f"/api/v1/coffees/{selected['id']}", json={"name": "Renamed", "archived": True})
    created = shot(
        client,
        setup_revision=setup["revision"],
        configured_preinfusion_s=3,
        configured_bloom_s=4,
        configured_brew_s=25,
        temperature_setpoint_c=93,
    )
    assert created["setup"] == setup
    assert created["dose_g"] == 18
    assert created["duration_s"] == 32.4
    assert created["configured_preinfusion_s"] == 3
    assert created["started_at"] is None
    assert created["received_at"].endswith("Z")
    assert client.get(f"/api/v1/shots/{created['id']}").json() == created


def test_setup_replacement_clears_omitted_fields(client):
    selected = coffee(client)
    client.put(
        "/api/v1/setup",
        json={
            "coffee_id": selected["id"],
            "dose_g": 18,
            "grind_setting": 15,
        },
    )
    setup = client.put("/api/v1/setup", json={"grind_setting": 0}).json()
    assert setup["coffee"] is None
    assert setup["dose_g"] is None
    assert setup["grind_setting"] == 0


def test_unknown_and_archived_coffee_cannot_be_selected(client):
    initial = client.get("/api/v1/setup").json()
    assert client.put("/api/v1/setup", json={"coffee_id": str(uuid4())}).status_code == 404
    selected = coffee(client)
    client.patch(f"/api/v1/coffees/{selected['id']}", json={"archived": True})
    assert client.put("/api/v1/setup", json={"coffee_id": selected["id"]}).status_code == 409
    assert client.get("/api/v1/setup").json() == initial


@pytest.mark.parametrize(
    "payload",
    [
        {"grind_setting": -0.1},
        {"grind_setting": "15"},
        {"grind_setting": True},
        {"dose_g": 0},
        {"target_yield_g": -1},
        {"unknown": 1},
    ],
)
def test_invalid_setup(client, payload):
    assert client.put("/api/v1/setup", json=payload).status_code == 422


def test_grind_setting_has_no_artificial_upper_bound(client):
    assert client.put("/api/v1/setup", json={"grind_setting": 100.5}).status_code == 200


@pytest.mark.parametrize(
    "raw",
    [
        '{"grind_setting": NaN}',
        '{"grind_setting": Infinity}',
        '{"grind_setting": -Infinity}',
    ],
)
def test_nonfinite_numbers(client, raw):
    assert (
        client.put(
            "/api/v1/setup", content=raw, headers={"Content-Type": "application/json"}
        ).status_code
        == 422
    )


def test_shot_unknown_setup(client):
    payload = shot_payload(client, setup_revision=12345)
    assert client.post("/api/v1/shots", json=payload).status_code == 404
    assert client.get("/api/v1/shots").json()["total"] == 0


def test_shot_time_normalized_to_utc(client):
    created = shot(client, started_at="2026-10-23T12:00:00+02:00")
    assert created["started_at"] == "2026-10-23T10:00:00Z"


@pytest.mark.parametrize(
    "overrides",
    [
        {"duration_s": -1},
        {"duration_s": "32"},
        {"duration_s": True},
        {"setup_revision": 0},
        {"setup_revision": 1.5},
        {"started_at": "2026-10-23T10:00:00"},  # Must not guess a timezone.
        {"configured_bloom_s": -1},
        {"yield_g": 0},
        {"dose_g": -1},
        {"rating": 0},
        {"rating": 6},
        {"rating": 2.5},
        {"temperature_setpoint_c": "93"},
        {"notes": "x" * 4001},
        {"grind_setting": 15},  # Must come from the captured setup revision.
    ],
)
def test_invalid_shot(client, overrides):
    assert client.post("/api/v1/shots", json=shot_payload(client, **overrides)).status_code == 422


def test_missing_required_shot_fields(client):
    assert client.post("/api/v1/shots", json={}).status_code == 422


def test_duplicate_shot_is_idempotent_after_annotation_edits(client):
    payload = shot_payload(client)
    first = client.post("/api/v1/shots", json=payload)
    assert first.status_code == 201
    assert first.headers["location"] == f"/api/v1/shots/{payload['id']}"
    repeated = client.post("/api/v1/shots", json=payload)
    assert repeated.status_code == 200
    assert repeated.json() == first.json()
    client.patch(f"/api/v1/shots/{payload['id']}", json={"rating": 4, "notes": "Nice"})
    repeated = client.post("/api/v1/shots", json=payload)
    assert repeated.status_code == 200
    assert repeated.json()["rating"] == 4
    assert repeated.json()["notes"] == "Nice"
    assert client.get("/api/v1/shots").json()["total"] == 1
    conflict = client.post("/api/v1/shots", json={**payload, "duration_s": 40})
    assert conflict.status_code == 409


def test_concurrent_duplicate_submissions(client):
    payload = shot_payload(client)
    with ThreadPoolExecutor(max_workers=4) as executor:
        responses = list(
            executor.map(lambda _: client.post("/api/v1/shots", json=payload), range(4))
        )
    assert sorted(response.status_code for response in responses) == [200, 200, 200, 201]
    assert client.get("/api/v1/shots").json()["total"] == 1


def test_shot_annotations_can_be_corrected_and_cleared(client):
    client.put("/api/v1/setup", json={"dose_g": 18})
    created = shot(client, dose_g=17.5, yield_g=35)
    assert created["dose_g"] == 17.5
    url = f"/api/v1/shots/{created['id']}"
    edited = client.patch(
        url,
        json={
            "dose_g": 18.2,
            "yield_g": 36.4,
            "rating": 5,
            "notes": "Sweet",
        },
    ).json()
    assert edited["dose_g"] == 18.2
    assert edited["setup"]["dose_g"] == 18  # Original recipe preserved.
    assert edited["rating"] == 5
    cleared = client.patch(url, json={"rating": None, "notes": None}).json()
    assert cleared["rating"] is None
    assert cleared["notes"] is None
    assert cleared["yield_g"] == 36.4
    assert client.patch(url, json={}).json() == cleared
    for field, value in [("duration_s", 5), ("setup_revision", 1), ("id", str(uuid4()))]:
        assert client.patch(url, json={field: value}).status_code == 422


def test_shot_pagination_and_filters(client):
    first_coffee = coffee(client)
    second_coffee = coffee(client, name="Other")
    client.put("/api/v1/setup", json={"coffee_id": first_coffee["id"]})
    first = shot(client)
    second = shot(client)
    client.put("/api/v1/setup", json={"coffee_id": second_coffee["id"]})
    third = shot(client)
    page = client.get("/api/v1/shots", params={"limit": 1, "offset": 1}).json()
    assert page["total"] == 3
    assert page["items"][0]["id"] == second["id"]
    assert page["limit"] == 1
    assert page["offset"] == 1
    page = client.get("/api/v1/shots", params={"coffee_id": first_coffee["id"]}).json()
    assert [item["id"] for item in page["items"]] == [second["id"], first["id"]]
    page = client.get(
        "/api/v1/shots",
        params={
            "since": second["received_at"],
            "until": third["received_at"],
        },
    ).json()
    assert [item["id"] for item in page["items"]] == [second["id"]]
    page = client.get("/api/v1/shots", params={"coffee_id": str(uuid4())}).json()
    assert page["total"] == 0
    assert page["items"] == []
    future = (datetime.now(UTC) + timedelta(days=1)).isoformat()
    assert client.get("/api/v1/shots", params={"since": future}).json()["total"] == 0


@pytest.mark.parametrize(
    "params",
    [
        {"limit": 0},
        {"limit": 101},
        {"offset": -1},
        {"coffee_id": "invalid"},
        {"since": "2026-01-01T00:00:00"},
        {"since": "2026-02-01T00:00:00Z", "until": "2026-01-01T00:00:00Z"},
    ],
)
def test_invalid_shot_filters(client, params):
    assert client.get("/api/v1/shots", params=params).status_code == 422


def test_not_found_and_invalid_ids(client):
    missing = str(uuid4())
    for resource in ["coffees", "shots"]:
        assert client.get(f"/api/v1/{resource}/{missing}").status_code == 404
        assert client.patch(f"/api/v1/{resource}/{missing}", json={}).status_code == 404
        assert client.get(f"/api/v1/{resource}/invalid").status_code == 422


def test_request_body_limit(client):
    response = client.post("/api/v1/coffees", content=b"x" * (64 * 1024 + 1))
    assert response.status_code == 413
    assert (
        client.post(
            "/api/v1/coffees", content=b"{", headers={"Content-Type": "application/json"}
        ).status_code
        == 422
    )


def test_persistence_and_migrations_across_restart(tmp_path):
    path = tmp_path / "nested" / "shots.sqlite3"
    with TestClient(create_app(path)) as first:
        selected = coffee(first)
        setup = first.put(
            "/api/v1/setup",
            json={
                "coffee_id": selected["id"],
                "grind_setting": 15.5,
            },
        ).json()
        created = shot(first)
    with TestClient(create_app(path)) as restarted:
        assert restarted.get("/api/v1/coffees").json()["items"][0] == selected
        assert restarted.get("/api/v1/setup").json() == setup
        assert restarted.get(f"/api/v1/shots/{created['id']}").json() == created
        with restarted.app.state.database.connect() as db:
            assert db.execute("PRAGMA user_version").fetchone()[0] == 1
            assert db.execute("PRAGMA foreign_keys").fetchone()[0] == 1
            assert db.execute("PRAGMA journal_mode").fetchone()[0] == "wal"


def test_database_path_environment(monkeypatch, tmp_path):
    path = tmp_path / "configured.sqlite3"
    monkeypatch.setenv("ESPRESSO_DB_PATH", str(path))
    with TestClient(create_app()) as configured:
        assert configured.get("/api/v1/health").status_code == 200
    assert path.exists()
