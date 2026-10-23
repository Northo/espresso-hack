# Espresso shot server

A small, unauthenticated **trusted-LAN-only** API built with FastAPI, Pydantic,
and SQLite. One application process, no separate database service or message
broker. Firmware, touchscreen integration, and a browser frontend are not
implemented here yet.

## Run with Docker

From this directory:

```sh
docker compose up --build -d
```

- API: http://localhost:8000/api/v1/health
- Interactive API documentation: http://localhost:8000/docs
- OpenAPI schema: http://localhost:8000/openapi.json

Other LAN devices can use `http://<server-ip>:8000`. To change the published port:

```sh
ESPRESSO_PORT=8080 docker compose up --build -d
```

SQLite lives in the persistent `espresso-data` named volume at
`/data/espresso.sqlite3`. Recreating the container preserves it.
**Do not run `docker compose down -v` unless you intend to delete the data.**

There is no authentication, TLS, or CORS configuration. Anyone who can reach the
port can read and modify the journal. Do not publish it to the Internet.

## Run locally

Requires Python 3.11+ and [uv](https://docs.astral.sh/uv/).

```sh
uv sync --frozen --extra dev
uv run uvicorn espresso_api.app:app --reload --host 127.0.0.1 --port 8000
```

The default database is `data/espresso.sqlite3`, relative to the working directory.
Set `ESPRESSO_DB_PATH` to override it. The directory and database are created at
startup; numbered SQL migrations run automatically in a transaction.

To listen on your LAN, use `--host 0.0.0.0`. Run one Uvicorn worker; SQLite is
appropriate for this workload and does not need a multi-process deployment.
Keep the database on the server's local disk, not an SMB/NFS share.

## API

All routes are under `/api/v1`. Requests and responses use JSON. See `/docs`
for full field schemas.

| Method | Path | Purpose |
| --- | --- | --- |
| GET | `/health` | Liveness and database connectivity |
| POST | `/coffees` | Create a coffee/bag |
| GET | `/coffees` | List active coffees |
| GET | `/coffees/{id}` | Read one coffee, including archived coffees |
| PATCH | `/coffees/{id}` | Edit metadata or archive/unarchive |
| GET | `/setup` | Read the current setup and its revision |
| PUT | `/setup` | Replace the current setup; save a historical revision |
| POST | `/shots` | Submit a completed shot |
| GET | `/shots` | Browse shot history |
| GET | `/shots/{id}` | Read one shot with its setup snapshot |
| PATCH | `/shots/{id}` | Correct dose/yield or edit rating/notes |

### Coffee and setup

Coffee fields are `name` (required), `roaster`, `origin`, `roast_date`
(`YYYY-MM-DD`), and `notes`. Coffee IDs are server-generated UUIDs.
Each coffee record represents one bag; a new bag gets a new record, even when
its name and roaster are the same. Archive rather than delete a finished bag
to preserve history.

The current setup contains:

- `coffee_id`: optional coffee UUID when writing; responses embed a `coffee` snapshot.
- `grind_setting`: optional non-negative number, e.g. `15.5` on the Niche Zero.
  Zero and values beyond the printed dial are allowed; there is no artificial upper limit.
- `dose_g`: optional positive dose in grams.
- `target_yield_g`: optional positive desired yield in grams.

**PUT replaces the whole setup.** Omitted fields become null. Read the current
setup and send all desired writable fields when changing just the grind setting;
do not send response-only `revision`, `coffee`, or `updated_at`.

Each saved setup has a revision. Its coffee metadata and settings are frozen.
Editing or archiving the coffee, or selecting another setup, cannot change old
shot records. To refresh metadata in the current setup after editing a coffee,
PUT the selection again. Archived coffees cannot be selected for a new setup,
but their existing revisions remain usable.

A new database starts with revision 1, an empty setup. Shots without known beans
or grind settings can still reference this revision.

### Recording a shot

1. Read `GET /setup` when brewing starts (or use the revision returned by a
   successful setup change). Keep that revision with the in-progress shot.
2. Generate a unique UUID on the submitting device for this shot.
3. At brew completion, POST the ID, captured `setup_revision`, and measurements.

Required fields:

- `id`: client-generated UUID.
- `setup_revision`: the saved setup used at brew start, not the current setup
  fetched after brewing ends.
- `duration_s`: actual total elapsed seconds, **including preinfusion and bloom**.

Optional fields:

| Field | Meaning |
| --- | --- |
| `started_at` | Machine start time, with `Z` or a timezone offset; omit if clock is unknown |
| `configured_preinfusion_s` | Configured preinfusion duration, not measured time |
| `configured_bloom_s` | Configured bloom duration, not measured time |
| `configured_brew_s` | Configured main brewing-stage duration, not total time |
| `temperature_setpoint_c` | Brew setpoint in degrees Celsius |
| `end_temperature_c` | Measured temperature at brew completion, if valid |
| `dose_g` | Actual/user-entered dose; null/omitted initially copies the setup dose |
| `yield_g` | Actual/user-entered yield, not the target yield |
| `rating` | Integer from 1 (worst) to 5 (best) |
| `notes` | Tasting notes, up to 4,000 characters |

Unknown measurements stay null, never fabricated as zero. Configured durations
may be zero to indicate a skipped stage. We do not require configured stage
durations to sum to actual time: a shot can be stopped early or held longer.

The server adds `received_at` in UTC. It never guesses the machine's start time.
Responses embed the immutable `setup` including its revision. The shot's
`dose_g` and `yield_g` can be corrected later without changing that setup.

Creation returns **201**. Resubmitting the same ID and normalized creation payload
returns **200**, preserving any later annotation edits. Reusing an ID with
different creation data returns **409**. This also works for concurrent duplicate
requests; it does not imply any device-side queue or delivery guarantee.

### Browsing and editing

Both list routes return `{items, total, limit, offset}`:

- `limit`: 1–100, default 50; `offset`: non-negative, default 0.
- Coffees sort by name; add `include_archived=true` to include archived bags.
- Shots sort newest server receipt first, with ID as a deterministic tie-breaker.
- Shots support `coffee_id`, `since` (inclusive), and `until` (exclusive).
  Date bounds require timezones and filter **server receipt time**, not optional
  machine start time. Use ISO 8601; URL-encode `+` in timezone offsets.

PATCH leaves omitted fields unchanged. Send null to clear nullable fields.
Only `dose_g`, `yield_g`, `rating`, and `notes` are editable on a shot;
brew measurements, start time, and setup history cannot be rewritten.

Unknown fields, invalid measurements, and naive timestamps return **422**.
Unknown resources return **404**. POST/PUT/PATCH bodies over 64 KiB return **413**.

### Example

With the server running and Python 3 available:

```sh
BASE=http://localhost:8000/api/v1

COFFEE_ID=$(curl --fail-with-body -sS "$BASE/coffees" \
  -H 'Content-Type: application/json' \
  -d '{"name":"House blend","roaster":"Local roaster","roast_date":"2026-10-01"}' \
  | python3 -c 'import json,sys; print(json.load(sys.stdin)["id"])')

REVISION=$(curl --fail-with-body -sS -X PUT "$BASE/setup" \
  -H 'Content-Type: application/json' \
  -d "{\"coffee_id\":\"$COFFEE_ID\",\"grind_setting\":15.5,\"dose_g\":18,\"target_yield_g\":36}" \
  | python3 -c 'import json,sys; print(json.load(sys.stdin)["revision"])')

SHOT_ID=$(python3 -c 'import uuid; print(uuid.uuid4())')
curl --fail-with-body -sS "$BASE/shots" \
  -H 'Content-Type: application/json' \
  -d "{\"id\":\"$SHOT_ID\",\"setup_revision\":$REVISION,\"duration_s\":32.4,\"configured_preinfusion_s\":3,\"configured_bloom_s\":4,\"configured_brew_s\":25,\"temperature_setpoint_c\":93,\"yield_g\":36.5}"

curl --fail-with-body -sS "$BASE/shots?coffee_id=$COFFEE_ID&limit=10"
```

### Backups

For the simplest consistent backup, stop the server and copy its data directory,
including any SQLite sidecar files. To take a live backup from Docker, use
SQLite's backup API rather than copying just the main file while WAL is active:

```sh
docker compose exec -T server python -c \
  'import sqlite3; s=sqlite3.connect("/data/espresso.sqlite3"); d=sqlite3.connect("/data/backup.sqlite3"); s.backup(d); d.close(); s.close()'
docker compose cp server:/data/backup.sqlite3 ./espresso-backup.sqlite3
```

Stop the service before restoring a backup. Keep backups outside the volume as
well. Back up before updating; migrations are forward-only and older server
versions refuse a database with a newer schema.

## Development

```sh
uv run --frozen pytest -q
uv run --frozen ruff check .
uv run --frozen ruff format --check .
```

Tests use isolated temporary SQLite databases and cover the HTTP API, historical
snapshots, validation, idempotency, concurrent duplicate submissions, and restart
persistence. No controller or external server is needed.
