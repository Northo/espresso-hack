"""Contract exercised by the touchscreen's setup/upload/yield workflow."""

from uuid import uuid4


def test_machine_main_time_snapshot_and_yield_retry(client):
    coffee = client.post(
        "/api/v1/coffees", json={"name": "Machine bag", "roaster": "Local roaster"}
    ).json()
    setup = client.put(
        "/api/v1/setup",
        json={"coffee_id": coffee["id"], "grind_setting": 16.5, "dose_g": 18, "target_yield_g": 36},
    ).json()
    # Machine freezes revision at START, not when its delayed HTTP upload arrives.
    client.put("/api/v1/setup", json={"grind_setting": 20})
    payload = {
        "id": str(uuid4()),
        "setup_revision": setup["revision"],
        "duration_s": 28.4,
        "configured_preinfusion_s": 2,
        "configured_bloom_s": 5,
        "configured_brew_s": 30,
        "temperature_setpoint_c": 93,
        "end_temperature_c": 92.3,
    }
    created = client.post("/api/v1/shots", json=payload)
    assert created.status_code == 201
    shot = created.json()
    assert shot["duration_s"] == 28.4  # not 35.4; not inferred from configured times
    assert shot["setup"]["coffee"]["id"] == coffee["id"]
    assert shot["setup"]["grind_setting"] == 16.5
    assert shot["dose_g"] == 18
    assert shot["yield_g"] is None  # target yield is never a measurement
    assert shot["started_at"] is None  # no trustworthy device wall clock required
    path = "/api/v1/shots/" + payload["id"]
    assert client.patch(path, json={"yield_g": 36.7}).json()["yield_g"] == 36.7
    # Lost HTTP response / user retry: same original POST body, then same PATCH.
    retry = client.post("/api/v1/shots", json=payload)
    assert retry.status_code == 200
    assert retry.json()["yield_g"] == 36.7
    assert client.patch(path, json={"yield_g": 36.7}).status_code == 200
    assert client.get("/api/v1/shots").json()["total"] == 1
    assert "Brew duration" in client.get("/shots/" + payload["id"]).text
    history = client.get("/shots").text
    assert "Brew time" in history
    assert "preinfusion and bloom are excluded" in history


def test_api_documents_main_stage_duration(client):
    schemas = client.get("/openapi.json").json()["components"]["schemas"]
    for name in ("Shot", "ShotCreate"):
        field = schemas[name]["properties"]["duration_s"]
        assert "excludes preinfusion and bloom" in field["description"]
        assert field["minimum"] == 0
        assert "duration_s" in schemas[name]["required"]
