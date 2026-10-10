import { env } from 'cloudflare:workers';
import { beforeEach, describe, expect, it } from 'vitest';
import worker from '../src/worker/index';
import type { QuestPack, SnapshotMetadata } from '../src/shared/types';

function pack(): QuestPack {
  return { schemaVersion: 1, extra: { format: 'preserve' }, nodes: [{
    definition: { quest_id: 1, title_string_key: 'synthetic.title', description_string_key: 'synthetic.desc', authoring_tags: ['chapter:1'], required_completed_quest_ids: [], unknown: { rewards: [1, 2] } },
    x: 12, y: 34, authoring: { prerequisitesStatus: 'unspecified', detail_string_key: 'synthetic.desc', unknown: ['preserved'] },
  }], strings: [
    { key: 'synthetic.title', locale: 'ko', value: '합성 제목', extra: 3 },
    { key: 'synthetic.desc', locale: 'ko', value: '합성 설명' },
    { key: 'synthetic.title', locale: 'en', value: 'Synthetic title' },
  ] };
}

const origin = 'https://quest.example.test';
function request(path: string, method = 'GET', body?: unknown, cookie?: string, headers: Record<string, string> = {}) {
  return worker.fetch(new Request(`${origin}${path}`, {
    method, headers: { ...(body === undefined ? {} : { 'content-type': 'application/json' }), ...(cookie ? { cookie } : {}), ...headers },
    ...(body === undefined ? {} : { body: JSON.stringify(body) }),
  }), env);
}
async function login() {
  const response = await request('/api/login', 'POST', { password: env.ADMIN_PASSWORD });
  expect(response.status).toBe(200);
  return response.headers.get('set-cookie')!.split(';')[0]!;
}
beforeEach(async () => {
  await env.DB.exec('DROP TRIGGER IF EXISTS fail_strings; DELETE FROM localization_strings; DELETE FROM quest_nodes; DELETE FROM snapshots; DELETE FROM admin_sessions; DELETE FROM admin_login_attempts;');
});

