import { describe, expect, it } from "vitest";
import {
  AuthoringValidationError,
  hashAuthoringPack,
  normalizeAuthoringPack,
  stableStringify,
} from "../src/shared/authoring";

function pack() {
  return {
    schemaVersion: 3,
    flavor: "Demo",
    runtime: {
      questDefinitions: [
        { quest_id: "Q1", required_completed_quest_ids: [] },
        { quest_id: "Q2", required_completed_quest_ids: ["Q1"] },
      ],
      questTextStringsCsv: "key,text\r\nq1,Quest 1\r\n",
    },
    editor: {
      schemaVersion: 2,
      title: "Demo",
      questNodes: [
        { id: "n1", questId: "Q1", title: "Quest 1", x: 0, y: 0, prerequisiteQuestIds: [] },
        { id: "n2", questId: "Q2", title: "Quest 2", x: 1, y: 1, prerequisiteQuestIds: ["Q1"] },
      ],
      places: [],
      steps: [],
      settings: { runs: 100 },
    },
  };
}

describe("quest authoring pack", () => {
  it("normalizes CSV line endings and hashes canonical JSON deterministically", async () => {
    const normalized = normalizeAuthoringPack(pack());
    expect(normalized.runtime.questTextStringsCsv).toBe("key,text\nq1,Quest 1\n");
    expect(stableStringify({ b: 2, a: 1 })).toBe('{"a":1,"b":2}');
    await expect(hashAuthoringPack(normalized)).resolves.toMatch(/^[0-9a-f]{64}$/u);
    await expect(
      hashAuthoringPack(normalizeAuthoringPack(JSON.parse(JSON.stringify(pack())))),
    ).resolves.toBe(await hashAuthoringPack(normalized));
  });

  it("rejects dangling runtime and editor prerequisites", () => {
    const invalid = structuredClone(pack());
    invalid.runtime.questDefinitions[1]!.required_completed_quest_ids = ["missing"];
    invalid.editor.questNodes[1]!.prerequisiteQuestIds = ["missing"];
    expect(() => normalizeAuthoringPack(invalid)).toThrow(AuthoringValidationError);
  });
});
