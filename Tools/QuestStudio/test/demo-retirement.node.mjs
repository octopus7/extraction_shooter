import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import test from 'node:test';
import { parse } from 'csv-parse/sync';

const dataRoot = new URL('../../../TunaSweeper/Content/Data/', import.meta.url);
const read = (name) => readFileSync(new URL(name, dataRoot), 'utf8');

test('retired demo catalog is empty and scenarios have no dangling quest references', () => {
  const definitions = JSON.parse(read('QuestDefinitions.json'));
  assert.deepEqual(definitions, []);
  const knownIds = new Set(definitions.map((quest) => quest.quest_id));
  const scenarios = JSON.parse(read('ScenarioDefinitions.json')).scenarios;
  assert.ok(scenarios.every((scenario) => scenario.scenario_id !== 'scenario.demo.ending.dinner'));
  for (const scenario of scenarios) {
    for (const condition of scenario.required_quest_states ?? []) {
      assert.ok(knownIds.has(condition.quest_id), `Dangling quest: ${condition.quest_id}`);
    }
  }
});

test('quest-specific translations retire together while shared UI strings remain', () => {
  for (const name of ['QuestTextStrings.csv', 'Translations/QuestTextStrings.csv']) {
    const rows = parse(read(name), { columns: true, skip_empty_lines: true, bom: true });
    assert.ok(rows.length > 0, `${name} must keep shared strings`);
    assert.ok(rows.every((row) => !row.string_key.startsWith('quest.demo_q')));
    assert.ok(rows.some((row) => row.string_key === 'quest.ui.title'));
  }
  for (const name of ['ScenarioTextStrings.csv', 'Translations/ScenarioTextStrings.csv']) {
    const rows = parse(read(name), { columns: true, skip_empty_lines: true, bom: true });
    assert.ok(rows.every((row) => !row.string_key.startsWith('scenario.demo.ending.dinner.')));
    assert.ok(rows.some((row) => row.string_key === 'scenario.demo.bunker_toilet.line1'));
  }
});
