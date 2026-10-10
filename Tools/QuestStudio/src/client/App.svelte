<script lang="ts">
  import { onMount } from 'svelte';
  import type { QuestPack, SnapshotMetadata } from '../shared/types';
  import { getChapter } from '../shared/graph';
  import { formatQuestId } from '../shared/graph';
  import { t } from '../shared/ui-strings';
  import { apiRequest, errorMessage } from './api';
  import { addPrerequisite, arrangeChapter, moveQuest, chapters, emptyDocument, exportDocument, importDocument, loadDocument, questText, removePrerequisite, selectChapter, selectedNode, selectQuest } from './state';
  import Graph from './Graph.svelte';

  let document = $state(emptyDocument());
  let snapEnabled = $state(true);
  let authenticated = $state(false);
  let authConfigured = $state(true);
  let checking = $state(true);
  let localMode = $state(false);
  let password = $state('');
  let busy = $state(false);
  let error = $state('');
  let notice = $state('');
  let dialog = $state<'save' | 'load' | 'login' | null>(null);
  let pendingReplacement = $state<(() => Promise<void> | void) | null>(null);
  let alias = $state('');
  let memo = $state('');
  let snapshots = $state<SnapshotMetadata[]>([]);
  let listLoaded = $state(false);
  let prerequisite = $state('');
  let fileInput: HTMLInputElement;
  const node = $derived(selectedNode(document));
  const external = $derived(node ? getChapter(node) !== document.chapter : false);
  const chapterList = $derived(chapters(document.pack));
  const visibleCount = $derived(document.pack?.nodes.filter(item => getChapter(item) === document.chapter).length ?? 0);

  async function run(task: () => Promise<void> | void) {
    busy = true; error = ''; notice = '';
    try { await task(); } catch (failure) {
      error = errorMessage(failure);
      if (failure && typeof failure === 'object' && 'key' in failure && failure.key === 'api.unauthorized') authenticated = false;
    } finally { busy = false; }
  }
  onMount(() => {
    window.document.title = t('app.title');
    void run(async () => {
      const session = await apiRequest<{ authenticated: boolean; authConfigured: boolean }>('/api/session');
      authenticated = session.authenticated; authConfigured = session.authConfigured;
    }).finally(() => checking = false);
  });
  function askReplace(task: () => Promise<void> | void) {
    if (document.dirty) pendingReplacement = task;
    else void run(task);
  }
  async function importFile(event: Event) {
    const file = (event.target as HTMLInputElement).files?.[0];
    fileInput.value = '';
    if (!file) return;
    // Parse and validate first; failed input cannot displace current work.
    await run(async () => {
      const imported = importDocument(await file.text(), file.name);
      askReplace(() => {
        document = imported; prerequisite = '';
        notice = t('file.imported', { count: imported.pack!.nodes.length });
      });
    });
  }
  function exportFile() {
    void run(() => {
      const blob = new Blob([exportDocument(document)], { type: 'application/json;charset=utf-8' });
      const url = URL.createObjectURL(blob);
      const anchor = window.document.createElement('a');
      anchor.href = url; anchor.download = t('file.exportName'); anchor.click();
      setTimeout(() => URL.revokeObjectURL(url), 0);
      notice = t('file.exported');
    });
  }
  function changeChapter(chapter: string) { document = selectChapter(document, chapter); prerequisite = ''; error = ''; }
  function changeSelection(id: number) { document = selectQuest(document, id); prerequisite = ''; error = ''; }
  function moveNode(id: number, x: number, y: number) {
    void run(() => { document = moveQuest(document, id, x, y); });
  }
  function arrangeNodes() {
    void run(() => { document = arrangeChapter(document); notice = t('graph.arranged'); });
  }
  function addLink(event: SubmitEvent) {
    event.preventDefault();
    void run(() => { document = addPrerequisite(document, prerequisite); prerequisite = ''; notice = t('inspector.updated'); });
  }
  function removeLink(id: number) {
    void run(() => { document = removePrerequisite(document, id); notice = t('inspector.updated'); });
  }
  async function login(event: SubmitEvent) {
    event.preventDefault();
    await run(async () => {
      await apiRequest('/api/login', { password });
      password = ''; authenticated = true; localMode = true; dialog = null;
    });
  }
  async function logout() {
    await run(async () => { await apiRequest('/api/logout', {}); authenticated = false; localMode = true; snapshots = []; });
  }
  function openSave() { alias = ''; memo = ''; dialog = 'save'; error = ''; }
  function openLoad() { dialog = 'load'; listLoaded = false; void refreshSnapshots(); }
  async function refreshSnapshots() {
    await run(async () => {
      snapshots = (await apiRequest<{ snapshots: SnapshotMetadata[] }>('/api/snapshots')).snapshots;
      listLoaded = true;
    });
  }
  async function saveSnapshot(event: SubmitEvent) {
    event.preventDefault();
    await run(async () => {
      const result = await apiRequest<{ snapshot: SnapshotMetadata }>('/api/snapshots', { alias: alias.trim(), memo: memo.trim(), pack: document.pack });
      document = { ...document, dirty: false, origin: result.snapshot.alias };
      dialog = null; notice = t('snapshot.saved');
    });
  }
  function loadSnapshot(id: string) {
    askReplace(async () => {
      const result = await apiRequest<{ snapshot: SnapshotMetadata; pack: QuestPack }>(`/api/snapshots/${encodeURIComponent(id)}`);
      const next = loadDocument(result.pack, result.snapshot.alias);
      document = next; prerequisite = ''; dialog = null; notice = t('snapshot.loaded');
    });
  }
  function acceptReplacement() {
    const task = pendingReplacement; pendingReplacement = null;
    if (task) void run(task);
  }
  function dateLabel(value: string) {
    return new Intl.DateTimeFormat('ko-KR', { dateStyle: 'medium', timeStyle: 'short' }).format(new Date(value));
  }
  function detailKeys(value: unknown): string[] {
    return typeof value === 'string' ? [value] : Array.isArray(value) ? value.filter((item): item is string => typeof item === 'string') : [];
  }
  function modalFocus(element: HTMLDivElement) {
    const previous = window.document.activeElement as HTMLElement | null;
    const selectors = 'button:not(:disabled), input:not(:disabled), textarea:not(:disabled), [tabindex="0"]';
    requestAnimationFrame(() => (element.querySelector<HTMLElement>('input:not(:disabled)') ?? element).focus());
    function trap(event: KeyboardEvent) {
      if (event.key !== 'Tab') return;
      const targets = [...element.querySelectorAll<HTMLElement>(selectors)];
      const first = targets[0], last = targets.at(-1);
      if (event.shiftKey && (window.document.activeElement === first || window.document.activeElement === element)) { event.preventDefault(); last?.focus(); }
      else if (!event.shiftKey && window.document.activeElement === last) { event.preventDefault(); first?.focus(); }
    }
    element.addEventListener('keydown', trap);
    return { destroy() { element.removeEventListener('keydown', trap); previous?.focus(); } };
  }
