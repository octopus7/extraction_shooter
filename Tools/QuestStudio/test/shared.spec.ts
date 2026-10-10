import { describe, expect, it } from 'vitest';
import { validatePack, ValidationError } from '../src/shared/validation';
import { projectChapter, resolveQuestId, updatePrerequisites } from '../src/shared/graph';
import type { QuestPack } from '../src/shared/types';

export function fixture(): QuestPack {
  return {
    schemaVersion: 1,
    nodes: [
      { definition: { quest_id: '1', title_string_key: 'q.1.title', description_string_key: 'q.1.desc', required_completed_quest_ids: [], authoring_tags: ['chapter:1'], unknown: { reward: 7 } }, x: 1, y: 2, authoring: { prerequisitesStatus: 'unspecified', sourceReference: 'synthetic' } },
      { definition: { quest_id: '2', title_string_key: 'q.2.title', description_string_key: 'q.2.desc', required_completed_quest_ids: ['21'], authoring_tags: ['chapter:1'] }, x: 3, y: 4, authoring: { prerequisitesStatus: 'confirmed' } },
      { definition: { quest_id: '21', title_string_key: 'q.3.title', description_string_key: 'q.3.desc', required_completed_quest_ids: ['57'], authoring_tags: ['chapter:2'] }, x: 5, y: 6, authoring: { prerequisitesStatus: 'confirmed', custom: { preserved: true } } },
      { definition: { quest_id: '57', title_string_key: 'q.4.title', description_string_key: 'q.4.desc', required_completed_quest_ids: [], authoring_tags: ['chapter:3'] }, x: 7, y: 8, authoring: { prerequisitesStatus: 'confirmed' } },
    ],
    strings: [1, 2, 3, 4].flatMap(n => [
      { key: `q.${n}.title`, locale: 'ko', value: `합성 ${n}` },
      { key: `q.${n}.desc`, locale: 'ko', value: `합성 설명 ${n}` },
      { key: `q.${n}.title`, locale: 'en', value: `Synthetic ${n}` },
    ]),
  };
}

function rejects(mutator: (pack: QuestPack) => void, key: string) {
  const pack = fixture(); mutator(pack);
  try { validatePack(pack); throw new Error('accepted invalid pack'); }
  catch (error) { expect(error).toBeInstanceOf(ValidationError); expect((error as ValidationError).key).toBe(key); }
}

describe('pack validation', () => {
  it('preserves unknown definition metadata and existing translations', () => {
    const pack = fixture(); expect(validatePack(pack)).toEqual(pack);
  });
  it('requires one chapter tag and finite coordinates', () => {
    rejects(p => { p.nodes[0]!.definition.authoring_tags.push('chapter:2'); }, 'validation.chapter');
    rejects(p => { p.nodes[0]!.x = Infinity; }, 'validation.position');
  });
  it('requires canonical global numeric IDs and numeric prerequisite references', () => {
    for (const id of ['R1-01', '01', '0', '-1', '1.5', 'legacy_intro']) {
      rejects(p => { p.nodes[0]!.definition.quest_id = id; }, 'validation.numeric_quest_id');
    }
    rejects(p => { p.nodes[1]!.definition.required_completed_quest_ids = ['R2-01']; }, 'validation.numeric_quest_id');
  });
  it('rejects duplicate IDs and duplicate localization tuples', () => {
    rejects(p => { p.nodes[1]!.definition.quest_id = '1'; }, 'validation.duplicate_quest');
    rejects(p => { p.strings.push({ ...p.strings[0]! }); }, 'validation.duplicate_string');
  });
  it('requires Korean entries for every nested string reference', () => {
    rejects(p => { p.nodes[0]!.definition.extra = { message_string_key: 'missing' }; }, 'validation.missing_string');
    rejects(p => { p.strings = p.strings.filter(s => s.key !== 'q.1.title' || s.locale !== 'ko'); }, 'validation.missing_string');
    rejects(p => { p.nodes[0]!.authoring.context_string_keys = ['q.1.title', 'missing']; }, 'validation.missing_string');
    rejects(p => { p.nodes[0]!.authoring.detail_string_key = 'missing'; }, 'validation.missing_string');
    rejects(p => { p.strings[0]!.value = ''; }, 'validation.missing_string');
  });
  it('rejects malformed contract fields and preserves optional unknown authoring data', () => {
    for (const value of [null, {}, { schemaVersion: 2, nodes: [], strings: [] }]) expect(() => validatePack(value)).toThrow(ValidationError);
    rejects(p => { p.nodes[0]!.authoring.prerequisitesStatus = { toString: () => 'confirmed' } as never; }, 'validation.node');
    rejects(p => { p.nodes[0]!.definition.required_completed_quest_ids = '21' as never; }, 'validation.node');
  });
  it('rejects missing, duplicate, self and cyclic prerequisites', () => {
    rejects(p => { p.nodes[0]!.definition.required_completed_quest_ids = ['999']; }, 'validation.unknown_quest');
    rejects(p => { p.nodes[0]!.definition.required_completed_quest_ids = ['21', '21']; }, 'validation.duplicate_prerequisite');
    rejects(p => { p.nodes[0]!.definition.required_completed_quest_ids = ['1']; }, 'validation.self_reference');
    rejects(p => { p.nodes[3]!.definition.required_completed_quest_ids = ['2']; }, 'validation.cycle');
  });
});

