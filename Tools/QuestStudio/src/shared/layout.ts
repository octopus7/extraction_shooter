import { getChapter, GRAPH_CARD_HEIGHT, GRAPH_CARD_WIDTH } from './graph';
import type { QuestPack } from './types';
import { validatePack } from './validation';

const compareIds = (a: string, b: string): number => a.length - b.length || (a < b ? -1 : a > b ? 1 : 0);

/** Explicit editor action only. Coordinates never change prerequisites or runtime data. */
export function arrangeChapterNodes(pack: QuestPack, chapter: string): QuestPack {
  validatePack(pack);
  const local = pack.nodes.filter(node => getChapter(node) === chapter);
  if (!local.length) return pack;
  const lookup = new Map(local.map(node => [node.definition.quest_id, node]));
  const ids = [...lookup.keys()].sort(compareIds);
  const parents = new Map(ids.map(id => [id, lookup.get(id)!.definition.required_completed_quest_ids.filter(parent => lookup.has(parent)).sort(compareIds)]));
  const depth = new Map<string, number>();
  const column = (id: string): number => {
    if (!depth.has(id)) depth.set(id, Math.max(-1, ...parents.get(id)!.map(column)) + 1);
    return depth.get(id)!;
  };
  for (const id of ids) column(id);

  // A deterministic spanning forest keeps each terminal branch in a contiguous band.
  // Merged nodes follow their deepest parent, while all DAG edges still point right.
  const children = new Map(ids.map(id => [id, [] as string[]]));
  const roots: string[] = [];
  for (const id of ids) {
    const candidates = [...parents.get(id)!].sort((a, b) => depth.get(b)! - depth.get(a)! || compareIds(a, b));
    if (candidates.length) children.get(candidates[0]!)!.push(id);
    else roots.push(id);
  }
  const rows = new Map<string, number>();
  let nextRow = 0;
  const place = (id: string): number => {
    const branch = children.get(id)!;
    let row: number;
    if (!branch.length) row = nextRow++;
    else {
      const childRows = branch.map(place);
      row = (childRows[0]! + childRows[childRows.length - 1]!) / 2;
    }
    rows.set(id, row);
    return row;
  };
  for (const id of roots) { place(id); nextRow++; }
  let changed = false;
  const nodes = pack.nodes.map(node => {
    const id = node.definition.quest_id;
    if (!lookup.has(id)) return node;
    const x = 80 + depth.get(id)! * (GRAPH_CARD_WIDTH + 100);
    const y = 80 + rows.get(id)! * (GRAPH_CARD_HEIGHT + 80);
    if (node.x === x && node.y === y) return node;
    changed = true;
    return { ...node, x, y };
  });
  return changed ? { ...pack, nodes } : pack;
}