describe('administrator authentication', () => {
  it('requires authentication on all snapshot routes without exposing data', async () => {
    for (const [path, method] of [['/api/snapshots', 'GET'], ['/api/snapshots/a', 'GET'], ['/api/snapshots', 'POST']] as const) {
      const response = await request(path, method, method === 'POST' ? {} : undefined);
      expect(response.status).toBe(401); expect(await response.json()).toEqual({ error: { key: 'api.unauthorized' } });
    }
    expect(await (await request('/api/session')).json()).toEqual({ authenticated: false, authConfigured: true });
  });
  it('issues a secure cookie, stores only a hash, and revokes on logout', async () => {
    const response = await request('/api/login', 'POST', { password: env.ADMIN_PASSWORD });
    const header = response.headers.get('set-cookie')!;
    expect(header).toContain('__Host-quest_session='); expect(header).toContain('HttpOnly'); expect(header).toContain('SameSite=Strict'); expect(header).toContain('Secure');
    const cookie = header.split(';')[0]!; const rawToken = cookie.split('=')[1];
    const session = await env.DB.prepare('SELECT token_hash FROM admin_sessions').first<{ token_hash: string }>();
    expect(session!.token_hash).toHaveLength(64); expect(session!.token_hash).not.toBe(rawToken);
    expect(await (await request('/api/session', 'GET', undefined, cookie)).json()).toEqual({ authenticated: true, authConfigured: true });
    const logout = await request('/api/logout', 'POST', undefined, cookie);
    expect(logout.headers.get('set-cookie')).toContain('Max-Age=0');
    expect((await request('/api/snapshots', 'GET', undefined, cookie)).status).toBe(401);
  });
  it('rejects expired sessions', async () => {
    const cookie = await login(); await env.DB.prepare('UPDATE admin_sessions SET expires_at = 0').run();
    expect((await request('/api/snapshots', 'GET', undefined, cookie)).status).toBe(401);
  });
  it('blocks an address after five failed passwords and does not allow correct password during block', async () => {
    for (let n = 0; n < 5; n++) expect((await request('/api/login', 'POST', { password: 'bad' })).status).toBe(401);
    const blocked = await request('/api/login', 'POST', { password: env.ADMIN_PASSWORD });
    expect(blocked.status).toBe(429); expect(Number(blocked.headers.get('retry-after'))).toBeGreaterThan(0);
    expect((await request('/api/login', 'POST', { password: env.ADMIN_PASSWORD }, undefined, { 'cf-connecting-ip': '198.51.100.2' })).status).toBe(200);
    await env.DB.prepare('UPDATE admin_login_attempts SET blocked_until = 0, window_started_at = 0').run();
    expect((await request('/api/login', 'POST', { password: env.ADMIN_PASSWORD })).status).toBe(200);
  });
  it('rejects foreign mutation origins and nonlocal insecure login transport', async () => {
    const cookie = await login();
    for (const path of ['/api/login', '/api/logout', '/api/snapshots']) {
      expect((await request(path, 'POST', {}, cookie, { origin: 'https://other.example.test' })).status).toBe(403);
    }
    expect((await worker.fetch(new Request('http://unsafe.example.test/api/login', { method: 'POST' }), env)).status).toBe(403);
  });
  it('reports missing password configuration without accepting login', async () => {
    const missing = { ...env, ADMIN_PASSWORD: '' };
    const session = await worker.fetch(new Request(`${origin}/api/session`), missing);
    expect(await session.json()).toEqual({ authenticated: false, authConfigured: false });
    expect((await worker.fetch(new Request(`${origin}/api/login`, { method: 'POST' }), missing)).status).toBe(503);
  });
  it('accepts a ten-character configured password and authenticates its session', async () => {
    const configured = { ...env, ADMIN_PASSWORD: 'test-12345' };
    expect(await (await worker.fetch(new Request(`${origin}/api/session`), configured)).json())
      .toEqual({ authenticated: false, authConfigured: true });
    const attempt = (password: string) => worker.fetch(new Request(`${origin}/api/login`, {
      method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ password }),
    }), configured);
    expect((await attempt('wrong-1234')).status).toBe(401);
    const response = await attempt(configured.ADMIN_PASSWORD);
    expect(response.status).toBe(200);
    const cookie = response.headers.get('set-cookie')!.split(';')[0]!;
    expect(await (await worker.fetch(new Request(`${origin}/api/session`, { headers: { cookie } }), configured)).json())
      .toEqual({ authenticated: true, authConfigured: true });
  });
  it('does not enable authentication for a password shorter than ten characters', async () => {
    const tooShort = { ...env, ADMIN_PASSWORD: 'test-1234' };
    expect(await (await worker.fetch(new Request(`${origin}/api/session`), tooShort)).json())
      .toEqual({ authenticated: false, authConfigured: false });
    expect((await worker.fetch(new Request(`${origin}/api/login`, { method: 'POST' }), tooShort)).status).toBe(503);
  });
  it('allows localhost development cookies and rejects malformed cookie values', async () => {
    const response = await worker.fetch(new Request('http://localhost/api/login', { method: 'POST', headers: { 'content-type': 'application/json' }, body: JSON.stringify({ password: env.ADMIN_PASSWORD }) }), env);
    expect(response.status).toBe(200);
    expect(response.headers.get('set-cookie')).toContain('quest_session=');
    expect(response.headers.get('set-cookie')).not.toContain('Secure');
    expect((await request('/api/snapshots', 'GET', undefined, '__Host-quest_session=malformed')).status).toBe(401);
  });
});

