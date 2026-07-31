PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS quest_catalogs (
  id TEXT PRIMARY KEY,
  slug TEXT NOT NULL UNIQUE,
  title TEXT NOT NULL,
  visibility TEXT NOT NULL CHECK (visibility IN ('public', 'authenticated')),
  schema_version INTEGER NOT NULL CHECK (schema_version > 0),
  dataset_version TEXT NOT NULL,
  source_kind TEXT NOT NULL,
  source_hash TEXT NOT NULL,
  data_json TEXT NOT NULL CHECK (json_valid(data_json)),
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP
);

CREATE INDEX IF NOT EXISTS idx_quest_catalogs_visibility_sort
  ON quest_catalogs (visibility, title, slug);

CREATE TABLE IF NOT EXISTS workspaces (
  id TEXT PRIMARY KEY,
  owner_subject TEXT NOT NULL,
  title TEXT NOT NULL,
  catalog_id TEXT NOT NULL,
  state_json TEXT NOT NULL CHECK (
    json_valid(state_json)
    AND length(CAST(state_json AS BLOB)) <= 262144
  ),
  revision INTEGER NOT NULL DEFAULT 1 CHECK (revision > 0),
  created_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
  FOREIGN KEY (catalog_id) REFERENCES quest_catalogs(id) ON DELETE RESTRICT
);

CREATE INDEX IF NOT EXISTS idx_workspaces_owner_updated
  ON workspaces (owner_subject, updated_at DESC, id);

CREATE INDEX IF NOT EXISTS idx_workspaces_owner_catalog
  ON workspaces (owner_subject, catalog_id);
