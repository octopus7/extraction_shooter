import { test } from 'node:test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import exchange from '../scripts/exchange.cjs';
import { createClient } from '../scripts/exchange-api.cjs';
import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';

const strings = [{ key: 'q.title', locale: 'ko', value: '한글, "제목"\n두 줄' }, { key: 'q.desc', locale: 'ko', value: '설명' }];
const definition = { quest_id: 1, title_string_key: 'q.title', description_string_key: 'q.desc', required_completed_quest_ids: [], authoring_tags: ['chapter:1'], objectives: [{ custom: 17 }] };
const pack = () => ({ schemaVersion: 1, nodes: [{ definition: structuredClone(definition), x: -15, y: 25, authoring: { prerequisitesStatus: 'confirmed', cardColor: 'blue', detail_string_key: 'q.desc' } }], strings: structuredClone(strings) });
function workspace(fn) {
  const root = fs.mkdtempSync(path.join(os.tmpdir(), 'quest-exchange-'));
  try { return fn(root); } finally { fs.rmSync(root, { recursive: true, force: true }); }
}

test('CSV and pack round trip preserve definitions, editor metadata, and Korean multiline strings', () => {
  const p = pack(); const files = exchange.splitPack(p);
  assert.deepEqual(exchange.packProject(files.definitions, files.metadata, exchange.parseStrings(exchange.writeStrings(p.strings))), p);
});
test('pull uses local strings and preserves local authoring context, never remote string contents', () => {
  const p = pack(); p.nodes[0].authoring.prerequisitesStatus = 'unspecified'; const local = exchange.splitPack(p);
  p.strings[0].value = 'REMOTE'; p.nodes[0].authoring.cardColor = 'rose';
  p.nodes[0].authoring.detail_string_key = 'untrusted.remote.key';
  const result = exchange.extractRemote(p, strings, local.metadata);
  assert.equal(result.metadata['1'].cardColor, 'rose');
  assert.equal(result.metadata['1'].authoring.detail_string_key, 'q.desc');
  assert.equal(result.metadata['1'].authoring.prerequisitesStatus, 'unspecified');
  assert.deepEqual(Object.keys(result).sort(), ['definitions', 'metadata']);
  assert.equal(strings[0].value, '한글, "제목"\n두 줄');
  p.nodes[0].definition.title_string_key = 'missing.local';
  assert.throws(() => exchange.extractRemote(p, strings, local.metadata));
});
test('latest means updatedAt with deterministic tie break and old snapshot fallback', () => {
  assert.equal(exchange.latest([{ id: 'new', createdAt: '2026-10-10', updatedAt: '2026-10-10' }, { id: 'old', createdAt: '2026-01-01', updatedAt: '2026-10-11' }]).id, 'old');
  assert.equal(exchange.latest([{ id: 'a', createdAt: '2026-10-11' }, { id: 'b', createdAt: '2026-10-11' }]).id, 'b');
  assert.throws(() => exchange.latest([]));
});
test('invalid data fails before either target is written', () => workspace(root => {
  fs.mkdirSync(path.join(root, 'Data'));
  const p = pack(); p.nodes[0].definition.required_completed_quest_ids = [1];
  assert.throws(() => exchange.extractRemote(p, strings, {}));
  assert.deepEqual(fs.readdirSync(path.join(root, 'Data')), []);
}));
test('transaction failure restores both previous files and keeps backups', () => workspace(root => {
  fs.mkdirSync(path.join(root, 'Data'));
  const a = 'Data/QuestDefinitions.json', b = 'Data/QuestStudioMetadata.json';
  fs.writeFileSync(path.join(root, a), 'old-a'); fs.writeFileSync(path.join(root, b), 'old-b');
  assert.throws(() => exchange.replaceFiles(root, { [a]: 'new-a', [b]: 'new-b' }, index => { if (index === 0) throw Error('injected'); }));
  assert.equal(fs.readFileSync(path.join(root, a), 'utf8'), 'old-a');
  assert.equal(fs.readFileSync(path.join(root, b), 'utf8'), 'old-b');
  exchange.replaceFiles(root, { [a]: 'new-a', [b]: 'new-b' });
  assert.equal(fs.readFileSync(path.join(root, a), 'utf8'), 'new-a');
}));