describe('complete immutable snapshots', () => {
  it('explains legacy regional snapshot IDs without overwriting the stored snapshot', async () => {
    const cookie = await login();
    const saved = await request('/api/snapshots', 'POST', { alias: 'legacy fixture', memo: '', pack: pack() }, cookie);
    const { snapshot } = await saved.json() as { snapshot: SnapshotMetadata };
    await env.DB.prepare("UPDATE quest_nodes SET node_json = json_set(node_json, '$.definition.quest_id', 'R1-01') WHERE snapshot_id = ?").bind(snapshot.id).run();
    const loaded = await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie);
    expect(loaded.status).toBe(422);
    expect(await loaded.json()).toEqual({ error: { key: 'validation.numeric_quest_id' } });
    expect(await env.DB.prepare("SELECT json_extract(node_json, '$.definition.quest_id') AS id FROM quest_nodes WHERE snapshot_id = ?").bind(snapshot.id).first()).toEqual({ id: 'R1-01' });
  });
  it('stores and restores full JSON, metadata, locale entries and original ordering without seeding', async () => {
    const cookie = await login(); expect(await (await request('/api/snapshots', 'GET', undefined, cookie)).json()).toEqual({ snapshots: [] });
    const value = pack(); const result = await request('/api/snapshots', 'POST', { alias: '합성 저장', memo: '합성 메모', pack: value }, cookie);
    expect(result.status).toBe(201);
    const { snapshot } = await result.json() as { snapshot: SnapshotMetadata };
    expect(snapshot).toMatchObject({ alias: '합성 저장', memo: '합성 메모', nodeCount: 1, stringCount: 3 });
    expect(Number.isNaN(Date.parse(snapshot.createdAt))).toBe(false);
    expect(await (await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: value });
    const second = await request('/api/snapshots', 'POST', { alias: 'second', memo: '', pack: { ...value, nodes: [] } }, cookie);
    const secondId = (await second.json() as { snapshot: SnapshotMetadata }).snapshot.id;
    expect(secondId).not.toBe(snapshot.id);
    expect(await (await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: value });
    expect((await (await request('/api/snapshots', 'GET', undefined, cookie)).json() as { snapshots: SnapshotMetadata[] }).snapshots).toHaveLength(2);
  });
  it('rolls back metadata and nodes if localization insertion fails', async () => {
    const cookie = await login();
    await env.DB.exec("CREATE TRIGGER fail_strings BEFORE INSERT ON localization_strings BEGIN SELECT RAISE(ABORT, 'injected write failure'); END;");
    expect((await request('/api/snapshots', 'POST', { alias: 'rollback', memo: '', pack: pack() }, cookie)).status).toBe(500);
    for (const table of ['snapshots', 'quest_nodes', 'localization_strings']) {
      const count = await env.DB.prepare(`SELECT count(*) AS count FROM ${table}`).first<{ count: number }>();
      expect(count!.count).toBe(0);
    }
  });
  it('validates packs before storing and returns keyed errors', async () => {
    const cookie = await login(); const value = pack(); value.nodes[0]!.definition.required_completed_quest_ids = [999];
    const response = await request('/api/snapshots', 'POST', { alias: 'invalid', memo: '', pack: value }, cookie);
    expect(response.status).toBe(422); expect(await response.json()).toEqual({ error: { key: 'validation.unknown_quest', params: { questId: 999 } } });
    expect((await request('/api/snapshots/no-such-id', 'GET', undefined, cookie)).status).toBe(404);
  });
  it('rejects invalid metadata, malformed JSON, media types and oversized input before storing', async () => {
    const cookie = await login();
    expect((await request('/api/snapshots', 'POST', { alias: '', memo: '', pack: pack() }, cookie)).status).toBe(422);
    const options = { method: 'POST', headers: { cookie, 'content-type': 'application/json' } };
    expect((await worker.fetch(new Request(`${origin}/api/snapshots`, { ...options, body: '{invalid' }), env)).status).toBe(422);
    expect((await request('/api/snapshots', 'POST', {}, cookie, { 'content-type': 'application/json-invalid' })).status).toBe(415);
    expect((await request('/api/snapshots', 'POST', {}, cookie, { 'content-length': String(9 * 1024 * 1024) })).status).toBe(413);
    expect(await (await request('/api/snapshots', 'GET', undefined, cookie)).json()).toEqual({ snapshots: [] });
  });
});

