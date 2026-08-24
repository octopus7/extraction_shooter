PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS quest_releases (
  id TEXT PRIMARY KEY,
  catalog_id TEXT NOT NULL,
  parent_release_id TEXT,
  dataset_version TEXT NOT NULL,
  content_hash TEXT NOT NULL CHECK (length(content_hash) = 64),
  schema_version INTEGER NOT NULL CHECK (schema_version > 0),
  data_json TEXT NOT NULL CHECK (
    json_valid(data_json)
    AND length(CAST(data_json AS BLOB)) <= 262144
  ),
  source_kind TEXT NOT NULL,
  created_by TEXT NOT NULL,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (catalog_id) REFERENCES quest_catalogs(id) ON DELETE RESTRICT,
  FOREIGN KEY (parent_release_id) REFERENCES quest_releases(id) ON DELETE RESTRICT,
  UNIQUE (catalog_id, dataset_version),
  UNIQUE (catalog_id, content_hash)
);

CREATE INDEX IF NOT EXISTS idx_quest_releases_catalog_created
  ON quest_releases (catalog_id, created_at DESC, id);

CREATE TABLE IF NOT EXISTS quest_channels (
  catalog_id TEXT PRIMARY KEY,
  current_release_id TEXT NOT NULL,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (catalog_id) REFERENCES quest_catalogs(id) ON DELETE CASCADE,
  FOREIGN KEY (current_release_id) REFERENCES quest_releases(id) ON DELETE RESTRICT
);

CREATE TABLE IF NOT EXISTS quest_sync_tokens (
  token_hash TEXT PRIMARY KEY CHECK (length(token_hash) = 64),
  token_id TEXT NOT NULL UNIQUE,
  label TEXT NOT NULL,
  scopes_json TEXT NOT NULL CHECK (json_valid(scopes_json)),
  expires_at INTEGER,
  revoked_at INTEGER,
  last_used_at INTEGER,
  created_by TEXT NOT NULL DEFAULT 'admin',
  created_at INTEGER NOT NULL DEFAULT (unixepoch())
);

CREATE INDEX IF NOT EXISTS idx_quest_sync_tokens_active
  ON quest_sync_tokens (revoked_at, expires_at, token_id);

CREATE TABLE IF NOT EXISTS quest_sync_audit_log (
  id TEXT PRIMARY KEY,
  actor_subject TEXT NOT NULL,
  action TEXT NOT NULL,
  catalog_id TEXT NOT NULL,
  previous_dataset_version TEXT,
  next_dataset_version TEXT,
  release_id TEXT NOT NULL,
  request_id TEXT NOT NULL,
  summary TEXT,
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (catalog_id) REFERENCES quest_catalogs(id) ON DELETE RESTRICT,
  FOREIGN KEY (release_id) REFERENCES quest_releases(id) ON DELETE RESTRICT
);

CREATE INDEX IF NOT EXISTS idx_quest_sync_audit_catalog_created
  ON quest_sync_audit_log (catalog_id, created_at DESC, id);

ALTER TABLE workspaces ADD COLUMN base_dataset_version TEXT;

UPDATE workspaces
   SET base_dataset_version = (
     SELECT dataset_version
       FROM quest_catalogs
      WHERE quest_catalogs.id = workspaces.catalog_id
   )
 WHERE base_dataset_version IS NULL;

INSERT OR IGNORE INTO quest_releases (
  id, catalog_id, parent_release_id, dataset_version, content_hash,
  schema_version, data_json, source_kind, created_by, created_at
)
SELECT
  'initial-' || id,
  id,
  NULL,
  dataset_version,
  substr(lower(source_hash), -64),
  3,
  json_object(
    'schemaVersion', 3,
    'flavor', CASE slug WHEN 'demo' THEN 'Demo' ELSE 'Main' END,
    'runtime', json_object(
      'questDefinitions', json('[]'),
      'questTextStringsCsv', ''
    ),
    'editor', json(data_json)
  ),
  'legacy-catalog-migration',
  'migration',
  created_at
FROM quest_catalogs
WHERE slug IN ('demo', 'main-m01-m20')
  AND length(source_hash) >= 64;

INSERT OR IGNORE INTO quest_channels (catalog_id, current_release_id, updated_at)
SELECT id, 'initial-' || id, updated_at
FROM quest_catalogs
WHERE slug IN ('demo', 'main-m01-m20')
  AND EXISTS (
    SELECT 1 FROM quest_releases WHERE quest_releases.id = 'initial-' || quest_catalogs.id
  );
