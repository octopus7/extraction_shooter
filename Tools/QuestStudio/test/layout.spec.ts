import { describe, expect, it } from 'vitest';
import { arrangeChapterNodes } from '../src/shared/layout';
import type { QuestPack } from '../src/shared/types';

function fixture(): QuestPack {
  const dependencies: Record<string, string[]> = { '1': [], '2': ['1'], '3': ['1'], '4': ['2'], '5': ['4'], '6': ['3'], '7': ['5', '6'], '8': [], '9': ['7'] };
  return { schemaVersion: 1, unknown: { preserve: true }, nodes: Object.entries(dependencies).map(([id, prerequisites]) => ({
    definition: { quest_id: Number(id), title_string_key: 't', description_string_key: 't', required_completed_quest_ids: prerequisites.map(Number), authoring_tags: [id === '9' ? 'chapter:2' : 'chapter:1'] },
    x: 0, y: 0, authoring: { prerequisitesStatus: 'confirmed', custom: id },
  })), strings: [{ key: 't', locale: 'ko', value: '합성' }] };
}

describe('explicit chapter layout', () => {
  it('separates branches and components with non-overlapping cards and forward dependency columns', () => {
    const pack = fixture();
    const arranged = arrangeChapterNodes(pack, '1');
    const visible = arranged.nodes.filter(node => node.definition.quest_id !== 9);
    const byId = new Map(visible.map(node => [node.definition.quest_id, node]));
    for (const node of visible) {
      for (const prerequisite of node.definition.required_completed_quest_ids) expect(node.x).toBeGreaterThan(byId.get(prerequisite)!.x);
      for (const other of visible) if (node !== other) expect(Math.abs(node.x - other.x) >= 248 || Math.abs(node.y - other.y) >= 116).toBe(true);
    }
    expect(byId.get(2)!.y).toBe(byId.get(4)!.y);
    expect(byId.get(4)!.y).toBe(byId.get(5)!.y);
    expect(byId.get(2)!.y).not.toBe(byId.get(3)!.y);
    expect(byId.get(8)!.y).toBeGreaterThan(Math.max(...visible.filter(node => node.definition.quest_id !== 8).map(node => node.y)));
    expect(arranged.nodes[8]).toBe(pack.nodes[8]);
    expect(arranged.strings).toBe(pack.strings);
    expect(arranged.unknown).toBe(pack.unknown);
    expect(pack.nodes.every(node => node.x === 0 && node.y === 0)).toBe(true);
    for (let index = 0; index < pack.nodes.length; index++) {
      expect(arranged.nodes[index]!.definition).toBe(pack.nodes[index]!.definition);
      expect(arranged.nodes[index]!.authoring).toBe(pack.nodes[index]!.authoring);
    }
  });

  it('is deterministic independent of source ordering, idempotent, and ignores other chapters', () => {
    const pack = fixture();
    const arranged = arrangeChapterNodes(pack, '1');
    const reversed = arrangeChapterNodes({ ...pack, nodes: [...pack.nodes].reverse() }, '1');
    for (const node of arranged.nodes) expect(reversed.nodes.find(other => other.definition.quest_id === node.definition.quest_id)).toEqual(node);
    expect(arrangeChapterNodes(arranged, '1')).toBe(arranged);
    expect(arrangeChapterNodes(pack, '99')).toBe(pack);
  });
});
