import type { QuestDataset } from "../shared/types";
import { normalizeQuestDataset } from "../shared/dataset";

interface StoredCatalogDraft {
  schemaVersion: 2;
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
    schemaVersion: 2,
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
      value.schemaVersion !== 2 ||
      value.datasetVersion !== expectedDatasetVersion ||
      !isQuestDataset(value.dataset)
    ) {
      return { dataset: null, discarded: true };
    }
    return {
      dataset: normalizeQuestDataset(value.dataset),
      discarded: false,
    };
  } catch {
    return { dataset: null, discarded: true };
  }
}

function isQuestDataset(value: unknown): value is QuestDataset {
  const dataset = normalizeQuestDataset(value);
  return dataset !== null && dataset.schemaVersion === 2;
}
