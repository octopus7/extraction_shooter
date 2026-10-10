<script lang="ts">
  import { onMount } from 'svelte';
  import { t } from '../shared/ui-strings';
  import type { QuestPack } from '../shared/types';
  import { GRAPH_CARD_HEIGHT, GRAPH_CARD_WIDTH, projectChapter } from '../shared/graph';
  import { questText } from './state';

  let { pack, chapter, selectedId, onselect }: { pack: QuestPack; chapter: string; selectedId: string | null; onselect: (id: string) => void } = $props();
  const projection = $derived(projectChapter(pack, chapter));
  let viewport: HTMLDivElement;
  let scale = $state(1);
  let offset = $state({ x: 40, y: 40 });
  let dragging = $state(false);
  let pointer: { x: number; y: number; originX: number; originY: number; id: number } | null = null;
  const cardWidth = GRAPH_CARD_WIDTH;
  const cardHeight = GRAPH_CARD_HEIGHT;
  const positioned = $derived(new Map(projection.nodes.map(node => [node.id, node])));

  function fit() {
    if (!viewport || !projection.nodes.length) return;
    const minX = Math.min(...projection.nodes.map(node => node.x));
    const minY = Math.min(...projection.nodes.map(node => node.y));
    const width = Math.max(...projection.nodes.map(node => node.x)) + cardWidth - minX;
    const height = Math.max(...projection.nodes.map(node => node.y)) + cardHeight - minY;
    const next = Math.min(1, Math.max(0.2, Math.min((viewport.clientWidth - 100) / width, (viewport.clientHeight - 100) / height)));
    scale = next;
    offset = { x: (viewport.clientWidth - width * next) / 2 - minX * next, y: (viewport.clientHeight - height * next) / 2 - minY * next };
  }
  function zoom(factor: number, x = viewport.clientWidth / 2, y = viewport.clientHeight / 2) {
    const next = Math.min(2, Math.max(0.2, scale * factor));
    offset = { x: x - (x - offset.x) * next / scale, y: y - (y - offset.y) * next / scale };
    scale = next;
  }
  function start(event: PointerEvent) {
    if (event.button !== 0 || (event.target as HTMLElement).closest('button')) return;
    pointer = { x: event.clientX, y: event.clientY, originX: offset.x, originY: offset.y, id: event.pointerId };
    viewport.setPointerCapture(event.pointerId);
    dragging = true;
  }
  function move(event: PointerEvent) {
    if (pointer) offset = { x: pointer.originX + event.clientX - pointer.x, y: pointer.originY + event.clientY - pointer.y };
  }
  function end() { pointer = null; dragging = false; }
  function wheel(event: WheelEvent) {
    event.preventDefault();
    const rect = viewport.getBoundingClientRect();
    zoom(Math.exp(-event.deltaY * 0.0015), event.clientX - rect.left, event.clientY - rect.top);
  }
  function path(source: string, target: string): string {
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

<div class="graph-frame">
  <div class:dragging class="graph-viewport" bind:this={viewport} role="region" aria-label={t('graph.label')} onpointerdown={start} onpointermove={move} onpointerup={end} onpointercancel={end} onlostpointercapture={end} onwheel={wheel}>
    <div class="graph-plane" style:transform={`translate(${offset.x}px, ${offset.y}px) scale(${scale})`}>
      <svg class="edges" aria-hidden="true">
        <defs><marker id="arrow" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M 0 0 L 10 5 L 0 10 z" /></marker></defs>
        {#each projection.edges as edge (`${edge.source}:${edge.target}`)}
          <path class:active={edge.source === selectedId || edge.target === selectedId} d={path(edge.source, edge.target)} marker-end="url(#arrow)" />
        {/each}
      </svg>
      {#each projection.nodes as node (node.id)}
        {@const definition = pack.nodes.find(item => item.definition.quest_id === node.questId)}
        {@const noPrerequisites = !node.external && node.questId !== '1' && definition?.definition.required_completed_quest_ids.length === 0}
        <button class="quest-card" class:selected={selectedId === node.questId} class:external={node.external} class:no-prerequisites={noPrerequisites} style:width={`${cardWidth}px`} style:height={`${cardHeight}px`} style:left={`${node.x}px`} style:top={`${node.y}px`} onclick={() => onselect(node.questId)} aria-label={t('graph.selectNode', { id: node.questId })} aria-pressed={selectedId === node.questId}>
          <span class="card-top"><span class="quest-id">{node.questId}</span><span class="node-dot"></span></span>
          <strong>{questText(pack, node.titleKey)}</strong>
          {#if node.external}<span class="card-bottom">{t('graph.legendExternal')}</span>{:else if noPrerequisites}<span class="card-bottom">{t('graph.noPrerequisites')}</span>{/if}
        </button>
      {/each}
    </div>
    {#if !projection.nodes.length}<p class="graph-no-nodes">{t('graph.noNodes')}</p>{/if}
  </div>
  <div class="graph-legend"><span><i class="legend-dot local"></i>{t('graph.legendLocal')}</span><span><i class="legend-dot external"></i>{t('graph.legendExternal')}</span><span><i class="legend-dot no-prerequisites"></i>{t('graph.noPrerequisites')}</span></div>
  <div class="graph-bottom"><span class="graph-hint">{t('graph.hint')}</span><div class="zoom-controls"><button aria-label={t('graph.zoomOut')} title={t('graph.zoomOut')} onclick={() => zoom(1 / 1.2)}>−</button><span>{t('graph.zoom', { percent: Math.round(scale * 100) })}</span><button aria-label={t('graph.zoomIn')} title={t('graph.zoomIn')} onclick={() => zoom(1.2)}>+</button><button class="fit-button" onclick={fit}>{t('graph.fit')}</button></div></div>
</div>
