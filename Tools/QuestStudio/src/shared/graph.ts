import type { GraphProjection, QuestNode, QuestPack } from './types';
import { validatePack, ValidationError } from './validation';
export const GRAPH_CARD_WIDTH = 248;
export const GRAPH_CARD_HEIGHT = 116;
const BOUNDARY_COLUMN_GAP = 80;
const BOUNDARY_ROW_GAP = 32;
export function getChapter(node: QuestNode): string {
  return node.definition.authoring_tags.find(tag => tag.startsWith('chapter:'))?.slice(8) ?? '';
}
export function resolveQuestId(pack: QuestPack, input: string): string {
  const trimmed = input.trim();
  if (!/^\d+$/.test(trimmed)) throw new ValidationError('validation.numeric_quest_id');
  const candidate = trimmed.replace(/^0+/, '') || '0';
  const match = pack.nodes.find(node => node.definition.quest_id === candidate);
  if (!match) throw new ValidationError('validation.unknown_quest', { questId: input });
  return match.definition.quest_id;
}
export function updatePrerequisites(pack: QuestPack, targetQuestId: string, prerequisiteIds: string[]): QuestPack {
  const target = pack.nodes.find(node => node.definition.quest_id.toLowerCase() === targetQuestId.toLowerCase());
  if (!target) throw new ValidationError('validation.unknown_quest', { questId: targetQuestId });
  const updated: QuestPack = { ...pack, nodes: pack.nodes.map(node => node === target ? {
    ...node,
    definition: { ...node.definition, required_completed_quest_ids: [...prerequisiteIds] },
    authoring: { ...node.authoring, prerequisitesStatus: 'confirmed' },
  } : node) };
  return validatePack(updated);
}
export function projectChapter(pack: QuestPack, chapter: string): GraphProjection {
  const visible = pack.nodes.filter(node => getChapter(node) === chapter);
  const included = new Set(visible.map(node => node.definition.quest_id.toLowerCase()));
  const external = new Map<string, QuestNode>();
  const lookup = new Map(pack.nodes.map(node => [node.definition.quest_id.toLowerCase(), node]));
  const edges: GraphProjection['edges'] = [];
  for (const node of visible) for (const id of node.definition.required_completed_quest_ids) {
    const prerequisite = lookup.get(id.toLowerCase());
    if (!prerequisite) continue;
    const canonical = prerequisite.definition.quest_id;
    if (!included.has(canonical.toLowerCase())) external.set(canonical, prerequisite);
    edges.push({ source: canonical, target: node.definition.quest_id });
  }
  const minX = visible.reduce((minimum, node) => Math.min(minimum, node.x), Infinity);
  const minY = visible.reduce((minimum, node) => Math.min(minimum, node.y), Infinity);
  const boundaryNodes = [...external.values()].sort((a, b) =>
    a.definition.quest_id < b.definition.quest_id ? -1 : a.definition.quest_id > b.definition.quest_id ? 1 : 0);
  const card = (node: QuestNode, isExternal: boolean, x = node.x, y = node.y): GraphProjection['nodes'][number] => ({
    id: node.definition.quest_id, questId: node.definition.quest_id,
    x, y, external: isExternal, titleKey: node.definition.title_string_key,
  });
  // Boundary coordinates belong only to this projection; saved chapter layouts stay intact.
  return { nodes: [
    ...visible.map(node => card(node, false)),
    ...boundaryNodes.map((node, index) => card(node, true,
      minX - GRAPH_CARD_WIDTH - BOUNDARY_COLUMN_GAP,
      minY + index * (GRAPH_CARD_HEIGHT + BOUNDARY_ROW_GAP))),
  ], edges };
}
