import html
import json
import re
from uuid import uuid4

import pytest


def create_coffee(client, **changes):
    response = client.post(
        "/api/v1/coffees",
        json={
            "name": "House blend",
            "roaster": "Local roaster",
            "roast_date": "2026-10-01",
            **changes,
        },
    )
    assert response.status_code == 201
    return response.json()


def create_shot(client, coffee=None, **changes):
    setup = client.put(
        "/api/v1/setup",
        json={
            "coffee_id": coffee["id"] if coffee else None,
            "grind_setting": 15.5,
            "dose_g": 18,
            "target_yield_g": 36,
        },
    ).json()
    response = client.post(
        "/api/v1/shots",
        json={
            "id": str(uuid4()),
            "setup_revision": setup["revision"],
            "duration_s": 32.4,
            **changes,
        },
    )
    assert response.status_code == 201
    return response.json()


@pytest.mark.parametrize("path", ["/", "/shots", "/coffees", "/coffees/new", "/setup"])
def test_pages_and_local_assets(client, path):
    response = client.get(path)
    assert response.status_code == 200
    assert response.headers["content-type"].startswith("text/html")
    assert response.headers["cache-control"] == "no-store"
    assert 'id="app"' in response.text
    assert 'name="viewport"' in response.text
    assert 'hx-boost="true"' in response.text
    assert not re.search(r'(?:src|href)="https?://', response.text)


def test_static_assets(client):
    for path in ["journal.css", "journal.js", "vendor/htmx-2.0.8.min.js"]:
        response = client.get("/static/" + path)
        assert response.status_code == 200
        assert len(response.content) > 500
    assert "Zero-Clause BSD" in client.get("/static/vendor/htmx-LICENSE.txt").text
    assert client.get("/static/../app.py").status_code == 404


def test_create_edit_and_archive_bag_from_browser(client):
    response = client.post(
        "/coffees/new",
        data={
            "name": "House blend",
            "roaster": "Local roaster",
            "roast_date": "2026-10-01",
            "origin": "Brazil",
            "notes": "First bag",
        },
        follow_redirects=False,
    )
    assert response.status_code == 303
    location = response.headers["location"]
    coffee_id = location.split("/")[-1].split("?")[0]
    coffee = client.get(f"/api/v1/coffees/{coffee_id}").json()
    assert coffee["roast_date"] == "2026-10-01"
    assert "Saved." in client.get(location).text
    response = client.post(
        f"/coffees/{coffee_id}",
        data={
            "name": "Renamed",
            "roaster": "",
            "roast_date": "",
            "origin": "",
            "notes": "",
        },
        follow_redirects=False,
    )
    assert response.status_code == 303
    coffee = client.get(f"/api/v1/coffees/{coffee_id}").json()
    assert coffee["name"] == "Renamed"
    assert coffee["roaster"] is None
    assert coffee["roast_date"] is None
    assert coffee["origin"] is None
    assert coffee["notes"] is None
    for archived in [True, False]:
        response = client.post(
            f"/coffees/{coffee_id}/archive",
            data={
                "archived": str(archived).lower(),
            },
            follow_redirects=False,
        )
        assert response.status_code == 303
        assert client.get(f"/api/v1/coffees/{coffee_id}").json()["archived"] is archived
        assert client.get("/api/v1/coffees").json()["total"] == (0 if archived else 1)


def test_invalid_coffee_form_preserves_input_without_writing(client):
    response = client.post(
        "/coffees/new",
        data={
            "name": "",
            "roaster": "Roaster & friends",
            "roast_date": "not-a-date",
            "notes": "Keep this note",
        },
    )
    assert response.status_code == 422
    assert "That wasn't saved" in response.text
    assert 'value="Roaster &amp; friends"' in response.text
    assert "Keep this note" in response.text
    assert client.get("/api/v1/coffees").json()["total"] == 0


def test_escaping_user_content(client):
    dangerous = '<script>alert("coffee")</script>'
    coffee = create_coffee(client, name=dangerous, notes="</textarea>" + dangerous)
    shot = create_shot(client, coffee, notes=dangerous)
    for path in ["/coffees", f"/coffees/{coffee['id']}", "/shots", f"/shots/{shot['id']}"]:
        page = client.get(path).text
        assert dangerous not in page
        assert "&lt;script&gt;" in page