</script>

<svelte:window onkeydown={(event) => { if (event.key === 'Escape' && !busy) { dialog = null; pendingReplacement = null; } }} />
<input class="hidden" type="file" accept=".json,application/json" bind:this={fileInput} onchange={importFile} aria-label={t('file.choose')} />

<div class="app-shell">
  <header class="app-header">
    <div class="brand"><span class="brand-mark" aria-hidden="true"><i></i><i></i><i></i></span><div><h1>{t('app.title')}</h1><p>{t('app.subtitle')}</p></div></div>
    <div class="header-right"><span class="local-badge"><span></span>{t('app.localFirst')}</span>{#if authenticated}<button class="quiet" onclick={logout} disabled={busy}>{t('auth.logout')}</button>{:else if localMode}<button class="quiet" onclick={() => dialog = 'login'} disabled={busy || !authConfigured}>{t('auth.connect')}</button>{/if}</div>
  </header>

  {#if error}<div class="banner error" role="alert"><span><strong>{t('common.error')}</strong>{error}</span><button class="icon-button" aria-label={t('common.dismiss')} onclick={() => error = ''}>×</button></div>{/if}
  {#if notice}<div class="banner notice" role="status"><span>{notice}</span><button class="icon-button" aria-label={t('common.dismiss')} onclick={() => notice = ''}>×</button></div>{/if}

  {#if checking}
    <main class="welcome"><div class="loading-dot"></div><p>{t('app.loading')}</p></main>
  {:else if !authenticated && !localMode}
    <main class="welcome"><section class="login-card"><div class="eyebrow">{t('app.workspace')}</div><h2>{t('auth.heading')}</h2><p>{t('auth.description')}</p>
      {#if authConfigured}<form onsubmit={login}><label for="initial-password">{t('auth.password')}</label><input id="initial-password" type="password" autocomplete="current-password" bind:value={password} required disabled={busy} /><button class="primary wide" type="submit" disabled={busy}>{busy ? t('common.working') : t('auth.login')}</button></form>{:else}<p class="warning-copy">{t('auth.unconfigured')}</p>{/if}
      <div class="login-divider"></div><button class="secondary wide" onclick={() => { localMode = true; error = ''; }}>{t('auth.localMode')}</button><p class="microcopy">{t('auth.localHint')}</p>
    </section></main>
  {:else}
    <div class="workspace-toolbar"><div class="document-identity"><strong>{document.origin ?? t('document.untitled')}</strong><span class:dirty={document.dirty}><i></i>{document.dirty ? t('document.changed') : t('document.clean')}</span></div><div class="toolbar-actions"><button class="secondary" onclick={() => fileInput.click()} disabled={busy}>{t('file.import')}</button><button class="secondary" onclick={exportFile} disabled={!document.pack || busy}>{t('file.export')}</button><span class="toolbar-divider"></span><button class="secondary" onclick={openLoad} disabled={!authenticated || busy}>{t('snapshot.load')}</button><button class="primary" onclick={openSave} disabled={!authenticated || !document.pack || busy}>{t('snapshot.save')}</button></div></div>

    {#if document.pack}
      <main class="editor-layout"><section class="canvas-section"><div class="canvas-heading"><div class="chapter-control"><label for="chapter-select">{t('chapter.label')}</label><select id="chapter-select" value={document.chapter} onchange={(event) => changeChapter(event.currentTarget.value)} disabled={busy}>{#each chapterList as chapter}<option value={chapter}>{t('chapter.name', { chapter })}</option>{/each}</select><span>{t('chapter.count', { count: visibleCount })}</span></div><span class="total-count">{t('document.total', { nodes: document.pack.nodes.length, strings: document.pack.strings.length })}</span></div>
        {#key `${document.chapter}:${document.origin}`}<Graph pack={document.pack} chapter={document.chapter} selectedId={document.selectedQuestId} onselect={changeSelection} onmove={moveNode} onarrange={arrangeNodes} disabled={busy} bind:snapEnabled />{/key}
      </section>
      <aside class="inspector"><div class="inspector-heading"><span class="eyebrow">{t('inspector.heading')}</span>{#if node}<span class="inspector-id">{formatQuestId(node.definition.quest_id)}</span>{/if}</div>
        {#if node}<div class="inspector-body"><h2>{questText(document.pack, node.definition.title_string_key)}</h2><span class="status-pill" class:unspecified={node.authoring.prerequisitesStatus === 'unspecified'}>{external ? t('graph.legendExternal') : node.authoring.prerequisitesStatus === 'unspecified' ? t('inspector.unspecified') : t('inspector.confirmed')}</span>
          <section class="detail-section"><h3>{t('inspector.description')}</h3><p class="quest-description">{questText(document.pack, node.definition.description_string_key)}</p>{#each detailKeys(node.authoring.detail_string_key) as key}<p class="quest-description">{questText(document.pack, key)}</p>{/each}{#each detailKeys(node.authoring.context_string_keys) as key}<p class="quest-description secondary-copy">{questText(document.pack, key)}</p>{/each}</section>
          <section class="detail-section"><div class="section-heading"><h3>{t('inspector.prerequisites')}</h3><span class="count-badge">{node.definition.required_completed_quest_ids.length}</span></div>
            {#if node.authoring.prerequisitesStatus === 'unspecified'}<p class="inline-note">{t('inspector.unspecifiedHelp')}</p>{/if}
            {#if node.definition.required_completed_quest_ids.length}<ul class="prerequisite-list">{#each node.definition.required_completed_quest_ids as id}<li><span class="prerequisite-id">{formatQuestId(id)}</span><span class="prerequisite-title">{questText(document.pack, document.pack.nodes.find(item => item.definition.quest_id === id)?.definition.title_string_key ?? formatQuestId(id))}</span>{#if !external}<button class="remove-button" onclick={() => removeLink(id)} disabled={busy} aria-label={t('inspector.remove', { id: formatQuestId(id) })} title={t('inspector.remove', { id: formatQuestId(id) })}>×</button>{/if}</li>{/each}</ul>{:else}<p class="empty-prerequisites">{t('inspector.noPrerequisites')}</p>{/if}
            {#each detailKeys(node.authoring.prerequisites_evidence_string_key) as key}<p class="quest-description secondary-copy">{questText(document.pack, key)}</p>{/each}
            {#if external}<p class="inline-note">{t('inspector.externalHelp')}</p>{:else}<form class="prerequisite-form" onsubmit={addLink}><label for="prerequisite-input">{t('inspector.numberLabel')}</label><div class="input-row"><input id="prerequisite-input" bind:value={prerequisite} placeholder={t('inspector.numberPlaceholder')} autocomplete="off" required disabled={busy} /><button class="secondary" type="submit" disabled={busy || !prerequisite.trim()}>{t('inspector.add')}</button></div><p class="microcopy">{t('inspector.numberHelp')}</p></form>{/if}
          </section>
          {#if node.authoring.sourceReference}<section class="detail-section"><h3>{t('inspector.source')}</h3><p class="source-reference">{node.authoring.sourceReference}</p></section>{/if}
          <details class="definition-details"><summary>{t('inspector.definition')}</summary><pre>{JSON.stringify(node.definition, null, 2)}</pre></details>
        </div>{:else}<p class="inspector-placeholder">{t('inspector.empty')}</p>{/if}
      </aside></main>
    {:else}
      <main class="empty-workspace"><div class="empty-graph" aria-hidden="true"><span></span><i></i><span></span><i></i><span></span></div><h2>{t('document.emptyTitle')}</h2><p>{t('document.emptyDescription')}</p><div class="empty-actions"><button class="primary" onclick={() => fileInput.click()} disabled={busy}>{t('file.import')}</button>{#if authenticated}<button class="secondary" onclick={openLoad} disabled={busy}>{t('snapshot.load')}</button>{/if}</div><span class="microcopy">{t('document.emptyHint')}</span></main>
    {/if}
  {/if}
</div>

{#if dialog || pendingReplacement}
  <div class="modal-backdrop"><div class="modal" class:snapshot-modal={dialog === 'load' && !pendingReplacement} role="dialog" aria-modal="true" aria-labelledby="modal-title" tabindex="-1" use:modalFocus>
    <div class="modal-top"><span class="eyebrow">{t('app.title')}</span><button class="icon-button" aria-label={t('common.close')} disabled={busy} onclick={() => { dialog = null; pendingReplacement = null; }}>×</button></div>
    {#if pendingReplacement}<h2 id="modal-title">{t('document.replaceTitle')}</h2><p>{t('document.replaceDescription')}</p><div class="modal-actions"><button class="secondary" onclick={() => pendingReplacement = null}>{t('common.cancel')}</button><button class="primary" onclick={acceptReplacement}>{t('document.replace')}</button></div>
    {:else if dialog === 'save'}<h2 id="modal-title">{t('snapshot.saveTitle')}</h2><p>{t('snapshot.saveDescription')}</p><form onsubmit={saveSnapshot}><label for="snapshot-alias">{t('snapshot.alias')}</label><input id="snapshot-alias" bind:value={alias} placeholder={t('snapshot.aliasPlaceholder')} required maxlength="120" disabled={busy} /><label for="snapshot-memo">{t('snapshot.memo')}</label><textarea id="snapshot-memo" bind:value={memo} placeholder={t('snapshot.memoPlaceholder')} rows="3" maxlength="2000" disabled={busy}></textarea><div class="modal-actions"><button class="secondary" type="button" onclick={() => dialog = null} disabled={busy}>{t('common.cancel')}</button><button class="primary" type="submit" disabled={busy || !alias.trim()}>{busy ? t('common.working') : t('snapshot.save')}</button></div></form>
    {:else if dialog === 'load'}<h2 id="modal-title">{t('snapshot.loadTitle')}</h2><p>{t('snapshot.loadDescription')}</p><div class="snapshot-list">{#each snapshots as snapshot (snapshot.id)}<button class="snapshot-row" onclick={() => loadSnapshot(snapshot.id)} disabled={busy} aria-label={t('snapshot.pick', { alias: snapshot.alias })}><span class="snapshot-row-top"><strong>{snapshot.alias}</strong><time datetime={snapshot.createdAt}>{dateLabel(snapshot.createdAt)}</time></span>{#if snapshot.memo}<span class="snapshot-memo">{snapshot.memo}</span>{/if}<span class="snapshot-count">{t('snapshot.count', { nodes: snapshot.nodeCount, strings: snapshot.stringCount })}</span></button>{:else}<p class="list-empty">{busy ? t('common.working') : listLoaded ? t('snapshot.empty') : ''}</p>{/each}</div><div class="modal-actions"><button class="secondary" onclick={refreshSnapshots} disabled={busy}>{t('snapshot.refresh')}</button><button class="secondary" onclick={() => dialog = null} disabled={busy}>{t('common.close')}</button></div>
    {:else if dialog === 'login'}<h2 id="modal-title">{t('auth.heading')}</h2><p>{t('auth.description')}</p><form onsubmit={login}><label for="modal-password">{t('auth.password')}</label><input id="modal-password" type="password" autocomplete="current-password" bind:value={password} required disabled={busy} /><div class="modal-actions"><button class="secondary" type="button" onclick={() => dialog = null} disabled={busy}>{t('common.cancel')}</button><button class="primary" type="submit" disabled={busy}>{busy ? t('common.working') : t('auth.login')}</button></div></form>{/if}
    {#if error}<p class="modal-error" role="alert">{error}</p>{/if}
  </div></div>
{/if}
