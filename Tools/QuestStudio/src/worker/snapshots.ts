import type { QuestPack, SnapshotMetadata, SourceFileMetadata } from '../shared/types';
import { isRecord, normalizeLoadedPack, validatePack, ValidationError } from '../shared/validation';
import { ApiError } from './http';

interface SnapshotRow {
  id: string; alias: string; memo: string; created_at: string; updated_at: string | null; revision: number; source_file_json: string | null;
  schema_version: number; node_count: number; string_count: number; pack_metadata_json: string;
}
const metadata = (row: SnapshotRow): SnapshotMetadata => ({
  id: row.id, alias: row.alias, memo: row.memo, createdAt: row.created_at, updatedAt: row.updated_at ?? row.created_at,
  revision: row.revision, sourceFile: row.source_file_json === null ? null : JSON.parse(row.source_file_json), nodeCount: row.node_count, stringCount: row.string_count,
});
function sourceFile(value: unknown): SourceFileMetadata | null {
  if (value === null || value === undefined) return null;
  if (!isRecord(value) || typeof value.name !== 'string' || !value.name.trim() || value.name.length > 1024 ||
      typeof value.lastModified !== 'number' || !Number.isSafeInteger(value.lastModified) || value.lastModified < 0 || value.lastModified > 8640000000000000) throw new ApiError(422, 'api.invalid_snapshot');
  return { name: value.name, lastModified: value.lastModified };
}
function expectedRevision(value: unknown): number {
  if (!isRecord(value) || typeof value.expectedRevision !== 'number' || !Number.isSafeInteger(value.expectedRevision) || value.expectedRevision < 1 || value.expectedRevision >= Number.MAX_SAFE_INTEGER) throw new ApiError(422, 'api.invalid_snapshot');
  return value.expectedRevision;
}
function childInserts(db: D1Database, id: string, pack: QuestPack, revision: number): D1PreparedStatement[] {
  return [
    db.prepare(`INSERT INTO quest_nodes (snapshot_id, quest_id, ordinal, node_json)
      SELECT ?, CAST(json_extract(value, '$.definition.quest_id') AS TEXT), CAST(key AS INTEGER), value FROM json_each(?)
      WHERE EXISTS (SELECT 1 FROM snapshots WHERE id = ? AND revision = ?)`)
      .bind(id, JSON.stringify(pack.nodes), id, revision),
    db.prepare(`INSERT INTO localization_strings (snapshot_id, string_key, locale, value, ordinal, entry_json)
      SELECT ?, json_extract(value, '$.key'), json_extract(value, '$.locale'), json_extract(value, '$.value'), CAST(key AS INTEGER), value FROM json_each(?)
      WHERE EXISTS (SELECT 1 FROM snapshots WHERE id = ? AND revision = ?)`)
      .bind(id, JSON.stringify(pack.strings), id, revision),
  ];
}
export async function listSnapshots(db: D1Database): Promise<SnapshotMetadata[]> {
  const { results } = await db.prepare('SELECT * FROM snapshots ORDER BY created_at DESC, id DESC').all<SnapshotRow>();
  return results.map(metadata);
}
export async function saveSnapshot(db: D1Database, value: unknown): Promise<SnapshotMetadata> {
  if (!isRecord(value) || typeof value.alias !== 'string' || !value.alias.trim() || value.alias.length > 120 ||
      typeof value.memo !== 'string' || value.memo.length > 2000) throw new ApiError(422, 'api.invalid_snapshot');
  const pack = validatePack(value.pack); const file = sourceFile(value.sourceFile); const id = crypto.randomUUID(); const createdAt = new Date().toISOString();
  const { nodes, strings, ...packMetadata } = pack;
  const snapshot: SnapshotMetadata = { id, alias: value.alias, memo: value.memo, createdAt, updatedAt: createdAt, revision: 1, sourceFile: file, nodeCount: nodes.length, stringCount: strings.length };
  // D1 batch commits the complete pack atomically; json_each keeps statement count constant.
  await db.batch([
    db.prepare(`INSERT INTO snapshots (id, alias, memo, created_at, updated_at, schema_version, node_count, string_count, pack_metadata_json, source_file_json)
      VALUES (?, ?, ?, ?, ?, 1, ?, ?, ?, ?)`).bind(id, snapshot.alias, snapshot.memo, createdAt, createdAt, nodes.length, strings.length, JSON.stringify(packMetadata), file === null ? null : JSON.stringify(file)),
    ...childInserts(db, id, pack, 1),
  ]);
  return snapshot;
}
export async function overwriteSnapshot(db: D1Database, id: string, value: unknown): Promise<SnapshotMetadata> {
  const revision = expectedRevision(value);
  const body = value as Record<string, unknown>;
  const pack = validatePack(body.pack); const file = sourceFile(body.sourceFile);
  const { nodes, strings, ...packMetadata } = pack;
  // Every write tests the same original revision, and only the final write increments it.
  // The single transactional batch prevents stale callers from changing any child rows.
  const results = await db.batch<SnapshotRow>([
    db.prepare('DELETE FROM quest_nodes WHERE snapshot_id = ? AND EXISTS (SELECT 1 FROM snapshots WHERE id = ? AND revision = ?)').bind(id, id, revision),
    db.prepare('DELETE FROM localization_strings WHERE snapshot_id = ? AND EXISTS (SELECT 1 FROM snapshots WHERE id = ? AND revision = ?)').bind(id, id, revision),
    ...childInserts(db, id, pack, revision),
    db.prepare(`UPDATE snapshots SET schema_version = 1, node_count = ?, string_count = ?, pack_metadata_json = ?, source_file_json = ?, updated_at = ?, revision = revision + 1
      WHERE id = ? AND revision = ? RETURNING *`).bind(nodes.length, strings.length, JSON.stringify(packMetadata), file === null ? null : JSON.stringify(file), new Date().toISOString(), id, revision),
    db.prepare('SELECT id FROM snapshots WHERE id = ?').bind(id),
  ]);
  const row = results[4]!.results[0];
  if (!row) throw new ApiError(results[5]!.results.length ? 409 : 404, results[5]!.results.length ? 'api.snapshot_conflict' : 'api.not_found');
  return metadata(row);
}
export async function deleteSnapshot(db: D1Database, id: string, value: unknown): Promise<void> {
  const revision = expectedRevision(value);
  const [deleted, remaining] = await db.batch([
    // Cascading foreign keys delete all nodes and strings within this transaction.
    db.prepare('DELETE FROM snapshots WHERE id = ? AND revision = ? RETURNING id').bind(id, revision),
    db.prepare('SELECT id FROM snapshots WHERE id = ?').bind(id),
  ]);
  if (!deleted!.results.length) throw new ApiError(remaining!.results.length ? 409 : 404, remaining!.results.length ? 'api.snapshot_conflict' : 'api.not_found');
}
export async function loadSnapshot(db: D1Database, id: string): Promise<{ snapshot: SnapshotMetadata; pack: QuestPack }> {
  // One batch observes a consistent metadata/children version during replacement.
  const [snapshots, nodes, strings] = await db.batch<SnapshotRow | { node_json: string } | { entry_json: string }>([
    db.prepare('SELECT * FROM snapshots WHERE id = ? LIMIT 1').bind(id),
    db.prepare('SELECT node_json FROM quest_nodes WHERE snapshot_id = ? ORDER BY ordinal').bind(id),
    db.prepare('SELECT entry_json FROM localization_strings WHERE snapshot_id = ? ORDER BY ordinal').bind(id),
  ]);
  const row = snapshots!.results[0] as SnapshotRow | undefined;
  if (!row) throw new ApiError(404, 'api.not_found');
  try {
    if (nodes!.results.length !== row.node_count || strings!.results.length !== row.string_count || row.schema_version !== 1) throw new Error();
    const pack = normalizeLoadedPack({ ...JSON.parse(row.pack_metadata_json), schemaVersion: 1,
      nodes: nodes!.results.map(node => JSON.parse((node as { node_json: string }).node_json)), strings: strings!.results.map(string => JSON.parse((string as { entry_json: string }).entry_json)) });
    return { snapshot: metadata(row), pack };
  } catch (error) {
    if (error instanceof ValidationError && error.key === 'validation.numeric_quest_id') throw error;
    throw new ApiError(500, 'api.internal_error');
  }
}
