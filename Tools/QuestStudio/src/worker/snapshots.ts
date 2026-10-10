import type { QuestPack, SnapshotMetadata } from '../shared/types';
import { isRecord, validatePack } from '../shared/validation';
import { ApiError } from './http';

interface SnapshotRow {
  id: string; alias: string; memo: string; created_at: string;
  schema_version: number; node_count: number; string_count: number; pack_metadata_json: string;
}
const metadata = (row: SnapshotRow): SnapshotMetadata => ({
  id: row.id, alias: row.alias, memo: row.memo, createdAt: row.created_at, nodeCount: row.node_count, stringCount: row.string_count,
});
export async function listSnapshots(db: D1Database): Promise<SnapshotMetadata[]> {
  const { results } = await db.prepare('SELECT * FROM snapshots ORDER BY created_at DESC, id DESC').all<SnapshotRow>();
  return results.map(metadata);
}
export async function saveSnapshot(db: D1Database, value: unknown): Promise<SnapshotMetadata> {
  if (!isRecord(value) || typeof value.alias !== 'string' || !value.alias.trim() || value.alias.length > 120 ||
      typeof value.memo !== 'string' || value.memo.length > 2000) throw new ApiError(422, 'api.invalid_snapshot');
  const pack = validatePack(value.pack); const id = crypto.randomUUID(); const createdAt = new Date().toISOString();
  const { nodes, strings, ...packMetadata } = pack;
  const snapshot: SnapshotMetadata = { id, alias: value.alias, memo: value.memo, createdAt, nodeCount: nodes.length, stringCount: strings.length };
  // D1 batch is transactional: metadata, every node and every string commit together.
  // json_each keeps the statement count constant even for large complete packs.
  await db.batch([
    db.prepare(`INSERT INTO snapshots (id, alias, memo, created_at, schema_version, node_count, string_count, pack_metadata_json)
      VALUES (?, ?, ?, ?, 1, ?, ?, ?)`).bind(id, snapshot.alias, snapshot.memo, createdAt, nodes.length, strings.length, JSON.stringify(packMetadata)),
    db.prepare(`INSERT INTO quest_nodes (snapshot_id, quest_id, ordinal, node_json)
      SELECT ?, json_extract(value, '$.definition.quest_id'), CAST(key AS INTEGER), value FROM json_each(?)`)
      .bind(id, JSON.stringify(nodes)),
    db.prepare(`INSERT INTO localization_strings (snapshot_id, string_key, locale, value, ordinal, entry_json)
      SELECT ?, json_extract(value, '$.key'), json_extract(value, '$.locale'), json_extract(value, '$.value'), CAST(key AS INTEGER), value FROM json_each(?)`)
      .bind(id, JSON.stringify(strings)),
  ]);
  return snapshot;
}
export async function loadSnapshot(db: D1Database, id: string): Promise<{ snapshot: SnapshotMetadata; pack: QuestPack }> {
  const row = await db.prepare('SELECT * FROM snapshots WHERE id = ? LIMIT 1').bind(id).first<SnapshotRow>();
  if (!row) throw new ApiError(404, 'api.not_found');
  const [nodes, strings] = await Promise.all([
    db.prepare('SELECT node_json FROM quest_nodes WHERE snapshot_id = ? ORDER BY ordinal').bind(id).all<{ node_json: string }>(),
    db.prepare('SELECT entry_json FROM localization_strings WHERE snapshot_id = ? ORDER BY ordinal').bind(id).all<{ entry_json: string }>(),
  ]);
  try {
    if (nodes.results.length !== row.node_count || strings.results.length !== row.string_count || row.schema_version !== 1) throw new Error();
    const pack = validatePack({ ...JSON.parse(row.pack_metadata_json), schemaVersion: 1,
      nodes: nodes.results.map(node => JSON.parse(node.node_json)), strings: strings.results.map(string => JSON.parse(string.entry_json)) });
    return { snapshot: metadata(row), pack };
  } catch { throw new ApiError(500, 'api.internal_error'); }
}