test('interrupted two-file write is recovered on next run', () => workspace(root => {
  fs.mkdirSync(path.join(root, 'Data'));
  const a = 'Data/QuestDefinitions.json', b = 'Data/QuestStudioMetadata.json';
  fs.writeFileSync(path.join(root, a), 'old-a'); fs.writeFileSync(path.join(root, b), 'old-b');
  const script = fileURLToPath(new URL('../scripts/exchange.cjs', import.meta.url));
  const result = spawnSync(process.execPath, ['-e', `require(${JSON.stringify(script)}).replaceFiles(${JSON.stringify(root)}, ${JSON.stringify({[a]: 'new-a', [b]: 'new-b'})}, () => process.exit(19))`]);
  assert.equal(result.status, 19);
  exchange.recover(root);
  assert.equal(fs.readFileSync(path.join(root, a), 'utf8'), 'old-a');
  assert.equal(fs.readFileSync(path.join(root, b), 'utf8'), 'old-b');
}));
test('metadata mismatch and malformed CSV are rejected', () => {
  assert.throws(() => exchange.packProject([definition], {}, strings));
  assert.throws(() => exchange.parseStrings('string_key,ko\na,one\na,two'));
  const p = pack(); p.nodes[0].authoring.cardColor = 17;
  assert.throws(() => exchange.extractRemote(p, strings, {}));
});
test('authenticated API exchange uses cookie, creates new snapshot, chooses updated snapshot and logs out', async () => {
  const token = 'A'.repeat(43); const routes = [];
  const selected = { id: 'older-created', revision: 2, createdAt: '2026-01-01', updatedAt: '2026-10-11' };
  const client = createClient('https://example.test', async (url, options) => {
    routes.push(url.pathname);
    assert.equal(options.redirect, 'error');
    assert.equal(options.headers.origin, 'https://example.test');
    if (url.pathname === '/api/login') {
      assert.equal(JSON.parse(options.body).password, 'synthetic-password');
      return Response.json({ authenticated: true }, { headers: { 'set-cookie': '__Host-quest_session=' + token + '; Path=/; HttpOnly' } });
    }
    assert.equal(options.headers.cookie, '__Host-quest_session=' + token);
    if (url.pathname === '/api/snapshots' && options.method === 'POST') return Response.json({ snapshot: { id: 'new' } });
    if (url.pathname === '/api/snapshots') return Response.json({ snapshots: [selected, { id: 'newer-created', createdAt: '2026-10-10', updatedAt: '2026-10-10' }] });
    if (url.pathname === '/api/logout') return Response.json({ authenticated: false });
    return Response.json({ snapshot: selected, pack: pack() });
  });
  await client.login('synthetic-password');
  assert.equal((await client.upload({ pack: pack() })).snapshot.id, 'new');
  assert.equal((await client.downloadLatest()).snapshot.id, 'older-created');
  await client.logout();
  assert.equal(routes.at(-1), '/api/logout');
});
test('network/HTTP failures never echo credentials and never retry uploads', async () => {
  let calls = 0;
  const client = createClient('https://example.test', async () => { calls++; return new Response('synthetic-password', { status: 401 }); });
  await assert.rejects(client.login('synthetic-password'), e => !e.message.includes('synthetic-password'));
  assert.equal(calls, 1);
  assert.throws(() => createClient('http://example.test'));
});
test('unstable latest snapshot does not yield a stale pack', async () => {
  let revision = 0;
  const client = createClient('https://example.test', async url => {
    const snapshot = { id: 'a', revision: ++revision, updatedAt: '2026-10-11' };
    return Response.json(url.pathname === '/api/snapshots' ? { snapshots: [snapshot] } : { snapshot, pack: pack() });
  });
  await assert.rejects(client.downloadLatest());
});
test('both BAT launchers work from another directory with a synthetic authenticated server', { skip: process.platform !== 'win32' }, () => workspace(root => {
  const project = path.join(root, 'TunaSweeper'), studio = path.join(root, 'Tools/QuestStudio');
  const payload = path.join(project, 'External/MainPayload');
  fs.mkdirSync(path.join(project, 'BatchScripts'), { recursive: true });
  fs.mkdirSync(path.join(payload, 'Data'), { recursive: true });
  fs.mkdirSync(path.join(studio, 'scripts'), { recursive: true });
  fs.mkdirSync(path.join(studio, 'src/shared'), { recursive: true });
  fs.mkdirSync(path.join(studio, '.local-admin'), { recursive: true });
  const source = fileURLToPath(new URL('..', import.meta.url));
  fs.symlinkSync(path.join(source, 'node_modules'), path.join(studio, 'node_modules'), 'junction');
  for (const file of ['exchange.cjs', 'exchange-api.cjs', 'quest-exchange.cjs']) fs.copyFileSync(path.join(source, 'scripts', file), path.join(studio, 'scripts', file));
  for (const file of ['validation.ts', 'ui-strings.ts']) fs.copyFileSync(path.join(source, 'src/shared', file), path.join(studio, 'src/shared', file));
  for (const file of ['UploadQuestSnapshot.bat', 'DownloadLatestQuestSnapshot.bat']) fs.copyFileSync(path.resolve(source, '../../TunaSweeper/BatchScripts', file), path.join(project, 'BatchScripts', file));
  const local = exchange.splitPack(pack());
  fs.writeFileSync(path.join(payload, 'Data/QuestDefinitions.json'), JSON.stringify(local.definitions));
  fs.writeFileSync(path.join(payload, 'Data/QuestStudioMetadata.json'), JSON.stringify(local.metadata));
  const csv = exchange.writeStrings(strings);
  fs.writeFileSync(path.join(payload, 'Data/QuestTextStrings.csv'), csv);
  fs.writeFileSync(path.join(studio, '.local-admin/admin-password.txt'), '\uFEFFsynthetic-password\r\n');
  const remote = pack(); remote.nodes[0].x = 123; remote.nodes[0].authoring.cardColor = 'rose'; remote.nodes[0].definition.custom = 'remote'; remote.strings[0].value = 'REMOTE';
  const mock = path.join(root, 'mock.cjs');
  fs.writeFileSync(mock, `const fs=require('fs'); const remote=${JSON.stringify(remote)};
    global.fetch=async(url,opts)=>{
      if(url.pathname==='/api/login'){if(JSON.parse(opts.body).password!=='synthetic-password')throw Error('password');return Response.json({}, {headers:{'set-cookie':'__Host-quest_session='+'A'.repeat(43)+'; Path=/'}});}
      if(!opts.headers.cookie)throw Error('cookie');
      if(url.pathname==='/api/logout')return Response.json({});
      const snapshot={id:'latest',revision:3,updatedAt:'2026-10-11'};
      if(opts.method==='POST'){fs.writeFileSync(${JSON.stringify(path.join(root,'uploaded.json'))},opts.body);return Response.json({snapshot});}
      return Response.json(url.pathname==='/api/snapshots'?{snapshots:[snapshot]}:{snapshot,pack:remote});
    };`);
  const run = name => spawnSync('cmd.exe', ['/d', '/c', path.join(project, 'BatchScripts', name)], {
    cwd: os.tmpdir(), encoding: 'utf8', timeout: 15000,
    env: { ...process.env, QUEST_EXCHANGE_NO_PAUSE: '1', NODE_OPTIONS: `--require "${mock.replaceAll('\\', '/')}"` },
  });
  const upload = run('UploadQuestSnapshot.bat');
  assert.equal(upload.status, 0, upload.stdout + upload.stderr);
  assert.deepEqual(JSON.parse(fs.readFileSync(path.join(root, 'uploaded.json'))).pack.nodes, pack().nodes);
  const download = run('DownloadLatestQuestSnapshot.bat');
  assert.equal(download.status, 0, download.stdout + download.stderr);
  assert.equal(fs.readFileSync(path.join(payload, 'Data/QuestTextStrings.csv'), 'utf8'), csv);
  assert.equal(JSON.parse(fs.readFileSync(path.join(payload, 'Data/QuestDefinitions.json')))[0].custom, 'remote');
  assert.equal(JSON.parse(fs.readFileSync(path.join(payload, 'Data/QuestStudioMetadata.json')))['1'].x, 123);
  assert(![upload.stdout,upload.stderr,download.stdout,download.stderr].join('').includes('synthetic-password'));
}));
