<script lang="ts">
  import { onMount, tick } from 'svelte';
  import { t } from '../shared/ui-strings';
  import type { QuestPack } from '../shared/types';
  import { formatQuestId, GRAPH_CARD_HEIGHT, GRAPH_CARD_WIDTH, projectChapter } from '../shared/graph';
  import { snapPosition } from './snap';
  import { questText } from './state';

  let { pack, chapter, selectedId, onselect, onmove, onconnect, onarrange, disabled = false, snapEnabled = $bindable(true) }: { pack: QuestPack; chapter: string; selectedId: number | null; onselect: (id: number) => void; onmove: (id: number, x: number, y: number) => void; onconnect: (dependentId: number, prerequisiteId: number) => void; onarrange: () => void; disabled?: boolean; snapEnabled?: boolean } = $props();
  const projection = $derived(projectChapter(pack, chapter));
  let viewport: HTMLDivElement;
  let scale = $state(1);
  let offset = $state({ x: 40, y: 40 });
  let dragging = $state(false);
  let pointer: { x: number; y: number; originX: number; originY: number; id: number } | null = null;
  let nodeDrag = $state<{ id: number; pointerId: number; clientX: number; clientY: number; originX: number; originY: number; x: number; y: number; scale: number; moved: boolean } | null>(null);
  let linkDrag = $state<{ id: number; pointerId: number; clientX: number; clientY: number; moved: boolean; x: number; y: number; target: number | null } | null>(null);
  let guides = $state<{ guideX: number | null; guideY: number | null }>({ guideX: null, guideY: null });
  const renderedNodes = $derived(projection.nodes.map(node => nodeDrag?.id === node.id ? { ...node, x: nodeDrag.x, y: nodeDrag.y } : node));
  const cardWidth = GRAPH_CARD_WIDTH;
  const cardHeight = GRAPH_CARD_HEIGHT;
  const positioned = $derived(new Map(renderedNodes.map(node => [node.id, node])));

  function fit() {
    if (linkDrag || nodeDrag || !viewport || !projection.nodes.length) return;
    const minX = Math.min(...projection.nodes.map(node => node.x));
    const minY = Math.min(...projection.nodes.map(node => node.y));
    const width = Math.max(...projection.nodes.map(node => node.x)) + cardWidth - minX;
    const height = Math.max(...projection.nodes.map(node => node.y)) + cardHeight - minY;
    const next = Math.min(1, Math.max(0.2, Math.min((viewport.clientWidth - 100) / width, (viewport.clientHeight - 100) / height)));
    scale = next;
    offset = { x: (viewport.clientWidth - width * next) / 2 - minX * next, y: (viewport.clientHeight - height * next) / 2 - minY * next };
  }
  function zoom(factor: number, x = viewport.clientWidth / 2, y = viewport.clientHeight / 2) {
    if (nodeDrag || linkDrag) return;
    const next = Math.min(2, Math.max(0.2, scale * factor));
    offset = { x: x - (x - offset.x) * next / scale, y: y - (y - offset.y) * next / scale };
    scale = next;
  }
  function start(event: PointerEvent) {
    if (pointer || nodeDrag || linkDrag || event.button !== 0 || (event.target as HTMLElement).closest('button')) return;
    pointer = { x: event.clientX, y: event.clientY, originX: offset.x, originY: offset.y, id: event.pointerId };
    viewport.setPointerCapture(event.pointerId);
    dragging = true;
  }
  function startNode(event: PointerEvent, id: number, x: number, y: number, external: boolean) {
    event.stopPropagation();
    if (event.button !== 0 || pointer || nodeDrag || linkDrag) return;
    onselect(id);
    (event.currentTarget as HTMLButtonElement).focus();
    if (external || disabled) return;
    nodeDrag = { id, pointerId: event.pointerId, clientX: event.clientX, clientY: event.clientY, originX: x, originY: y, x, y, scale, moved: false };
    viewport.setPointerCapture(event.pointerId);
  }
  function cardAt(event: PointerEvent): number | null {
    const element = window.document.elementFromPoint(event.clientX, event.clientY)?.closest<HTMLElement>('.quest-card, .prerequisite-port');
    return element && viewport.contains(element) ? Number(element.dataset.questId) : null;
  }
  function startLink(event: PointerEvent, id: number, x: number, y: number) {
    event.stopPropagation();
    if (event.button !== 0 || disabled || pointer || nodeDrag || linkDrag) return;
    onselect(id);
    (event.currentTarget as HTMLButtonElement).focus();
    linkDrag = { id, pointerId: event.pointerId, clientX: event.clientX, clientY: event.clientY, moved: false, x, y: y + cardHeight / 2, target: null };
    viewport.setPointerCapture(event.pointerId);
  }
  function move(event: PointerEvent) {
    if (linkDrag?.pointerId === event.pointerId) {
      const rect = viewport.getBoundingClientRect();
      linkDrag = { ...linkDrag, moved: linkDrag.moved || Math.hypot(event.clientX - linkDrag.clientX, event.clientY - linkDrag.clientY) >= 3, x: (event.clientX - rect.left - offset.x) / scale, y: (event.clientY - rect.top - offset.y) / scale, target: cardAt(event) };
      return;
    }
    if (nodeDrag?.pointerId === event.pointerId) {
      const dx = event.clientX - nodeDrag.clientX, dy = event.clientY - nodeDrag.clientY;
      if (nodeDrag.moved || Math.hypot(dx, dy) >= 3) {
        const x = Math.round(nodeDrag.originX + dx / nodeDrag.scale), y = Math.round(nodeDrag.originY + dy / nodeDrag.scale);
        const aligned = snapEnabled ? snapPosition(nodeDrag.id, x, y, nodeDrag.scale, projection.nodes) : { x, y, guideX: null, guideY: null };
        guides = aligned;
        nodeDrag = { ...nodeDrag, moved: true, x: aligned.x, y: aligned.y };
      }
    } else if (pointer?.id === event.pointerId) offset = { x: pointer.originX + event.clientX - pointer.x, y: pointer.originY + event.clientY - pointer.y };
  }
  function end(event?: PointerEvent, commit = false) {
    const id = linkDrag?.pointerId ?? nodeDrag?.pointerId ?? pointer?.id;
    if (event && event.pointerId !== id) return;
    const connection = linkDrag;
    const target = commit && event && connection ? cardAt(event) : null;
    linkDrag = null;
    const finished = nodeDrag;
    guides = { guideX: null, guideY: null };
    nodeDrag = null; pointer = null; dragging = false;
    if (id !== undefined && viewport.hasPointerCapture(id)) viewport.releasePointerCapture(id);
    if (connection?.moved && target !== null && !disabled) onconnect(connection.id, target);
    if (commit && finished?.moved && !disabled) onmove(finished.id, finished.x, finished.y);
  }
  async function arrange() { onarrange(); await tick(); fit(); }
  function wheel(event: WheelEvent) {
    event.preventDefault();
    const rect = viewport.getBoundingClientRect();
    zoom(Math.exp(-event.deltaY * 0.0015), event.clientX - rect.left, event.clientY - rect.top);
  }
  function path(source: number, target: number): string {
    const a = positioned.get(source);
    const b = positioned.get(target);
    if (!a || !b) return '';
    const x1 = a.x + cardWidth;
    const y1 = a.y + cardHeight / 2;
    const x2 = b.x;
    const y2 = b.y + cardHeight / 2;
    const bend = Math.max(60, Math.abs(x2 - x1) / 2);
    return `M ${x1} ${y1} C ${x1 + bend} ${y1}, ${x2 - bend} ${y2}, ${x2} ${y2}`;
  }
  onMount(fit);