describe('graph editing and chapter projection', () => {
  it('resolves global numeric IDs without consulting the selected chapter', () => {
    const pack = fixture();
    expect(resolveQuestId(pack, '1')).toBe('1');
    expect(resolveQuestId(pack, ' 021 ')).toBe('21');
    pack.nodes[0]!.definition.authoring_tags = ['chapter:9'];
    expect(resolveQuestId(pack, '1')).toBe('1');
    for (const input of ['R1-01', 'legacy_intro', '-1', '1.0', '99']) {
      expect(() => resolveQuestId(pack, input)).toThrow(ValidationError);
    }
  });
  it('includes only direct outside prerequisites, without expanding their ancestors', () => {
    const projection = projectChapter(fixture(), '1');
    expect(projection.nodes.map(n => [n.questId, n.external])).toEqual([['1', false], ['2', false], ['21', true]]);
    expect(projection.edges).toEqual([{ source: '21', target: '2' }]);
  });
  it('places canonical shared external cards in a separate nonoverlapping column without changing saved coordinates', () => {
    const pack = fixture();
    pack.nodes[0]!.x = 0; pack.nodes[0]!.y = 0;
    pack.nodes[1]!.x = 360; pack.nodes[1]!.y = 0;
    pack.nodes[2]!.x = 0; pack.nodes[2]!.y = 0;
    pack.nodes[3]!.x = 0; pack.nodes[3]!.y = 0;
    pack.nodes[0]!.definition.required_completed_quest_ids = ['21', '57'];
    pack.nodes[3]!.definition.required_completed_quest_ids = ['90'];
    pack.nodes.push({ ...pack.nodes[3]!, definition: { ...pack.nodes[3]!.definition, quest_id: '90', authoring_tags: ['chapter:4'], required_completed_quest_ids: [] } });
    validatePack(pack);
    const before = JSON.stringify(pack);
    const projection = projectChapter(pack, '1');
    const boundary = projection.nodes.filter(node => node.external);
    expect(boundary.map(node => node.questId)).toEqual(['21', '57']);
    expect(projection.edges.filter(edge => edge.source === '21')).toHaveLength(2);
    expect(boundary.every(node => node.x + 248 < 0)).toBe(true);
    expect(boundary[0]!.x).toBe(boundary[1]!.x);
    expect(boundary[1]!.y - boundary[0]!.y).toBeGreaterThan(116);
    expect(projection.nodes.some(node => node.questId === '90')).toBe(false);
    expect(projectChapter({ ...pack, nodes: [...pack.nodes].reverse() }, '1').nodes.filter(node => node.external)).toEqual(boundary);
    expect(JSON.stringify(pack)).toBe(before);
  });
  it('edits immutably while preserving hidden nodes, strings and unknown fields', () => {
    const pack = fixture(); const edited = updatePrerequisites(pack, '2', ['1']);
    expect(edited.nodes[1]!.definition.required_completed_quest_ids).toEqual(['1']);
    expect(edited.nodes[2]!).toEqual(pack.nodes[2]!); expect(edited.strings).toEqual(pack.strings);
    expect(pack.nodes[1]!.definition.required_completed_quest_ids).toEqual(['21']);
    expect(edited.nodes[0]!.authoring.prerequisitesStatus).toBe('unspecified');
  });
  it('rejects edits that create a cycle without changing input data', () => {
    const pack = fixture(); const before = JSON.stringify(pack);
    expect(() => updatePrerequisites(pack, '57', ['2'])).toThrow(ValidationError);
    expect(JSON.stringify(pack)).toBe(before);
  });
});
