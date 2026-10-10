import type { QuestPack } from './types';
export class ValidationError extends Error {
  constructor(readonly key: string, readonly params?: Record<string, string | number>) { super(key); }
}
export function isRecord(value: unknown): value is Record<string, unknown> {
  return typeof value === 'object' && value !== null && !Array.isArray(value);
}
const nonempty = (value: unknown): value is string => typeof value === 'string' && value.trim().length > 0;
const stringList = (value: unknown): value is string[] => Array.isArray(value) && value.every(nonempty);
function fail(key: string, params?: Record<string, string | number>): never { throw new ValidationError(key, params); }

export function validatePack(value: unknown): QuestPack {
  if (!isRecord(value) || value.schemaVersion !== 1 || !Array.isArray(value.nodes) || !Array.isArray(value.strings)) fail('validation.pack');
  const ids = new Map<number, number>();
  for (const item of value.nodes) {
    if (!isRecord(item) || !isRecord(item.definition) || !isRecord(item.authoring)) fail('validation.node');
    const d = item.definition;
    if (!nonempty(d.title_string_key) || !nonempty(d.description_string_key) ||
        !Array.isArray(d.required_completed_quest_ids) || !stringList(d.authoring_tags) ||
        (item.authoring.prerequisitesStatus !== 'confirmed' && item.authoring.prerequisitesStatus !== 'unspecified') ||
        (item.authoring.sourceReference !== undefined && typeof item.authoring.sourceReference !== 'string')) fail('validation.node');
    if (!isQuestId(d.quest_id) || !d.required_completed_quest_ids.every(isQuestId)) {
      fail('validation.numeric_quest_id');
    }
    const chapters = d.authoring_tags.filter(tag => tag.startsWith('chapter:'));
    if (chapters.length !== 1 || !/^chapter:[1-9]\d*$/.test(chapters[0]!)) fail('validation.chapter', { questId: d.quest_id });
    if (typeof item.x !== 'number' || typeof item.y !== 'number' || !Number.isFinite(item.x) || !Number.isFinite(item.y)) fail('validation.position', { questId: d.quest_id });
    const folded = d.quest_id;
    if (ids.has(folded)) fail('validation.duplicate_quest', { questId: d.quest_id });
    ids.set(folded, d.quest_id);
  }
  const ko = new Set<string>(); const tuples = new Set<string>();
  for (const entry of value.strings) {
    if (!isRecord(entry) || !nonempty(entry.key) || !nonempty(entry.locale) || typeof entry.value !== 'string') fail('validation.string');
    const tuple = JSON.stringify([entry.key, entry.locale]);
    if (tuples.has(tuple)) fail('validation.duplicate_string', { stringKey: entry.key, locale: entry.locale });
    tuples.add(tuple);
    if (entry.locale === 'ko' && entry.value.trim()) ko.add(entry.key);
  }
  // Traverse every JSON object, including unknown definition and authoring fields.
  const pending: unknown[] = [value]; const visited = new Set<object>();
  while (pending.length) {
    const item = pending.pop();
    if (typeof item !== 'object' || item === null || visited.has(item)) continue;
    visited.add(item);
    if (Array.isArray(item)) { for (const child of item) pending.push(child); continue; }
    for (const [field, fieldValue] of Object.entries(item)) {
      if (field.endsWith('_string_key')) {
        if (!nonempty(fieldValue) || !ko.has(fieldValue)) fail('validation.missing_string', { stringKey: typeof fieldValue === 'string' ? fieldValue : field });
      } else if (field.endsWith('_string_keys')) {
        if (!stringList(fieldValue)) fail('validation.string');
        for (const ref of fieldValue) if (!ko.has(ref)) fail('validation.missing_string', { stringKey: ref });
      }
      pending.push(fieldValue);
    }
  }
  const pack = value as QuestPack;
  const dependents = new Map<number, number[]>(); const degree = new Map<number, number>();
  for (const node of pack.nodes) {
    const id = node.definition.quest_id; const seen = new Set<number>();
    for (const prerequisite of node.definition.required_completed_quest_ids) {
      const ref = prerequisite;
      if (!ids.has(ref)) fail('validation.unknown_quest', { questId: prerequisite });
      if (ref === id) fail('validation.self_reference', { questId: node.definition.quest_id });
      if (seen.has(ref)) fail('validation.duplicate_prerequisite', { questId: prerequisite });
      seen.add(ref);
      const children = dependents.get(ref) ?? []; children.push(id); dependents.set(ref, children);
    }
    degree.set(id, seen.size);
  }
  const available = [...degree.keys()].filter(id => degree.get(id) === 0); let count = 0;
  while (available.length) {
    const id = available.pop()!; count++;
    for (const next of dependents.get(id) ?? []) {
      const remaining = degree.get(next)! - 1; degree.set(next, remaining);
      if (remaining === 0) available.push(next);
    }
  }
  if (count !== pack.nodes.length) fail('validation.cycle');
  return pack;
}

export function isQuestId(value: unknown): value is number {
  return typeof value === 'number' && Number.isInteger(value) && value > 0 && value <= 2147483647;
}
/** Read compatibility only: normalize old schemaVersion 1 packs without mutating storage or input. */
export function normalizeLoadedPack(value: unknown): QuestPack {
  const copy: unknown = structuredClone(value);
  const legacyId = (id: unknown): unknown => typeof id === 'string' && /^[1-9]\d*$/.test(id) ? Number(id) : id;
  if (isRecord(copy) && Array.isArray(copy.nodes)) for (const node of copy.nodes) {
    if (!isRecord(node) || !isRecord(node.definition)) continue;
    node.definition.quest_id = legacyId(node.definition.quest_id);
    if (Array.isArray(node.definition.required_completed_quest_ids)) node.definition.required_completed_quest_ids = node.definition.required_completed_quest_ids.map(legacyId);
  }
  return validatePack(copy);
}
