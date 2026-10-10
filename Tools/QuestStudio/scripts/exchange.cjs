const fs = require('node:fs');
const path = require('node:path');
const crypto = require('node:crypto');
const Module = require('node:module');
const ts = require('typescript');
const { parse } = require('csv-parse/sync');

// Use the web tool's validation and localization rather than maintaining a second contract.
function shared(name) {
  const filename = path.join(__dirname, '../src/shared', name + '.ts');
  const compiled = new Module(filename, module);
  compiled.paths = module.paths;
  compiled._compile(ts.transpileModule(fs.readFileSync(filename, 'utf8'), {
    compilerOptions: { target: ts.ScriptTarget.ES2022, module: ts.ModuleKind.CommonJS },
  }).outputText, filename);
  return compiled.exports;
}
const { validatePack, isRecord, isQuestId } = shared('validation');
const { t } = shared('ui-strings');
const fail = key => { throw new Error(t(key)); };
const json = value => JSON.stringify(value, null, 2) + '\n';
const digest = value => crypto.createHash('sha256').update(value).digest('hex');
const readJson = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));

function parseStrings(csv) {
  const rows = parse(csv, { bom: true, skip_empty_lines: true });
  const header = rows.shift();
  if (!header || header[0] !== 'string_key' || !header.includes('ko') || new Set(header).size !== header.length) fail('exchange.csv');
  const result = []; const keys = new Set();
  for (const row of rows) {
    if (!row[0] || keys.has(row[0])) fail('exchange.csv');
    keys.add(row[0]);
    for (let i = 1; i < header.length; i++) if (row[i] !== '') result.push({ key: row[0], locale: header[i], value: row[i] });
  }
  return result;
}
function writeStrings(strings, locales = [...new Set(['ko', 'en', 'ja', ...strings.map(s => s.locale)])]) {
  const keys = [...new Set(strings.map(s => s.key))];
  const values = new Map(strings.map(s => [JSON.stringify([s.key, s.locale]), s.value]));
  const quote = value => /[",\r\n]/.test(value) ? '"' + value.replaceAll('"', '""') + '"' : value;
  return [['string_key', ...locales], ...keys.map(key => [key, ...locales.map(locale => values.get(JSON.stringify([key, locale])) ?? '')])]
    .map(row => row.map(quote).join(',')).join('\r\n') + '\r\n';
}
function parseNotes(csv) {
  const rows = parse(csv, { bom: true, skip_empty_lines: true });
  if (JSON.stringify(rows[0]) !== JSON.stringify(['string_key', 'ko'])) fail('exchange.notesKo');
  return parseStrings(csv);
}
function writeNotes(strings) {
  if (strings.some(s => s.locale !== 'ko')) fail('exchange.notesKo');
  return writeStrings(strings, ['ko']);
}
function stringRefs(value, refs = new Set()) {
  if (Array.isArray(value)) value.forEach(v => stringRefs(v, refs));
  else if (isRecord(value)) for (const [key, v] of Object.entries(value)) {
    if (key.endsWith('_string_key')) refs.add(v);
    else if (key.endsWith('_string_keys')) v.forEach(ref => refs.add(ref));
    else stringRefs(v, refs);
  }
  return refs;
}
function splitStrings(pack) {
  validatePack(pack);
  const runtime = stringRefs(pack.nodes.map(n => n.definition));
  const authoring = stringRefs(pack.nodes.map(({ definition, ...rest }) => rest));
  const isNote = s => authoring.has(s.key) && !runtime.has(s.key);
  const notes = pack.strings.filter(isNote), game = pack.strings.filter(s => !isNote(s));
  writeNotes(notes);
  return { game, notes };
}
function packProject(definitions, metadata, strings) {
  if (!Array.isArray(definitions) || !isRecord(metadata)) fail('exchange.source');
  for (const key of Object.keys(metadata)) if (!/^[1-9]\d*$/.test(key) || !isQuestId(Number(key))) fail('exchange.source');
  const nodes = definitions.map(definition => {
    const meta = metadata[String(definition.quest_id)];
    if (!isRecord(meta)) fail('exchange.metadataMissing');
    const { x, y, cardColor, authoring = {}, ...extra } = meta;
    if (!isRecord(authoring) || (cardColor !== undefined && typeof cardColor !== 'string')) fail('exchange.source');
    return { ...extra, definition, x, y, authoring: { prerequisitesStatus: 'confirmed', ...authoring, ...(cardColor === undefined ? {} : { cardColor }) } };
  });
  return validatePack({ schemaVersion: 1, nodes, strings });
}
function splitPack(pack) {
  validatePack(pack);
  const metadata = {};
  for (const node of pack.nodes) {
    const { definition, x, y, authoring, ...extra } = node;
    const { cardColor, ...context } = authoring;
    metadata[String(definition.quest_id)] = { ...extra, x, y, ...(cardColor === undefined ? {} : { cardColor }), authoring: context };
  }
  return { definitions: pack.nodes.map(n => n.definition), metadata };
}
function extractRemote(remote, localStrings, localMetadata) {
  // Only the definition and x/y/color cross back. Remote strings and other authoring data are ignored.
  if (!isRecord(remote) || remote.schemaVersion !== 1 || !Array.isArray(remote.nodes)) fail('exchange.source');
  const nodes = remote.nodes.map(node => {
    if (!isRecord(node) || !isRecord(node.definition) || !isRecord(node.authoring)) fail('exchange.source');
    const previous = localMetadata[String(node.definition.quest_id)] ?? {};
    const { x, y, cardColor, authoring = {}, ...extra } = previous;
    return { ...extra, definition: node.definition, x: node.x, y: node.y,
      authoring: { prerequisitesStatus: 'confirmed', ...authoring, ...(node.authoring.cardColor === undefined ? {} : { cardColor: node.authoring.cardColor }) } };
  });
  const result = splitPack({ schemaVersion: 1, nodes, strings: localStrings });
  packProject(result.definitions, result.metadata, localStrings);
  return result;
}
function latest(snapshots) {
  if (!Array.isArray(snapshots) || !snapshots.length) fail('exchange.noSnapshots');
  const time = s => Date.parse(s.updatedAt ?? s.createdAt);
  if (snapshots.some(s => !s || typeof s.id !== 'string' || !Number.isFinite(time(s)))) fail('exchange.response');
  return [...snapshots].sort((a, b) => time(b) - time(a) || b.id.localeCompare(a.id))[0];
}

const targetNames = new Set(['Data/QuestDefinitions.json', 'Data/QuestStudioMetadata.json', 'Data/QuestTextStrings.csv', 'Authoring/QuestStudio/QuestStudioNotes.ko.csv']);
function stateDirectory(root) { return path.join(root, '.queststudio-exchange'); }
function safeTarget(root, name) {
  if (!targetNames.has(name)) fail('exchange.source');
  const file = path.join(root, name);
  if (fs.existsSync(file) && fs.lstatSync(file).isSymbolicLink()) fail('exchange.source');
  if (fs.lstatSync(path.dirname(file)).isSymbolicLink()) fail('exchange.source');
  return file;
}
function recover(root) {
  const state = stateDirectory(root), journal = path.join(state, 'pending.json');
  if (!fs.existsSync(journal)) return;
  const entries = readJson(journal);
  for (const entry of entries) {
    const target = safeTarget(root, entry.name);
    const current = fs.existsSync(target) ? digest(fs.readFileSync(target)) : null;
    if (current !== entry.oldHash && current !== entry.newHash) fail('exchange.recoveryConflict');
    if (entry.backup && (!/^backup-[\w-]+\/\d+$/.test(entry.backup) || digest(fs.readFileSync(path.join(state, entry.backup))) !== entry.oldHash)) fail('exchange.recoveryConflict');
  }
  for (const entry of entries) {
    const target = safeTarget(root, entry.name);
    if (entry.backup) fs.copyFileSync(path.join(state, entry.backup), target);
    else fs.rmSync(target, { force: true });
  }
  fs.unlinkSync(journal);
}
function replaceFiles(root, files, afterWrite = () => {}) {
  const state = stateDirectory(root); fs.mkdirSync(state, { recursive: true }); recover(root);
  const backupName = 'backup-' + crypto.randomUUID(); const backup = path.join(state, backupName); fs.mkdirSync(backup);
  const entries = Object.entries(files).map(([name, value], index) => {
    const target = safeTarget(root, name), existed = fs.existsSync(target);
    const old = existed ? fs.readFileSync(target) : null;
    if (existed) fs.writeFileSync(path.join(backup, String(index)), old);
    fs.writeFileSync(path.join(backup, String(index) + '.new'), value);
    return { name, backup: existed ? backupName + '/' + index : null, oldHash: old === null ? null : digest(old), newHash: digest(value) };
  });
  fs.writeFileSync(path.join(backup, 'manifest.json'), json(entries));
  const journal = path.join(state, 'pending.json');
  fs.writeFileSync(journal + '.tmp', json(entries)); fs.renameSync(journal + '.tmp', journal);
  try {
    entries.forEach((entry, i) => { fs.renameSync(path.join(backup, String(i) + '.new'), safeTarget(root, entry.name)); afterWrite(i); });
    fs.unlinkSync(journal);
  } catch (error) { recover(root); throw error; }
  return backup;
}
function lock(root) {
  const state = stateDirectory(root); fs.mkdirSync(state, { recursive: true });
  const file = path.join(state, 'lock');
  if (fs.existsSync(file)) {
    const pid = Number(fs.readFileSync(file, 'utf8'));
    if (!Number.isSafeInteger(pid) || pid <= 0) fail('exchange.locked');
    try { process.kill(pid, 0); fail('exchange.locked'); }
    catch (error) { if (error.code !== 'ESRCH') throw error; }
    fs.unlinkSync(file);
  }
  const fd = fs.openSync(file, 'wx'); fs.writeFileSync(fd, String(process.pid)); fs.closeSync(fd);
  return () => fs.unlinkSync(file);
}

module.exports = { t, json, readJson, parseStrings, writeStrings, parseNotes, writeNotes, splitStrings, packProject, splitPack, extractRemote, latest, replaceFiles, recover, lock, digest };
