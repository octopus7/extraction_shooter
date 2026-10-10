import type { QuestNode, QuestPack } from '../shared/types';
import { getChapter, resolveQuestId, updatePrerequisites } from '../shared/graph';
import { validatePack } from '../shared/validation';
import { ClientError } from './api';

export interface EditorDocument {
  pack: QuestPack | null;
  chapter: string;
  selectedQuestId: string | null;
  dirty: boolean;
  origin: string | null;
}

export const emptyDocument = (): EditorDocument => ({ pack: null, chapter: '', selectedQuestId: null, dirty: false, origin: null });
export function chapters(pack: QuestPack | null): string[] {
  return pack ? [...new Set(pack.nodes.map(getChapter))].sort((a, b) => a.localeCompare(b, undefined, { numeric: true })) : [];
}
export function loadDocument(value: unknown, origin: string | null = null): EditorDocument {
  const pack = validatePack(structuredClone(value));
  const chapter = chapters(pack)[0] ?? '';
  return { pack, chapter, selectedQuestId: pack.nodes.find(node => getChapter(node) === chapter)?.definition.quest_id ?? null, dirty: false, origin };
}
export function importDocument(text: string, origin: string | null = null): EditorDocument {
  let value: unknown;
  try { value = JSON.parse(text); } catch { throw new ClientError('client.invalidJson'); }
  return loadDocument(value, origin);
}
export function exportDocument(document: EditorDocument): string {
  if (!document.pack) throw new ClientError('client.noDocument');
  return JSON.stringify(validatePack(document.pack), null, 2);
}
export function selectChapter(document: EditorDocument, chapter: string): EditorDocument {
  return { ...document, chapter, selectedQuestId: document.pack?.nodes.find(node => getChapter(node) === chapter)?.definition.quest_id ?? null };
}
export function selectQuest(document: EditorDocument, questId: string): EditorDocument {
  return { ...document, selectedQuestId: questId };
}
export function selectedNode(document: EditorDocument): QuestNode | undefined {
  return document.pack?.nodes.find(node => node.definition.quest_id === document.selectedQuestId);
}
function editableNode(document: EditorDocument): QuestNode {
  const node = selectedNode(document);
  if (!document.pack || !node) throw new ClientError('client.noSelection');
  if (getChapter(node) !== document.chapter) throw new ClientError('client.externalReadOnly');
  return node;
}
export function addPrerequisite(document: EditorDocument, input: string): EditorDocument {
  const node = editableNode(document);
  const id = resolveQuestId(document.pack!, input.trim(), document.chapter);
  return { ...document, pack: updatePrerequisites(document.pack!, node.definition.quest_id, [...node.definition.required_completed_quest_ids, id]), dirty: true };
}
export function removePrerequisite(document: EditorDocument, id: string): EditorDocument {
  const node = editableNode(document);
  return { ...document, pack: updatePrerequisites(document.pack!, node.definition.quest_id, node.definition.required_completed_quest_ids.filter(value => value !== id)), dirty: true };
}
export function questText(pack: QuestPack | null, key: string): string {
  return pack?.strings.find(entry => entry.key === key && (entry.locale === 'ko' || entry.locale === 'ko-KR'))?.value ?? key;
}
