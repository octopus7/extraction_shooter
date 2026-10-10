const fs = require('node:fs');
const path = require('node:path');
const { t, json, readJson, parseStrings, writeStrings, packProject, splitPack, extractRemote, replaceFiles, recover, lock, digest } = require('./exchange.cjs');
const { createClient } = require('./exchange-api.cjs');
const repo = path.resolve(__dirname, '../../..');
const root = path.join(repo, 'TunaSweeper/External/MainPayload');
const names = ['Data/QuestDefinitions.json', 'Data/QuestStudioMetadata.json', 'Data/QuestTextStrings.csv'];
const read = name => fs.readFileSync(path.join(root, name));
const baseline = () => names.map(name => fs.existsSync(path.join(root, name)) ? digest(read(name)) : null);
const unchanged = hashes => { if (JSON.stringify(hashes) !== JSON.stringify(baseline())) throw new Error(t('exchange.localChanged')); };
const say = (key, values) => console.log(t(key, values));

async function main(mode) {
  if (!['upload', 'download', 'initialize'].includes(mode) || process.argv.length > 3) throw new Error(t('exchange.usage'));
  if (!fs.existsSync(path.join(root, 'Data'))) throw new Error(t('exchange.source'));
  const release = lock(root);
  let client;
  try {
    recover(root);
    const hashes = baseline();
    if (mode === 'initialize') {
      if (readJson(path.join(root, names[0])).length || parseStrings(read(names[2])).length || fs.existsSync(path.join(root, names[1]))) throw new Error(t('exchange.initialized'));
      const pack = readJson(path.join(root, 'Authoring/QuestStudio/quest-pack.ko.json'));
      const result = splitPack(pack);
      const csv = writeStrings(pack.strings);
      packProject(result.definitions, result.metadata, parseStrings(csv));
      unchanged(hashes);
      const backup = replaceFiles(root, { [names[0]]: json(result.definitions), [names[1]]: json(result.metadata), [names[2]]: csv });
      say('exchange.initializedDone', { count: result.definitions.length, backup });
      return;
    }
    const strings = parseStrings(read(names[2]));
    const metadata = fs.existsSync(path.join(root, names[1])) ? readJson(path.join(root, names[1])) : {};
    let outgoing;
    if (mode === 'upload') {
      outgoing = packProject(readJson(path.join(root, names[0])), metadata, strings);
      if (!outgoing.nodes.length) throw new Error(t('exchange.empty'));
    }
    const secretPath = path.join(repo, 'Tools/QuestStudio/.local-admin/admin-password.txt');
    let password;
    try { password = fs.readFileSync(secretPath, 'utf8').replace(/^\uFEFF/, '').replace(/\r?\n$/, ''); }
    catch { throw new Error(t('exchange.passwordFile', { path: secretPath })); }
    if (!password || /[\r\n]/.test(password)) throw new Error(t('exchange.passwordFile', { path: secretPath }));
    client = createClient('https://quest.oc7.workers.dev');
    try { await client.login(password); } finally { password = undefined; }
    if (mode === 'upload') {
      unchanged(hashes);
      // No automatic retry: a timed-out POST may already have created a snapshot.
      const result = await client.upload({ pack: outgoing, alias: t('exchange.alias', { time: new Date().toISOString() }), memo: t('exchange.memo'), sourceFile: null });
      if (!result.snapshot?.id) throw new Error(t('exchange.response'));
      say('exchange.uploaded', { id: result.snapshot.id, count: outgoing.nodes.length });
    } else {
      const loaded = await client.downloadLatest();
      const result = extractRemote(loaded.pack, strings, metadata);
      if (!result.definitions.length) throw new Error(t('exchange.empty'));
      unchanged(hashes);
      say('exchange.selected', { id: loaded.snapshot.id, time: loaded.snapshot.updatedAt, revision: loaded.snapshot.revision });
      const previous = readJson(path.join(root, names[0]));
      const byId = new Map(previous.map(d => [d.quest_id, d]));
      say('exchange.diff', { before: previous.length, after: result.definitions.length, changed: result.definitions.filter(d => JSON.stringify(byId.get(d.quest_id)) !== JSON.stringify(d)).length });
      const backup = replaceFiles(root, { [names[0]]: json(result.definitions), [names[1]]: json(result.metadata) });
      say('exchange.downloaded', { backup });
    }
  } finally {
    try { if (client) await client.logout(); } catch { say('exchange.logoutFailed'); }
    release();
  }
}
main(process.argv[2]).catch(error => {
  // Only our localized operational/validation errors are shown; native errors can contain data.
  const message = error.key ? t(error.key, error.params) : error.message;
  const safe = typeof message === 'string' && /[가-힣]/.test(message) && !message.includes('\n');
  console.error(safe ? message : t('exchange.failed'));
  process.exitCode = 1;
});