def test_htmx_success_navigation_and_invalid_form(client):
    response = client.post(
        "/coffees/new",
        data={"name": "New bag"},
        headers={
            "HX-Request": "true",
        },
    )
    assert response.status_code == 200
    navigation = json.loads(response.headers["hx-location"])
    assert navigation["target"] == "#app"
    assert navigation["select"] == "#app"
    assert navigation["path"].startswith("/coffees/")
    assert "New bag" in client.get(navigation["path"], headers={"HX-Request": "true"}).text
    response = client.post("/coffees/new", data={"name": ""}, headers={"HX-Request": "true"})
    assert response.status_code == 422
    assert 'id="app"' in response.text
    assert "That wasn't saved" in response.text
    assert client.get("/api/v1/coffees").json()["total"] == 1


def test_setup_from_browser_and_clear_optional_fields(client):
    coffee = create_coffee(client)
    response = client.post(
        "/setup",
        data={
            "coffee_id": coffee["id"],
            "grind_setting": "15.5",
            "dose_g": "18",
            "target_yield_g": "36",
        },
        follow_redirects=False,
    )
    assert response.status_code == 303
    setup = client.get("/api/v1/setup").json()
    assert setup["coffee"]["id"] == coffee["id"]
    assert setup["grind_setting"] == 15.5
    assert setup["dose_g"] == 18
    assert 'value="15.5"' in client.get("/setup").text
    assert "journal metadata only" in client.get("/setup").text
    client.post(
        "/setup",
        data={
            "coffee_id": "",
            "grind_setting": "0",
            "dose_g": "",
            "target_yield_g": "",
        },
    )
    setup = client.get("/api/v1/setup").json()
    assert setup["coffee"] is None
    assert setup["grind_setting"] == 0
    assert setup["dose_g"] is None
    assert setup["target_yield_g"] is None


@pytest.mark.parametrize(
    "field,value",
    [
        ("grind_setting", "not-a-number"),
        ("grind_setting", "NaN"),
        ("grind_setting", "Infinity"),
        ("grind_setting", "-1"),
        ("dose_g", "0"),
        ("coffee_id", "invalid"),
    ],
)
def test_invalid_setup_does_not_change_revision(client, field, value):
    before = client.get("/api/v1/setup").json()
    response = client.post("/setup", data={field: value})
    assert response.status_code == 422
    assert "That wasn't saved" in response.text
    assert client.get("/api/v1/setup").json() == before


def test_archived_and_unknown_bag_setup_errors(client):
    coffee = create_coffee(client)
    create_shot(client, coffee)
    client.post(f"/coffees/{coffee['id']}/archive", data={"archived": "true"})
    assert "selected bag is marked finished" in client.get("/setup").text
    before = client.get("/api/v1/setup").json()
    response = client.post("/setup", data={"coffee_id": coffee["id"]})
    assert response.status_code == 409
    assert "Archived coffee cannot be selected" in response.text
    response = client.post("/setup", data={"coffee_id": str(uuid4())})
    assert response.status_code == 404
    assert "That wasn't saved" in response.text
    assert client.get("/api/v1/setup").json() == before
    client.post("/setup", data={"coffee_id": ""})
    assert client.get("/api/v1/setup").json()["coffee"] is None


def test_shot_annotations_preserve_snapshot_and_measurements(client):
    coffee = create_coffee(client)
    original = create_shot(client, coffee)
    url = f"/shots/{original['id']}"
    assert "32.4" in client.get(url).text
    response = client.post(
        url,
        data={
            "dose_g": "18.2",
            "yield_g": "36.4",
            "rating": "5",
            "notes": "Sweet & balanced",
        },
        follow_redirects=False,
    )
    assert response.status_code == 303
    shot = client.get(f"/api/v1/shots/{original['id']}").json()
    assert shot["dose_g"] == 18.2
    assert shot["yield_g"] == 36.4
    assert shot["rating"] == 5
    assert shot["notes"] == "Sweet & balanced"
    assert shot["setup"] == original["setup"]
    assert shot["duration_s"] == original["duration_s"]
    client.post(url, data={"dose_g": "", "yield_g": "", "rating": "", "notes": ""})
    cleared = client.get(f"/api/v1/shots/{original['id']}").json()
    assert all(cleared[field] is None for field in ["dose_g", "yield_g", "rating", "notes"])
    client.post(f"/coffees/{coffee['id']}", data={"name": "Renamed"})
    assert client.get(f"/api/v1/shots/{original['id']}").json()["setup"] == original["setup"]


@pytest.mark.parametrize(
    "fields",
    [
        {"rating": "2.5"},
        {"rating": "6"},
        {"yield_g": "0"},
        {"duration_s": "1"},
        {"setup_revision": "123"},
        {"notes": "x" * 4001},
    ],
)
def test_invalid_shot_form_does_not_modify_shot(client, fields):
    original = create_shot(client)
    response = client.post(f"/shots/{original['id']}", data=fields)
    assert response.status_code == 422
    assert "That wasn't saved" in response.text
    assert client.get(f"/api/v1/shots/{original['id']}").json() == original


