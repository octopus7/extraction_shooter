import { z } from "zod";
import { normalizeQuestDataset } from "./dataset";
import type {
  JsonValue,
  QuestAuthoringPack,
  QuestDataset,
  QuestFlavor,
} from "./types";

export const AUTHORING_SCHEMA_VERSION = 3 as const;

const jsonValueSchema: z.ZodType<JsonValue> = z.lazy(() =>
  z.union([
    z.null(),
    z.boolean(),
    z.number().finite(),
    z.string(),
    z.array(jsonValueSchema),
    z.record(z.string(), jsonValueSchema),
  ]),
);

const rawPackSchema = z
  .object({
    schemaVersion: z.literal(AUTHORING_SCHEMA_VERSION),
    flavor: z.enum(["Demo", "Main"]),
    runtime: z
      .object({
        questDefinitions: z.array(jsonValueSchema).max(2_000),
        questTextStringsCsv: z.string().max(220_000),
      })
      .strict(),
    editor: z.record(z.string(), jsonValueSchema),
  })
  .strict();

export class AuthoringValidationError extends Error {
  constructor(readonly issues: string[]) {
    super(issues.join("\n"));
  }
}

export function normalizeAuthoringPack(value: unknown): QuestAuthoringPack {
  const parsed = rawPackSchema.safeParse(value);
  if (!parsed.success) {
    throw new AuthoringValidationError(
      parsed.error.issues.map((issue) =>
        `${issue.path.join(".") || "pack"}: ${issue.message}`,
      ),
    );
  }

  const editor = normalizeQuestDataset(parsed.data.editor);
  if (!editor) {
    throw new AuthoringValidationError(["editor: invalid quest editor dataset"]);
  }

  const pack: QuestAuthoringPack = {
    schemaVersion: AUTHORING_SCHEMA_VERSION,
    flavor: parsed.data.flavor,
    runtime: {
      questDefinitions: parsed.data.runtime.questDefinitions,
      questTextStringsCsv: normalizeCsv(parsed.data.runtime.questTextStringsCsv),
    },
    editor,
  };
  validateAuthoringPack(pack);
  return pack;
}

export function validateAuthoringPack(pack: QuestAuthoringPack): void {
  const issues = [
    ...validateEditor(pack.editor),
    ...validateRuntimeDefinitions(pack.runtime.questDefinitions),
  ];
  if (issues.length > 0) {
    throw new AuthoringValidationError(issues);
  }
}

export function flavorForSlug(slug: string): QuestFlavor | null {
  if (slug === "demo") return "Demo";
  if (slug === "main-m01-m20") return "Main";
  return null;
}

export async function hashAuthoringPack(
  pack: QuestAuthoringPack,
): Promise<string> {
  const bytes = new TextEncoder().encode(stableStringify(pack));
  const digest = await crypto.subtle.digest("SHA-256", bytes);
  return Array.from(new Uint8Array(digest), (byte) =>
    byte.toString(16).padStart(2, "0"),
  ).join("");
}

export function stableStringify(value: unknown): string {
  return JSON.stringify(sortValue(value));
}

function sortValue(value: unknown): unknown {
  if (Array.isArray(value)) return value.map(sortValue);
  if (!value || typeof value !== "object") return value;
  return Object.fromEntries(
    Object.entries(value as Record<string, unknown>)
      .sort(([left], [right]) => left.localeCompare(right))
      .map(([key, child]) => [key, sortValue(child)]),
  );
}

function normalizeCsv(value: string): string {
  const withoutBom = value.charCodeAt(0) === 0xfeff ? value.slice(1) : value;
  return withoutBom.replaceAll("\r\n", "\n").replaceAll("\r", "\n");
}

function validateEditor(editor: QuestDataset): string[] {
  const issues: string[] = [];
  const questIds = new Set<string>();
  for (const node of editor.questNodes) {
    if (questIds.has(node.questId)) {
      issues.push(`editor.questNodes: duplicate questId ${node.questId}`);
    }
    questIds.add(node.questId);
  }
  for (const node of editor.questNodes) {
    for (const prerequisite of node.prerequisiteQuestIds) {
      if (!questIds.has(prerequisite)) {
        issues.push(
          `editor.questNodes.${node.questId}: unknown prerequisite ${prerequisite}`,
        );
      }
      if (prerequisite === node.questId) {
        issues.push(`editor.questNodes.${node.questId}: self prerequisite`);
      }
    }
  }
  const placeIds = new Set(editor.places.map((place) => place.id));
  for (const step of editor.steps) {
    if (!questIds.has(step.questId)) {
      issues.push(`editor.steps.${step.id}: unknown questId ${step.questId}`);
    }
    if (!placeIds.has(step.fromPlaceId) || !placeIds.has(step.toPlaceId)) {
      issues.push(`editor.steps.${step.id}: unknown place reference`);
    }
  }
  return issues;
}

function validateRuntimeDefinitions(definitions: JsonValue[]): string[] {
  const issues: string[] = [];
  const questIds = new Set<string>();
  const rows: Array<{ questId: string; value: Record<string, JsonValue> }> = [];
  for (const [index, value] of definitions.entries()) {
    if (!value || typeof value !== "object" || Array.isArray(value)) {
      issues.push(`runtime.questDefinitions.${index}: expected object`);
      continue;
    }
    const questId = readString(value, "quest_id", "questId");
    if (!questId) {
      issues.push(`runtime.questDefinitions.${index}: quest_id is required`);
      continue;
    }
    if (questIds.has(questId)) {
      issues.push(`runtime.questDefinitions: duplicate quest_id ${questId}`);
    }
    questIds.add(questId);
    rows.push({ questId, value });
  }
  for (const row of rows) {
    const prerequisites = readStringArray(
      row.value,
      "required_completed_quest_ids",
      "requiredCompletedQuestIds",
    );
    for (const prerequisite of prerequisites) {
      if (!questIds.has(prerequisite)) {
        issues.push(
          `runtime.questDefinitions.${row.questId}: unknown prerequisite ${prerequisite}`,
        );
      }
    }
  }
  return issues;
}

function readString(
  value: Record<string, JsonValue>,
  snakeKey: string,
  camelKey: string,
): string | null {
  const candidate = value[snakeKey] ?? value[camelKey];
  return typeof candidate === "string" && candidate.trim()
    ? candidate.trim()
    : null;
}

function readStringArray(
  value: Record<string, JsonValue>,
  snakeKey: string,
  camelKey: string,
): string[] {
  const candidate = value[snakeKey] ?? value[camelKey];
  return Array.isArray(candidate)
    ? candidate.filter((item): item is string => typeof item === "string")
    : [];
}
