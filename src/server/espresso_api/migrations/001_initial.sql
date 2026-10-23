CREATE TABLE coffees (
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    roaster TEXT,
    origin TEXT,
    roast_date TEXT,
    notes TEXT,
    archived INTEGER NOT NULL DEFAULT 0 CHECK (archived IN (0, 1)),
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);

-- Append-only snapshots. The highest revision is the current setup.
CREATE TABLE setup_revisions (
    revision INTEGER PRIMARY KEY AUTOINCREMENT,
    coffee_id TEXT REFERENCES coffees(id),
    coffee_snapshot TEXT,
    grind_setting REAL CHECK (grind_setting >= 0),
    dose_g REAL CHECK (dose_g > 0),
    target_yield_g REAL CHECK (target_yield_g > 0),
    updated_at TEXT NOT NULL
);

INSERT INTO setup_revisions (updated_at)
VALUES (strftime('%Y-%m-%dT%H:%M:%fZ', 'now'));

CREATE TABLE shots (
    id TEXT PRIMARY KEY,
    setup_revision INTEGER NOT NULL REFERENCES setup_revisions(revision),
    started_at TEXT,
    received_at TEXT NOT NULL,
    duration_s REAL NOT NULL CHECK (duration_s >= 0),
    configured_preinfusion_s REAL CHECK (configured_preinfusion_s >= 0),
    configured_bloom_s REAL CHECK (configured_bloom_s >= 0),
    configured_brew_s REAL CHECK (configured_brew_s >= 0),
    temperature_setpoint_c REAL,
    end_temperature_c REAL,
    dose_g REAL CHECK (dose_g > 0),
    yield_g REAL CHECK (yield_g > 0),
    rating INTEGER CHECK (rating BETWEEN 1 AND 5),
    notes TEXT,
    -- Original normalized request, for idempotency even after annotations are edited.
    creation_request TEXT NOT NULL
);

CREATE INDEX shots_received_at ON shots(received_at DESC, id DESC);
CREATE INDEX shots_setup_revision ON shots(setup_revision);
CREATE INDEX setup_coffee_id ON setup_revisions(coffee_id);
