import type { QuestDataset } from "../shared/types";

interface StoredCatalogDraft {
  schemaVersion: 1;
  datasetVersion: string;
  dataset: QuestDataset;
}

export interface CatalogDraftResult {
  dataset: QuestDataset | null;
  discarded: boolean;
}

export function serializeCatalogDraft(
  datasetVersion: string,
  dataset: QuestDataset,
): string {
  const draft: StoredCatalogDraft = {
    schemaVersion: 1,
    datasetVersion,
    dataset,
  };
  return JSON.stringify(draft);
}

export function readCatalogDraft(
  raw: string | null,
  expectedDatasetVersion: string | undefined,
): CatalogDraftResult {
  if (!raw) {
    return { dataset: null, discarded: false };
  }

  try {
    const value = JSON.parse(raw) as Partial<StoredCatalogDraft>;
    if (
      !expectedDatasetVersion ||
      value.schemaVersion !== 1 ||
      value.datasetVersion !== expectedDatasetVersion ||
      !isQuestDataset(value.dataset)
    ) {
      return { dataset: null, discarded: true };
    }
    return { dataset: value.dataset, discarded: false };
  } catch {
    return { dataset: null, discarded: true };
  }
}

function isQuestDataset(value: unknown): value is QuestDataset {
  if (!value || typeof value !== "object") return false;
  const dataset = value as Partial<QuestDataset>;
  return (
    typeof dataset.schemaVersion === "number" &&
    typeof dataset.title === "string" &&
    Array.isArray(dataset.locations) &&
    Array.isArray(dataset.steps) &&
    Boolean(dataset.settings) &&
    typeof dataset.settings?.runs === "number"
  );
}
