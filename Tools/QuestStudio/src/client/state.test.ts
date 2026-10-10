import { describe, expect, it } from 'vitest';
import { addPrerequisite, arrangeChapter, exportDocument, importDocument, loadDocument, moveQuest, removePrerequisite, selectChapter, selectQuest } from './state';
import type { QuestPack } from '../shared/types';

// Synthetic exchange data: never copy an authoring dataset into public tests.
function fixture(): QuestPack {
  return {
    schemaVersion: 1,
    customPackRevision: 17,
    nodes: [
      { definition: { quest_id: 1, title_string_key: 'test.a', description_string_key: 'test.ad', required_completed_quest_ids: [], authoring_tags: ['chapter:1'], custom: { reward: 7 } }, x: 100, y: 100, authoring: { prerequisitesStatus: 'unspecified', customNote: 'synthetic' }, customNode: { nested: true } },
      { definition: { quest_id: 2, title_string_key: 'test.b', description_string_key: 'test.bd', required_completed_quest_ids: [], authoring_tags: ['chapter:1'] }, x: 450, y: 100, authoring: { prerequisitesStatus: 'confirmed' } },
      { definition: { quest_id: 21, title_string_key: 'test.c', description_string_key: 'test.cd', required_completed_quest_ids: [1], authoring_tags: ['chapter:2'] }, x: 450, y: 350, authoring: { prerequisitesStatus: 'confirmed' } },
    ],
    strings: [
      ...['a', 'ad', 'b', 'bd', 'c', 'cd'].map(id => ({ key: `test.${id}`, locale: 'ko', value: `합성 ${id}` })),
      { key: 'test.a', locale: 'en', value: 'Synthetic A', translatorComment: 'synthetic' },
    ],
  };
}

describe('manual editor document', () => {
  it('moves only the addressed local card and preserves coordinates through editing and full exchange', () => {
    const original = loadDocument(fixture());
    const moved = moveQuest(original, 2, -25.5, 0);
    expect(moved.dirty).toBe(true);
    expect(moved.pack!.nodes[1]).toEqual({ ...original.pack!.nodes[1], x: -25.5, y: 0 });
    expect(moved.pack!.nodes[0]).toBe(original.pack!.nodes[0]);
    expect(moved.pack!.nodes[2]).toBe(original.pack!.nodes[2]);
    expect(original.pack!.nodes[1]!.x).toBe(450);
    const linked = addPrerequisite(selectQuest(moved, 2), '1');
    const unlinked = removePrerequisite(linked, 1);
    const reloaded = importDocument(exportDocument(selectChapter(unlinked, '2')));
    expect(reloaded.pack!.nodes[1]!.x).toBe(-25.5);
    expect(reloaded.pack!.nodes[1]!.y).toBe(0);
    expect(reloaded.pack!.strings).toEqual(original.pack!.strings);
  });

  it('keeps no-op moves clean and rejects non-finite or external card moves', () => {
    const original = loadDocument(fixture());
    expect(moveQuest(original, 1, 100, 100)).toBe(original);
    expect(original.dirty).toBe(false);
    for (const coordinate of [NaN, Infinity, -Infinity]) {
      expect(() => moveQuest(original, 1, coordinate, 0)).toThrow('validation.position');
      expect(() => moveQuest(original, 1, 0, coordinate)).toThrow('validation.position');
    }
    expect(() => moveQuest(original, 21, 0, 0)).toThrow('client.externalReadOnly');
    expect(() => moveQuest(original, 999, 0, 0)).toThrow('client.noSelection');
  });

  it('arranges only the active chapter on explicit request and keeps a repeated arrangement clean', () => {
    const original = loadDocument(fixture());
    const arranged = arrangeChapter(original);
    expect(arranged.dirty).toBe(true);
    expect(arranged.pack!.nodes[2]).toBe(original.pack!.nodes[2]);
    expect(arranged.pack!.strings).toBe(original.pack!.strings);
    const saved = { ...arranged, dirty: false };
    expect(arrangeChapter(saved)).toBe(saved);
  });
  it('switches chapters without narrowing the exported full pack or its unknown fields and locales', () => {
    const imported = importDocument(JSON.stringify(fixture()), 'synthetic.json');
    const switched = selectChapter(imported, '2');
    expect(switched.chapter).toBe('2');
    expect(JSON.parse(exportDocument(switched))).toEqual(fixture());
    expect(switched.dirty).toBe(false);
  });

  it('adds a global number and removes a prerequisite without affecting hidden chapters', () => {
    const imported = selectQuest(importDocument(JSON.stringify(fixture())), 2);
    const added = addPrerequisite(imported, '01');
    expect(added.pack!.nodes[1]!.definition.required_completed_quest_ids).toEqual([1]);
    expect(added.pack!.nodes[2]!.definition.required_completed_quest_ids).toEqual([1]);
    expect(added.dirty).toBe(true);
    expect(imported.pack!.nodes[1]!.definition.required_completed_quest_ids).toEqual([]);
    const removed = removePrerequisite(added, 1);
    expect(removed.pack!.nodes[1]!.definition.required_completed_quest_ids).toEqual([]);
    expect(removed.pack!.strings.at(-1)).toEqual({ key: 'test.a', locale: 'en', value: 'Synthetic A', translatorComment: 'synthetic' });
  });

  it('rejects unknown, duplicate, self and cyclic input while preserving the original document', () => {
    const imported = selectQuest(importDocument(JSON.stringify(fixture())), 2);
    for (const input of ['99', '02']) expect(() => addPrerequisite(imported, input)).toThrow();
    const added = addPrerequisite(imported, '01');
    expect(() => addPrerequisite(added, '01')).toThrow();
    expect(() => addPrerequisite(selectQuest(added, 1), '02')).toThrow();
    expect(JSON.parse(exportDocument(imported))).toEqual(fixture());
  });

  it('loads a full snapshot and isolates its data from later edits', () => {
    const snapshotPack = fixture();
    const loaded = loadDocument(snapshotPack, 'snapshot-1');
    const edited = addPrerequisite(selectQuest(loaded, 2), '1');
    expect(snapshotPack.nodes[1]!.definition.required_completed_quest_ids).toEqual([]);
    const switched = selectChapter(edited, '2');
    expect(JSON.parse(exportDocument(switched)).nodes).toHaveLength(3);
    expect(JSON.parse(exportDocument(switched)).strings).toHaveLength(7);
    expect(loaded.origin).toBe('snapshot-1');
  });

  it('rejects malformed file input before replacing a working document', () => {
    const original = importDocument(JSON.stringify(fixture()));
    expect(() => importDocument('{broken')).toThrow();
    expect(() => importDocument('{}')).toThrow();
    expect(JSON.parse(exportDocument(original))).toEqual(fixture());
  });

  it('prevents editing external boundary nodes until their own chapter is selected', () => {
    const switched = selectChapter(importDocument(JSON.stringify(fixture())), '2');
    const boundarySelected = selectQuest(switched, 1);
    expect(() => addPrerequisite(boundarySelected, '2')).toThrow();
    expect(() => removePrerequisite(boundarySelected, 2)).toThrow();
    expect(JSON.parse(exportDocument(boundarySelected))).toEqual(fixture());
  });
});