def test_coffee_type_matching_across_bags_and_roasters(client):
    first = create_coffee(client, name="Éthiopia   Natural", roaster="Roast   House")
    second = create_coffee(client, name="éTHIOPIA natural", roaster="roast house")
    other = create_coffee(client, name="Éthiopia Natural", roaster="Another roaster")
    first_shot, second_shot, other_shot = [
        create_shot(client, bag) for bag in [first, second, other]
    ]
    client.post(f"/coffees/{first['id']}/archive", data={"archived": "true"})
    page = client.get(
        "/shots", params={"name": " Éthiopia Natural ", "roaster": "roast house"}
    ).text
    assert f"/shots/{first_shot['id']}" in page
    assert f"/shots/{second_shot['id']}" in page
    assert f"/shots/{other_shot['id']}" not in page
    page = client.get("/shots", params={"coffee_id": first["id"]}).text
    assert f"/shots/{first_shot['id']}" in page
    assert f"/shots/{second_shot['id']}" not in page
    assert "Past settings" in client.get("/coffees", params={"include_archived": True}).text


def test_unknown_roaster_matches_only_unknown_roasters(client):
    first = create_coffee(client, roaster=None)
    known = create_coffee(client)
    first_shot = create_shot(client, first)
    known_shot = create_shot(client, known)
    page = client.get("/shots", params={"name": "House blend", "roaster": ""}).text
    assert f"/shots/{first_shot['id']}" in page
    assert f"/shots/{known_shot['id']}" not in page


def test_search_and_finished_bag_filter(client):
    coffee = create_coffee(client, name="Éthiopia Natural", roaster="Roast House")
    assert "Éthiopia Natural" in client.get("/coffees", params={"q": "ÉTHIOPIA"}).text
    assert "Éthiopia Natural" in client.get("/coffees", params={"q": "roast  house"}).text
    client.post(f"/coffees/{coffee['id']}/archive", data={"archived": "true"})
    assert "Éthiopia Natural" not in client.get("/coffees").text
    assert "Éthiopia Natural" in client.get("/coffees", params={"include_archived": True}).text
    assert "No matching bags" in client.get("/coffees", params={"q": "Absent"}).text


def test_history_pagination_keeps_filters(client):
    coffee = create_coffee(client)
    shots = [create_shot(client, coffee) for _ in range(23)]
    page = client.get("/shots", params={"name": coffee["name"], "roaster": coffee["roaster"]})
    assert sum(f"/shots/{shot['id']}" in page.text for shot in shots) == 20
    next_url = html.unescape(re.search(r'href="([^"]+)">Next →', page.text).group(1))
    assert "name=House+blend" in next_url
    assert "roaster=Local+roaster" in next_url
    assert "offset=20" in next_url
    page = client.get(next_url)
    assert sum(f"/shots/{shot['id']}" in page.text for shot in shots) == 3
    assert "Previous" in page.text


def test_empty_date_filters_and_receipt_date_bounds(client):
    coffee = create_coffee(client)
    shot = create_shot(client, coffee)
    assert client.get("/shots?coffee_id=&since=&until=").status_code == 200
    day = shot["received_at"][:10]
    assert shot["id"] in client.get("/shots", params={"since": day}).text
    assert shot["id"] not in client.get("/shots", params={"until": day}).text
    response = client.get("/shots?since=2026-02-01&until=2026-01-01")
    assert response.status_code == 422
    assert response.headers["content-type"].startswith("text/html")


@pytest.mark.parametrize(
    "path,status",
    [
        ("/shots/not-a-uuid", 422),
        ("/coffees/not-a-uuid", 422),
        (f"/shots/{uuid4()}", 404),
        (f"/coffees/{uuid4()}", 404),
        ("/shots?since=not-a-date", 422),
        ("/shots?offset=-1", 422),
    ],
)
def test_browser_errors_are_html(client, path, status):
    response = client.get(path)
    assert response.status_code == status
    assert 'id="app"' in response.text
    assert response.headers["content-type"].startswith("text/html")


def test_cross_origin_browser_form_rejected_but_same_origin_works(client):
    response = client.post(
        "/coffees/new",
        data={"name": "Nope"},
        headers={
            "Origin": "http://another-host",
        },
    )
    assert response.status_code == 403
    assert client.get("/api/v1/coffees").json()["total"] == 0
    response = client.post(
        "/coffees/new",
        data={"name": "Good"},
        headers={
            "Origin": "http://testserver",
        },
        follow_redirects=False,
    )
    assert response.status_code == 303
