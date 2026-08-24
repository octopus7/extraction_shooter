CREATE TABLE IF NOT EXISTS admin_sessions (
  token_hash TEXT PRIMARY KEY
    CHECK (length(token_hash) = 64),
  owner_subject TEXT NOT NULL DEFAULT 'admin'
    CHECK (owner_subject = 'admin'),
  expires_at INTEGER NOT NULL,
  created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE INDEX IF NOT EXISTS idx_admin_sessions_expires_at
  ON admin_sessions(expires_at);

CREATE TABLE IF NOT EXISTS admin_login_attempts (
  client_key TEXT PRIMARY KEY
    CHECK (length(client_key) = 64),
  window_started_at INTEGER NOT NULL,
  failure_count INTEGER NOT NULL DEFAULT 0
    CHECK (failure_count >= 0),
  blocked_until INTEGER NOT NULL DEFAULT 0
);

CREATE INDEX IF NOT EXISTS idx_admin_login_attempts_blocked_until
  ON admin_login_attempts(blocked_until);