</script>

<svelte:window onkeydown={(event) => { if (event.key === 'Escape') end(); }} />

<div class="graph-frame">
  <div class:dragging class="graph-viewport" bind:this={viewport} role="region" aria-label={t('graph.label')} onpointerdown={start} onpointermove={move} onpointerup={(event) => end(event, true)} onpointercancel={(event) => end(event)} onlostpointercapture={(event) => end(event)} onwheel={wheel}>
    <div class="graph-plane" style:transform={`translate(${offset.x}px, ${offset.y}px) scale(${scale})`}>
      <svg class="edges" aria-hidden="true">
        <defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M 0 0 L 10 5 L 0 10 z" /></marker></defs>
        {#each projection.edges as edge (`${edge.source}:${edge.target}`)}
          <path class:active={edge.source === selectedId || edge.target === selectedId} d={path(edge.source, edge.target)} marker-end="url(#arrow)" />
        {/each}
        {#if guides.guideX !== null}<line class="snap-guide" x1={guides.guideX} x2={guides.guideX} y1={Math.min(...renderedNodes.map(node => node.y)) - 40} y2={Math.max(...renderedNodes.map(node => node.y)) + cardHeight + 40} />{/if}
        {#if guides.guideY !== null}<line class="snap-guide" y1={guides.guideY} y2={guides.guideY} x1={Math.min(...renderedNodes.map(node => node.x)) - 40} x2={Math.max(...renderedNodes.map(node => node.x)) + cardWidth + 40} />{/if}
        {#if linkDrag}
          {@const origin = positioned.get(linkDrag.id)}
          {#if origin}<line class="link-preview" x1={origin.x} y1={origin.y + cardHeight / 2} x2={linkDrag.x} y2={linkDrag.y} />{/if}
        {/if}
      </svg>
      {#each renderedNodes as node (node.id)}
        {@const definition = pack.nodes.find(item => item.definition.quest_id === node.questId)}
        {@const noPrerequisites = !node.external && node.questId !== 1 && definition?.definition.required_completed_quest_ids.length === 0}
        <button data-quest-id={node.questId} class:link-target={linkDrag?.target === node.questId} class="quest-card" class:moving={nodeDrag?.id === node.id && nodeDrag.moved} class:selected={selectedId === node.questId} class:external={node.external} class:no-prerequisites={noPrerequisites} style:width={`${cardWidth}px`} style:height={`${cardHeight}px`} style:left={`${node.x}px`} style:top={`${node.y}px`} onpointerdown={(event) => startNode(event, node.id, node.x, node.y, node.external)} title={node.external ? t('graph.legendExternal') : t('graph.moveHint')} onclick={() => onselect(node.questId)} aria-label={t('graph.selectNode', { id: formatQuestId(node.questId) })} aria-pressed={selectedId === node.questId}>
          <span class="card-top"><span class="quest-id">{formatQuestId(node.questId)}</span><span class="node-dot"></span></span>
          <strong>{questText(pack, node.titleKey)}</strong>
          {#if node.external}<span class="card-bottom">{t('graph.legendExternal')}</span>{:else if noPrerequisites}<span class="card-bottom">{t('graph.noPrerequisites')}</span>{/if}
        </button>
        {#if !node.external && definition?.definition.required_completed_quest_ids.length === 0}
          <button data-quest-id={node.questId} class="prerequisite-port" style:left={(node.x - 11) + 'px'} style:top={(node.y + cardHeight / 2 - 11) + 'px'} disabled={disabled} aria-label={t('graph.linkHandle', { id: formatQuestId(node.questId) })} title={t('graph.linkHelp')} onpointerdown={(event) => startLink(event, node.questId, node.x, node.y)} onclick={() => onselect(node.questId)}><span aria-hidden="true"></span></button>
        {/if}
      {/each}
    </div>
    {#if !projection.nodes.length}<p class="graph-no-nodes">{t('graph.noNodes')}</p>{/if}
  </div>
  <div class="graph-legend"><span><i class="legend-dot local"></i>{t('graph.legendLocal')}</span><span><i class="legend-dot external"></i>{t('graph.legendExternal')}</span><span><i class="legend-dot no-prerequisites"></i>{t('graph.noPrerequisites')}</span></div>
  <div class="graph-bottom"><span class="graph-hint">{t('graph.hint')}</span><div class="zoom-controls"><label class="snap-toggle" title={t('graph.snapHelp')}><input type="checkbox" bind:checked={snapEnabled} disabled={!!nodeDrag || !!linkDrag} />{t('graph.snapMode')}</label><button class="fit-button" disabled={disabled || !!nodeDrag || !!linkDrag} onclick={arrange}>{t('graph.arrange')}</button><button aria-label={t('graph.zoomOut')} title={t('graph.zoomOut')} onclick={() => zoom(1 / 1.2)}>−</button><span>{t('graph.zoom', { percent: Math.round(scale * 100) })}</span><button aria-label={t('graph.zoomIn')} title={t('graph.zoomIn')} onclick={() => zoom(1.2)}>+</button><button class="fit-button" onclick={fit}>{t('graph.fit')}</button></div></div>
</div>
