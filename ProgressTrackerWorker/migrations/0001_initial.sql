CREATE TABLE IF NOT EXISTS events (
 event_id TEXT PRIMARY KEY,
 player_id TEXT NOT NULL,
 run_id TEXT NOT NULL,
 build_id TEXT NOT NULL,
 dataset TEXT NOT NULL CHECK (dataset = 'demo'),
 checkpoint_id TEXT NOT NULL,
 category TEXT NOT NULL CHECK (category IN ('start','quest','location','complete')),
 playtime_seconds REAL NOT NULL CHECK (playtime_seconds >= 0 AND playtime_seconds <= 315576000),
 received_at TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
);
CREATE INDEX IF NOT EXISTS events_build_stage ON events(build_id, checkpoint_id, player_id);
CREATE INDEX IF NOT EXISTS events_run ON events(player_id, run_id, build_id, dataset, checkpoint_id);
CREATE INDEX IF NOT EXISTS events_received ON events(received_at DESC);
