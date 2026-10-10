import { expect, it } from 'vitest';
import { validatePack, ValidationError } from '../src/shared/validation';
import { importDocument, exportDocument } from '../src/client/state';
import { formatQuestId, resolveQuestId } from '../src/shared/graph';
const pack = (id: unknown = 3, refs: unknown[] = []) => ({ schemaVersion: 1, custom: { untouched: true }, nodes: [
  { definition: { quest_id: id, required_completed_quest_ids: refs, title_string_key: 't', description_string_key: 't', authoring_tags: ['chapter:1'], custom: { reward: 5 } }, x: 12.5, y: -4, authoring: { prerequisitesStatus: 'confirmed', detail: 'preserved' } },
], strings: [{ key: 't', locale: 'ko', value: '합성' }] });
it('accepts positive int32 IDs and resolves padded user input to integers', () => {
  expect(validatePack(pack()).nodes[0]!.definition.quest_id).toBe(3);
  expect(resolveQuestId(validatePack(pack()), '003')).toBe(3);
  expect(validatePack(pack(2147483647)).nodes[0]!.definition.quest_id).toBe(2147483647);
});
it('rejects noninteger and out-of-range canonical identities and references', () => {
  for (const id of [0, -1, 1.5, 2147483648, Infinity, NaN, '3', '003', 'text', null, true]) {
    expect(() => validatePack(pack(id))).toThrow(ValidationError);
    expect(() => validatePack(pack(3, [id]))).toThrow(ValidationError);
  }
});
it('normalizes legacy canonical numeric strings only on explicit import and preserves all other data', () => {
  const legacy = pack('3');
  expect(JSON.parse(exportDocument(importDocument(JSON.stringify(legacy))))).toEqual(pack());
  expect(legacy.nodes[0]!.definition.quest_id).toBe('3');
  for (const id of ['003', '0', '1.5', '2147483648', 'legacy']) expect(() => importDocument(JSON.stringify(pack(id)))).toThrow(ValidationError);
});
it('rejects duplicate identities and references after legacy normalization', () => {
  const duplicate = pack('3'); duplicate.nodes.push(pack(3).nodes[0]!);
  expect(() => importDocument(JSON.stringify(duplicate))).toThrow('validation.duplicate_quest');
  const refs = pack(4, [3, '3']); refs.nodes.push(pack('3').nodes[0]!);
  expect(() => importDocument(JSON.stringify(refs))).toThrow('validation.duplicate_prerequisite');
});

it('pads display IDs to at least three digits without changing larger identities', () => {
  expect(formatQuestId(3)).toBe('003');
  expect(formatQuestId(123)).toBe('123');
  expect(formatQuestId(1000)).toBe('1000');
  expect(resolveQuestId(validatePack(pack(123)), '123')).toBe(123);
  expect(resolveQuestId(validatePack(pack(1000)), '01000')).toBe(1000);
});

it('normalizes legacy prerequisite references while retaining coordinates and unknown metadata', () => {
  const legacy = pack('4', ['3']); legacy.nodes.push(pack('3').nodes[0]!);
  const result = JSON.parse(exportDocument(importDocument(JSON.stringify(legacy))));
  expect(result.nodes.map((node: { definition: { quest_id: number; required_completed_quest_ids: number[] } }) => [node.definition.quest_id, node.definition.required_completed_quest_ids])).toEqual([[4, [3]], [3, []]]);
  expect(result.nodes[0].x).toBe(12.5);
  expect(result.nodes[0].authoring.detail).toBe('preserved');
  expect(result.strings).toEqual(legacy.strings);
});
