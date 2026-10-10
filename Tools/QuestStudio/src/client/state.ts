import type { QuestNode, QuestPack, SnapshotMetadata, SourceFileMetadata } from '../shared/types';
import { getChapter, resolveQuestId, updatePrerequisites } from '../shared/graph';
import { normalizeLoadedPack, validatePack, ValidationError } from '../shared/validation';
import { arrangeChapterNodes } from '../shared/layout';
import { ClientError } from './api';
import { CARD_COLORS } from './card-colors';

export interface EditorDocument {
  pack: QuestPack | null;
  chapter: string;
  selectedQuestId: number | null;
  dirty: boolean;
  origin: string | null;
  loadedSnapshot: SnapshotMetadata | null;
  sourceFile: SourceFileMetadata | null;
}

export const emptyDocument = (): EditorDocument => ({ pack: null, chapter: '', selectedQuestId: null, dirty: false, origin: null, loadedSnapshot: null, sourceFile: null });
export function chapters(pack: QuestPack | null): string[] {
  return pack ? [...new Set(pack.nodes.map(getChapter))].sort((a, b) => a.localeCompare(b, undefined, { numeric: true })) : [];
}
export function loadDocument(value: unknown, origin: string | null = null, snapshot?: SnapshotMetadata): EditorDocument {
  const pack = normalizeLoadedPack(value);
  const chapter = chapters(pack)[0] ?? '';
  return { pack, chapter, selectedQuestId: pack.nodes.find(node => getChapter(node) === chapter)?.definition.quest_id ?? null, dirty: false, origin, loadedSnapshot: snapshot ? structuredClone(snapshot) : null, sourceFile: snapshot?.sourceFile ? { ...snapshot.sourceFile } : null };
}
export function importDocument(text: string, origin: string | null = null, sourceFile: SourceFileMetadata | null = null): EditorDocument {
  let value: unknown;
  try { value = JSON.parse(text); } catch { throw new ClientError('client.invalidJson'); }
  return { ...loadDocument(value, origin), sourceFile: sourceFile ? { ...sourceFile } : null };
}
export function exportDocument(document: EditorDocument): string {
  if (!document.pack) throw new ClientError('client.noDocument');
  return JSON.stringify(validatePack(document.pack), null, 2);
}
export function selectChapter(document: EditorDocument, chapter: string): EditorDocument {
  return { ...document, chapter, selectedQuestId: document.pack?.nodes.find(node => getChapter(node) === chapter)?.definition.quest_id ?? null };
}
export function selectQuest(document: EditorDocument, questId: number): EditorDocument {
  return { ...document, selectedQuestId: questId };
}
export function selectedNode(document: EditorDocument): QuestNode | undefined {
  return document.pack?.nodes.find(node => node.definition.quest_id === document.selectedQuestId);
}
export function moveQuest(document: EditorDocument, questId: number, x: number, y: number): EditorDocument {
  const node = document.pack?.nodes.find(candidate => candidate.definition.quest_id === questId);
  if (!document.pack || !node) throw new ClientError('client.noSelection');
  if (getChapter(node) !== document.chapter) throw new ClientError('client.externalReadOnly');
  if (!Number.isFinite(x) || !Number.isFinite(y)) throw new ValidationError('validation.position', { questId });
  if (node.x === x && node.y === y) return document;
  return { ...document, dirty: true, pack: { ...document.pack, nodes: document.pack.nodes.map(candidate => candidate === node ? { ...node, x, y } : candidate) } };
}
export function arrangeChapter(document: EditorDocument): EditorDocument {
  if (!document.pack) throw new ClientError('client.noDocument');
  const pack = arrangeChapterNodes(document.pack, document.chapter);
  return pack === document.pack ? document : { ...document, pack, dirty: true };
}
function editableNode(document: EditorDocument): QuestNode {
  const node = selectedNode(document);
  if (!document.pack || !node) throw new ClientError('client.noSelection');
  if (getChapter(node) !== document.chapter) throw new ClientError('client.externalReadOnly');
  return node;
}
export function setCardColor(document: EditorDocument, color: string): EditorDocument {
  const node = editableNode(document);
  if (!CARD_COLORS.some(preset => preset.id === color)) throw new ValidationError('validation.cardColor');
  const next = color === 'default' ? undefined : color;
  if (node.authoring.cardColor === next) return document;
  const authoring = { ...node.authoring };
  if (next === undefined) delete authoring.cardColor;
  else authoring.cardColor = next;
  return { ...document, dirty: true, pack: { ...document.pack!, nodes: document.pack!.nodes.map(candidate => candidate === node ? { ...node, authoring } : candidate) } };
}
export function addPrerequisite(document: EditorDocument, input: string): EditorDocument {
  const node = editableNode(document);
  const id = resolveQuestId(document.pack!, input.trim());
  return { ...document, pack: updatePrerequisites(document.pack!, node.definition.quest_id, [...node.definition.required_completed_quest_ids, id]), dirty: true };
}
export function removePrerequisite(document: EditorDocument, id: number): EditorDocument {
  const node = editableNode(document);
  return { ...document, pack: updatePrerequisites(document.pack!, node.definition.quest_id, node.definition.required_completed_quest_ids.filter(value => value !== id)), dirty: true };
}
export function questText(pack: QuestPack | null, key: string): string {
  return pack?.strings.find(entry => entry.key === key && (entry.locale === 'ko' || entry.locale === 'ko-KR'))?.value ?? key;
}

/** Handle owner is the dependent; dropped card is its prerequisite. */
export function connectPrerequisite(document: EditorDocument, dependentId: number, prerequisiteId: number): EditorDocument {
  return addPrerequisite(selectQuest(document, dependentId), String(prerequisiteId));
}

export function markSnapshotSaved(document: EditorDocument, snapshot: SnapshotMetadata): EditorDocument {
  return { ...document, dirty: false, origin: snapshot.alias, loadedSnapshot: structuredClone(snapshot), sourceFile: snapshot.sourceFile ? { ...snapshot.sourceFile } : null };
}
export function detachSnapshot(document: EditorDocument, id: string): EditorDocument {
  return document.loadedSnapshot?.id === id ? { ...document, loadedSnapshot: null, dirty: true } : document;
}
