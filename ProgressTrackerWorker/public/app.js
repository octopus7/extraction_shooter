const $ = (id) => document.getElementById(id);
let strings = {};
let token = '';
let buildId = '';
let page = 1;
let requestVersion = 0;
let controller;
let detailVersion = 0;
let detailController;
const knownLabels = new Map([
  ['game.start', 'stage.start'], ['quest.demo_q1_water_intake_check', 'stage.quest1'],
  ['quest.demo_q2_clear_water_screen', 'stage.quest2'], ['quest.demo_q3a_repair_valve', 'stage.quest3a'],
  ['quest.demo_q3b_repair_bunker_pipe', 'stage.quest3b'], ['quest.demo_q4_todays_reward', 'stage.quest4'],
  ['demo.complete', 'stage.complete'],
]);
export function t(key, values = {}) {
  return (strings[key] ?? key).replace(/\{(\w+)\}/g, (_, name) => String(values[name] ?? ''));
}
const number = (value) => new Intl.NumberFormat('ko-KR').format(value ?? 0);
const percent = (value) => new Intl.NumberFormat('ko-KR', { style: 'percent', maximumFractionDigits: 1 }).format(value ?? 0);
function duration(value) {
  if (value == null) return t('format.none');
  const seconds = Math.max(0, Math.floor(value));
  if (seconds < 60) return t('format.seconds', { value: seconds });
  if (seconds < 3600) return t('format.minutes', { minutes: Math.floor(seconds / 60), seconds: seconds % 60 });
  return t('format.hours', { hours: Math.floor(seconds / 3600), minutes: Math.floor(seconds % 3600 / 60) });
}
function date(value) {
  const parsed = new Date(value);
  return Number.isNaN(parsed.getTime()) ? t('format.none') : new Intl.DateTimeFormat('ko-KR', { dateStyle: 'short', timeStyle: 'short' }).format(parsed);
}
function el(tag, text, className) {
  const node = document.createElement(tag);
  if (text != null) node.textContent = String(text);
  if (className) node.className = className;
  return node;
}
function cell(row, text, className) { const node = el('td', text, className); row.append(node); return node; }
function stageName(stage) { return stage.registered && strings[stage.labelKey] ? t(stage.labelKey) : stage.id; }
function checkpointName(id) { return knownLabels.has(id) ? t(knownLabels.get(id)) : id; }
function setStatus(key, error = false) { $('status').textContent = t(key); $('status').classList.toggle('error', error); }
function closeDetail() {
  ++detailVersion;
  detailController?.abort();
  $('detail').close();
  $('detail-body').replaceChildren();
}
function clearData() {
  $('dashboard').hidden = true;
  $('stages').replaceChildren();
  $('runs').replaceChildren();
  closeDetail();
}
function disconnect(key = 'status.signedout', error = false) {
  ++requestVersion;
  controller?.abort();
  token = '';
  $('token').value = '';
  buildId = '';
  page = 1;
  $('build').replaceChildren();
  clearData();
  $('login').hidden = false;
  $('signout').hidden = true;
  $('connect').disabled = false;
  setStatus(key, error);
}
const errors = new Set(['unauthorized', 'not_configured', 'invalid_filter', 'not_found', 'unavailable']);
function errorKey(error) { return `error.${errors.has(error.message) ? error.message : 'unavailable'}`; }
async function api(path, signal) {
  const response = await fetch(path, { headers: { Authorization: `Bearer ${token}` }, cache: 'no-store', credentials: 'omit', signal });
  const data = await response.json();
  if (!response.ok) throw new Error(errors.has(data.error) ? data.error : 'unavailable');
  return data;
}
function query(extra = {}) {
  const params = new URLSearchParams(extra);
  if (buildId) params.set('buildId', buildId);
  return params.toString();
}
function renderSummary(data) {
  $('build').replaceChildren(el('option', t('filter.all')));
  $('build').firstChild.value = '';
  for (const build of data.builds) { const option = el('option', build); option.value = build; $('build').append(option); }
  $('build').value = buildId;
  for (const [id, key] of [['total-players', 'players'], ['total-runs', 'runs'], ['total-completed', 'completedPlayers'], ['total-missing', 'missingRuns']]) $(id).textContent = number(data.totals[key]);
  $('completed-rate').textContent = t('metric.completed.rate', { rate: percent(data.totals.players ? data.totals.completedPlayers / data.totals.players : 0) });
  $('start-count').textContent = t('stages.started', { count: number(data.totals.startedPlayers) });
  for (const stage of data.stages) {
    const row = el('tr');
    const title = cell(row);
    title.append(el('strong', stageName(stage)), el('span', stage.id, 'stage-id'));
    if (!stage.registered) title.append(el('span', t('stage.unregistered'), 'unknown'));
    cell(row, number(stage.players), 'numeric');
    const rate = cell(row, null, 'rate');
    rate.append(el('span', percent(stage.rate / 100), 'rate-label'));
    const track = el('div', null, 'track');
    track.setAttribute('aria-hidden', 'true');
    const fill = el('div', null, 'fill');
    fill.style.width = `${Math.max(0, Math.min(100, stage.rate))}%`;
    track.append(fill); rate.append(track);
    cell(row, number(stage.runs), 'numeric');
    cell(row, duration(stage.averageSeconds), 'numeric');
    cell(row, number(stage.missingRuns), stage.missingRuns ? 'numeric missing' : 'numeric');
    $('stages').append(row);
  }
}
function renderRuns(data) {
  $('run-count').textContent = t('runs.count', { count: number(data.total) });
  if (!data.items.length) { const row = el('tr'); const empty = cell(row, t('status.empty'), 'empty'); empty.colSpan = 7; $('runs').append(row); }
  for (const run of data.items) {
    const row = el('tr');
    for (const value of [run.playerId, run.runId]) { const node = cell(row, value, 'id'); node.title = value; }
    cell(row, run.buildId);
    cell(row, date(run.lastReceivedAt));
    cell(row, number(run.checkpointCount), 'numeric');
    const state = cell(row);
    state.append(el('div', t(run.completed ? 'runs.complete' : 'runs.observed'), run.completed ? 'good' : ''));
    if (run.missingCount) state.append(el('div', t('runs.missing', { count: number(run.missingCount) }), 'missing'));
    const button = el('button', t('action.detail'), 'detail-button');
    button.setAttribute('aria-label', t('action.detailFor', { id: run.runId }));
    button.addEventListener('click', () => showDetail(run));
    cell(row).append(button);
    $('runs').append(row);
  }
  page = data.page;
  const pages = Math.max(1, Math.ceil(data.total / data.pageSize));
  $('page-label').textContent = t('pagination.page', { page, pages });
  $('previous').disabled = page <= 1;
  $('next').disabled = page >= pages;
}
async function load() {
  const version = ++requestVersion;
  controller?.abort();
  controller = new AbortController();
  clearData();
  setStatus('status.loading');
  $('connect').disabled = true;
  try {
    const [summary, runs] = await Promise.all([
      api(`/api/admin/summary?${query()}`, controller.signal),
      api(`/api/admin/runs?${query({ page: String(page) })}`, controller.signal),
    ]);
    if (version !== requestVersion) return;
    renderSummary(summary); renderRuns(runs);
    $('login').hidden = true;
    $('dashboard').hidden = false;
    $('signout').hidden = false;
    setStatus(summary.totals.runs ? 'status.loaded' : 'status.empty');
  } catch (error) {
    if (version !== requestVersion || error.name === 'AbortError') return;
    disconnect(errorKey(error), true);
  } finally { if (version === requestVersion) $('connect').disabled = false; }
}
async function showDetail(run) {
  closeDetail();
  const version = ++detailVersion;
  detailController = new AbortController();
  $('detail-body').append(el('p', t('status.loading'), 'muted'));
  $('detail').showModal();
  try {
    const params = new URLSearchParams({ playerId: run.playerId, runId: run.runId, buildId: run.buildId, dataset: run.dataset });
    const data = await api(`/api/admin/run?${params}`, detailController.signal);
    if (version !== detailVersion) return;
    const body = $('detail-body'); body.replaceChildren();
    body.append(el('p', t('detail.identity', { player: data.identity.playerId, run: data.identity.runId, build: data.identity.buildId }), 'identity'));
    body.append(el('h3', t('detail.observed'), 'detail-heading'));
    for (const stage of data.checkpoints) {
      const item = el('div', null, 'checkpoint');
      item.append(el('strong', stageName(stage)), el('span', stage.id, 'stage-id'));
      if (!stage.registered) item.append(el('span', t('stage.unregistered'), 'unknown'));
      item.append(el('p', t('detail.time', { time: duration(stage.playtimeSeconds) })), el('p', t('detail.received', { time: date(stage.receivedAt) }), 'muted'));
      body.append(item);
    }
    body.append(el('h3', t('detail.missing'), 'detail-heading'));
    if (!data.missing.length) body.append(el('p', t('detail.none'), 'muted'));
    for (const missing of data.missing) {
      const item = el('div', null, 'missing-item');
      item.append(el('strong', checkpointName(missing.checkpointId), 'missing'));
      item.append(el('p', t('detail.required', { stages: missing.requiredBy.map(checkpointName).join(' · ') })));
      body.append(item);
    }
  } catch (error) {
    if (version !== detailVersion || error.name === 'AbortError') return;
    if (error.message === 'unauthorized' || error.message === 'not_configured') disconnect(errorKey(error), true);
    else $('detail-body').replaceChildren(el('p', t(errorKey(error)), 'missing'));
  }
}
async function start() {
  const response = await fetch('/strings/ko.json', { cache: 'no-cache' });
  if (!response.ok) throw new Error();
  strings = await response.json();
  document.title = t('page.title');
  for (const node of document.querySelectorAll('[data-i18n]')) node.textContent = t(node.dataset.i18n);
  for (const node of document.querySelectorAll('[data-i18n-placeholder]')) node.placeholder = t(node.dataset.i18nPlaceholder);
  for (const node of document.querySelectorAll('[data-i18n-label]')) node.setAttribute('aria-label', t(node.dataset.i18nLabel));
  $('login-form').addEventListener('submit', (event) => { event.preventDefault(); token = $('token').value.trim(); $('token').value = ''; if (token) load(); });
  $('signout').addEventListener('click', () => disconnect());
  $('refresh').addEventListener('click', load);
  $('build').addEventListener('change', () => { buildId = $('build').value; page = 1; load(); });
  $('previous').addEventListener('click', () => { --page; load(); });
  $('next').addEventListener('click', () => { ++page; load(); });
  $('close-detail').addEventListener('click', closeDetail);
  $('detail').addEventListener('cancel', (event) => { event.preventDefault(); closeDetail(); });
  window.addEventListener('pagehide', () => disconnect());
  fetch('/api/health', { cache: 'no-store' }).then((result) => result.json()).then((data) => { $('preview-note').hidden = data.preview !== true; }).catch(() => {});
}
start().catch(() => { $('connect').disabled = true; });
