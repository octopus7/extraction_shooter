import test from 'node:test';
import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
const read = (file) => readFile(new URL(`../public/${file}`, import.meta.url), 'utf8');
test('dashboard labels resolve from the Korean string table', async () => {
  const html = await read('index.html');
  const strings = JSON.parse(await read('strings/ko.json'));
  for (const match of html.matchAll(/data-i18n(?:-placeholder|-label)?="([^"]+)"/g)) assert.ok(strings[match[1]], `Missing string: ${match[1]}`);
  assert.match(html, /type="password"/);
  assert.match(html, /autocomplete="off"/);
  assert.match(html, /role="status"/);
});
test('external values use safe DOM writes and credentials stay in memory', async () => {
  const app = await read('app.js');
  assert.doesNotMatch(app, /innerHTML|outerHTML|insertAdjacentHTML|localStorage|sessionStorage|console\./);
  assert.match(app, /textContent/);
  assert.match(app, /Authorization/);
  assert.match(app, /AbortController/);
});
test('all literal translation keys and catalog labels are present', async () => {
  const app = await read('app.js');
  const strings = JSON.parse(await read('strings/ko.json'));
  for (const match of app.matchAll(/\bt\('([^']+)'/g)) assert.ok(strings[match[1]], `Missing string: ${match[1]}`);
  for (const key of ['stage.start', 'stage.quest1', 'stage.quest2', 'stage.quest3a', 'stage.quest3b', 'stage.quest4', 'stage.complete', 'error.unauthorized', 'error.not_configured', 'error.invalid_filter', 'error.not_found', 'error.unavailable']) assert.ok(strings[key]);
});
