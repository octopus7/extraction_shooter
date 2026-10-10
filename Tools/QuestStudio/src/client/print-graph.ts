import { formatQuestId, projectChapter } from '../shared/graph';
import { cardSize, edgeCurve } from '../shared/graph-geometry';
import type { QuestPack } from '../shared/types';
import { t } from '../shared/ui-strings';
import { questText } from './state';

// Render the complete chapter from model coordinates, independently of viewport zoom/pan.
export async function renderPrintGraph(pack: QuestPack, chapter: string): Promise<Blob> {
  const graph = projectChapter(pack, chapter);
  if (!graph.nodes.length) throw new Error(t('graph.noNodes'));
  await document.fonts.ready;
  const lookup = new Map(graph.nodes.map(node => [node.id, node]));
  const curves = graph.edges.map(edge => edgeCurve(lookup.get(edge.source)!, lookup.get(edge.target)!));
  const xs = graph.nodes.flatMap(node => [node.x, node.x + cardSize(node).width]);
  const ys = graph.nodes.flatMap(node => [node.y, node.y + cardSize(node).height]);
  // Include Bezier control points so backward/manual connections cannot be clipped.
  for (const curve of curves) xs.push(curve.c1, curve.c2);
  const left = Math.min(...xs) - 32, top = Math.min(...ys) - 32;
  const width = Math.max(...xs) - left + 32, height = Math.max(...ys) - top + 32;
  const scale = Math.min(3, 16000 / width, 16000 / height, Math.sqrt(64000000 / (width * height)));
  const canvas = document.createElement('canvas');
  canvas.width = Math.ceil(width * scale); canvas.height = Math.ceil(height * scale);
  const ctx = canvas.getContext('2d');
  if (!ctx) throw new Error(t('graph.printFailed'));
  ctx.fillStyle = '#fff'; ctx.fillRect(0, 0, canvas.width, canvas.height);
  ctx.scale(scale, scale); ctx.translate(-left, -top);
  ctx.strokeStyle = '#000'; ctx.fillStyle = '#000'; ctx.lineWidth = 1;
  for (const c of curves) {
    ctx.beginPath(); ctx.moveTo(c.x1, c.y1);
    ctx.bezierCurveTo(c.c1, c.y1, c.c2, c.y2, c.x2, c.y2); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(c.x2, c.y2); ctx.lineTo(c.x2 - 8, c.y2 - 4);
    ctx.lineTo(c.x2 - 8, c.y2 + 4); ctx.closePath(); ctx.fill();
  }
  for (const node of graph.nodes) {
    const { width: w, height: h } = cardSize(node);
    ctx.fillStyle = '#fff'; ctx.beginPath(); ctx.roundRect(node.x, node.y, w, h, 7); ctx.fill(); ctx.stroke();
    ctx.save(); ctx.beginPath(); ctx.rect(node.x + 12, node.y + 8, w - 24, h - 16); ctx.clip();
    ctx.fillStyle = '#000'; ctx.textBaseline = 'top';
    ctx.font = '600 11px "Malgun Gothic", sans-serif';
    ctx.fillText(formatQuestId(node.questId), node.x + 16, node.y + 13);
    ctx.font = `600 ${node.external ? 12 : 14}px "Malgun Gothic", sans-serif`;
    const lines: string[] = []; let line = '';
    for (const character of questText(pack, node.titleKey)) {
      if (character === '\n' || (line && ctx.measureText(line + character).width > w - 32)) {
        lines.push(line); line = character === '\n' ? '' : character;
      } else line += character;
    }
    lines.push(line);
    // Fit long localized titles within the card without dropping any text.
    const available = h - (node.external ? 65 : 48);
    const step = Math.min(19, available / lines.length);
    ctx.save(); ctx.translate(node.x + 16, node.y + 36); ctx.scale(1, Math.min(1, step / 19));
    lines.forEach((text, i) => ctx.fillText(text, 0, i * 19)); ctx.restore();
    if (node.external) {
      ctx.font = '10px "Malgun Gothic", sans-serif';
      ctx.fillText(t('graph.legendExternal'), node.x + 16, node.y + h - 20);
    }
    ctx.restore();
  }
  return new Promise((resolve, reject) => canvas.toBlob(blob => {
    canvas.width = canvas.height = 0;
    if (blob) resolve(blob); else reject(new Error(t('graph.printFailed')));
  }, 'image/png'));
}

export async function downloadPrintGraph(pack: QuestPack, chapter: string) {
  const blob = await renderPrintGraph(pack, chapter);
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = t('graph.printFilename', { chapter: chapter.replace(/[<>:"/\\|?*\x00-\x1f]/g, '_') });
  link.click();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
