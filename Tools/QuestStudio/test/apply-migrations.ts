import { env } from 'cloudflare:workers';
import { applyD1Migrations } from 'cloudflare:test';
import type { D1Migration } from '@cloudflare/vitest-pool-workers';
import { expect } from 'vitest';
import { loadSnapshot } from '../src/worker/snapshots';

const testEnv = env as Env & { TEST_MIGRATIONS: D1Migration[] };
// Seed the original schema before upgrading so preservation is tested with a real legacy row.
await applyD1Migrations(testEnv.DB, testEnv.TEST_MIGRATIONS.slice(0, 1));
await testEnv.DB.prepare(`INSERT INTO snapshots (id, alias, memo, created_at, schema_version, node_count, string_count, pack_metadata_json)
  VALUES ('migration-fixture', 'legacy', 'preserved', '2026-01-01T00:00:00.000Z', 1, 0, 0, '{"legacy":true}')`).run();
await applyD1Migrations(testEnv.DB, testEnv.TEST_MIGRATIONS);
expect(await loadSnapshot(testEnv.DB, 'migration-fixture')).toEqual({
  snapshot: { id: 'migration-fixture', alias: 'legacy', memo: 'preserved', createdAt: '2026-01-01T00:00:00.000Z', updatedAt: '2026-01-01T00:00:00.000Z', revision: 1, sourceFile: null, nodeCount: 0, stringCount: 0 },
  pack: { legacy: true, schemaVersion: 1, nodes: [], strings: [] },
});
await testEnv.DB.prepare("DELETE FROM snapshots WHERE id = 'migration-fixture'").run();
