CREATE TABLE IF NOT EXISTS admin_sessions (
  token_hash TEXT PRIMARY KEY CHECK (length(token_hash) = 64),
  owner_subject TEXT NOT NULL DEFAULT 'admin' CHECK (owner_subject = 'admin'),
  expires_at INTEGER NOT NULL,
  created_at INTEGER NOT NULL DEFAULT (unixepoch())
);
CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires_at ON admin_sessions(expires_at);
CREATE TABLE IF NOT EXISTS admin_login_attempts (
  client_key TEXT PRIMARY KEY CHECK (length(client_key) = 64),
  window_started_at INTEGER NOT NULL,
  failure_count INTEGER NOT NULL DEFAULT 0 CHECK (failure_count >= 0),
  blocked_until INTEGER NOT NULL DEFAULT 0
);
CREATE INDEX IF NOT EXISTS idx_admin_login_attempts_blocked_until ON admin_login_attempts(blocked_until);
CREATE TABLE IF NOT EXISTS snapshots (
  id TEXT PRIMARY KEY,
  alias TEXT NOT NULL,
  memo TEXT NOT NULL,
  created_at TEXT NOT NULL,
  schema_version INTEGER NOT NULL CHECK (schema_version = 1),
  node_count INTEGER NOT NULL CHECK (node_count >= 0),
  string_count INTEGER NOT NULL CHECK (string_count >= 0),
  pack_metadata_json TEXT NOT NULL DEFAULT '{}' CHECK (json_valid(pack_metadata_json))
);
CREATE TABLE IF NOT EXISTS quest_nodes (
  snapshot_id TEXT NOT NULL REFERENCES snapshots(id) ON DELETE CASCADE,
  quest_id TEXT NOT NULL COLLATE NOCASE,
  ordinal INTEGER NOT NULL,
  node_json TEXT NOT NULL CHECK (json_valid(node_json)),
  PRIMARY KEY (snapshot_id, quest_id),
  UNIQUE (snapshot_id, ordinal)
);
CREATE TABLE IF NOT EXISTS localization_strings (
  snapshot_id TEXT NOT NULL REFERENCES snapshots(id) ON DELETE CASCADE,
  string_key TEXT NOT NULL,
  locale TEXT NOT NULL,
  value TEXT NOT NULL,
  ordinal INTEGER NOT NULL,
  entry_json TEXT NOT NULL CHECK (json_valid(entry_json)),
  PRIMARY KEY (snapshot_id, string_key, locale),
  UNIQUE (snapshot_id, ordinal)
);
