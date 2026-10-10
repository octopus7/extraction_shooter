ALTER TABLE snapshots ADD COLUMN source_file_json TEXT CHECK (source_file_json IS NULL OR json_valid(source_file_json));
ALTER TABLE snapshots ADD COLUMN updated_at TEXT;
ALTER TABLE snapshots ADD COLUMN revision INTEGER NOT NULL DEFAULT 1 CHECK (revision >= 1);
UPDATE snapshots SET updated_at = created_at WHERE updated_at IS NULL;