it('loads legacy string identities numerically without rewriting D1 snapshot JSON', async () => {
  const cookie = await login();
  const saved = await request('/api/snapshots', 'POST', { alias: 'legacy numbers', memo: '', pack: pack() }, cookie);
  const { snapshot } = await saved.json() as { snapshot: SnapshotMetadata };
  await env.DB.prepare("UPDATE quest_nodes SET node_json = json_set(node_json, '$.definition.quest_id', '1') WHERE snapshot_id = ?").bind(snapshot.id).run();
  const before = await env.DB.prepare('SELECT node_json FROM quest_nodes WHERE snapshot_id = ?').bind(snapshot.id).first();
  const loaded = await request('/api/snapshots/' + snapshot.id, 'GET', undefined, cookie);
  expect(loaded.status).toBe(200);
  expect(await loaded.json()).toEqual({ snapshot, pack: pack() });
  expect(await env.DB.prepare('SELECT node_json FROM quest_nodes WHERE snapshot_id = ?').bind(snapshot.id).first()).toEqual(before);
  expect(await env.DB.prepare('SELECT count(*) AS count FROM snapshots').first()).toEqual({ count: 1 });
});

describe('snapshot replacement and deletion', () => {
  async function create(cookie: string) {
    const response = await request('/api/snapshots', 'POST', { alias: 'keep alias', memo: 'keep memo', pack: pack(), sourceFile: { name: 'original.json', lastModified: 123 } }, cookie);
    expect(response.status).toBe(201);
    return (await response.json() as { snapshot: SnapshotMetadata }).snapshot;
  }
  it('replaces the complete pack in place and preserves identity with a new revision', async () => {
    const cookie = await login(); const original = await create(cookie);
    expect(original).toMatchObject({ revision: 1, updatedAt: original.createdAt, sourceFile: { name: 'original.json', lastModified: 123 } });
    const replacement = { schemaVersion: 1, replacementOnly: true, nodes: [], strings: [] };
    const response = await request(`/api/snapshots/${original.id}/overwrite`, 'POST', { pack: replacement, sourceFile: { name: 'replacement.json', lastModified: 456 }, expectedRevision: 1 }, cookie);
    expect(response.status).toBe(200);
    const { snapshot } = await response.json() as { snapshot: SnapshotMetadata };
    expect(snapshot).toMatchObject({ id: original.id, alias: original.alias, memo: original.memo, createdAt: original.createdAt, revision: 2, nodeCount: 0, stringCount: 0, sourceFile: { name: 'replacement.json', lastModified: 456 } });
    expect(Date.parse(snapshot.updatedAt)).toBeGreaterThanOrEqual(Date.parse(original.updatedAt));
    expect(await (await request(`/api/snapshots/${original.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: replacement });
    for (const action of ['overwrite', 'delete']) {
      const stale = await request(`/api/snapshots/${original.id}/${action}`, 'POST', { expectedRevision: 1, pack: pack(), sourceFile: null }, cookie);
      expect(stale.status).toBe(409); expect(await stale.json()).toEqual({ error: { key: 'api.snapshot_conflict' } });
    }
    expect(await (await request(`/api/snapshots/${original.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: replacement });
  });
  it('rolls back every replacement change when a child insertion fails', async () => {
    const cookie = await login(); const snapshot = await create(cookie);
    await env.DB.exec("CREATE TRIGGER fail_strings BEFORE INSERT ON localization_strings BEGIN SELECT RAISE(ABORT, 'injected write failure'); END;");
    const replacement = pack(); replacement.extra = 'changed'; replacement.nodes[0]!.x = 999;
    expect((await request(`/api/snapshots/${snapshot.id}/overwrite`, 'POST', { pack: replacement, sourceFile: null, expectedRevision: 1 }, cookie)).status).toBe(500);
    expect(await (await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: pack() });
  });
  it('deletes all snapshot rows and reports missing targets', async () => {
    const cookie = await login(); const snapshot = await create(cookie);
    expect((await request(`/api/snapshots/${snapshot.id}/delete`, 'POST', { expectedRevision: 1 }, cookie)).status).toBe(200);
    for (const table of ['snapshots', 'quest_nodes', 'localization_strings']) expect(await env.DB.prepare(`SELECT count(*) AS count FROM ${table}`).first()).toEqual({ count: 0 });
    for (const action of ['delete', 'overwrite']) expect((await request(`/api/snapshots/${snapshot.id}/${action}`, 'POST', { expectedRevision: 1, pack: pack(), sourceFile: null }, cookie)).status).toBe(404);
  });
  it('guards both mutations by authentication, origin, revision and source metadata validation', async () => {
    const cookie = await login(); const snapshot = await create(cookie);
    for (const action of ['delete', 'overwrite']) {
      const path = `/api/snapshots/${snapshot.id}/${action}`;
      expect((await request(path, 'POST', {})).status).toBe(401);
      expect((await request(path, 'POST', {}, cookie, { origin: 'https://foreign.test' })).status).toBe(403);
      expect((await request(path, 'POST', { expectedRevision: 0, pack: pack(), sourceFile: null }, cookie)).status).toBe(422);
    }
    for (const sourceFile of [{ name: '', lastModified: 1 }, { name: 'a', lastModified: -1 }, { name: 'a', lastModified: 'bad' }]) expect((await request('/api/snapshots', 'POST', { alias: 'invalid', memo: '', pack: pack(), sourceFile }, cookie)).status).toBe(422);
  });
  it('defaults metadata for snapshots without file provenance', async () => {
    const cookie = await login(); const response = await request('/api/snapshots', 'POST', { alias: 'legacy compatible', memo: '', pack: pack() }, cookie);
    const { snapshot } = await response.json() as { snapshot: SnapshotMetadata };
    expect(snapshot).toMatchObject({ sourceFile: null, revision: 1, updatedAt: snapshot.createdAt });
  });
});

it('allows exactly one concurrent replacement per revision and keeps one complete winner', async () => {
  const cookie = await login();
  const { snapshot } = await (await request('/api/snapshots', 'POST', { alias: 'race', memo: '', pack: pack() }, cookie)).json() as { snapshot: SnapshotMetadata };
  const first = pack(); first.extra = 'first'; first.nodes[0]!.x = 100;
  const second = pack(); second.extra = 'second'; second.nodes[0]!.x = 200; second.strings[0]!.value = 'second';
  const responses = await Promise.all([first, second].map(value => request(`/api/snapshots/${snapshot.id}/overwrite`, 'POST', { pack: value, sourceFile: null, expectedRevision: 1 }, cookie)));
  expect(responses.map(response => response.status).sort()).toEqual([200, 409]);
  const winner = responses[0]!.status === 200 ? first : second;
  const loaded = await (await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie)).json() as { snapshot: SnapshotMetadata; pack: QuestPack };
  expect(loaded.pack).toEqual(winner); expect(loaded.snapshot.revision).toBe(2);
});

it('rolls back parent and child deletion when a child delete fails', async () => {
  const cookie = await login();
  const { snapshot } = await (await request('/api/snapshots', 'POST', { alias: 'delete rollback', memo: '', pack: pack() }, cookie)).json() as { snapshot: SnapshotMetadata };
  await env.DB.exec("CREATE TRIGGER fail_strings BEFORE DELETE ON localization_strings BEGIN SELECT RAISE(ABORT, 'injected delete failure'); END;");
  expect((await request(`/api/snapshots/${snapshot.id}/delete`, 'POST', { expectedRevision: 1 }, cookie)).status).toBe(500);
  expect(await (await request(`/api/snapshots/${snapshot.id}`, 'GET', undefined, cookie)).json()).toEqual({ snapshot, pack: pack() });
});
