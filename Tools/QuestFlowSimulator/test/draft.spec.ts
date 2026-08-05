import { describe, expect, it } from "vitest";
import demoCatalogSource from "../data/demo.json";
import {
  readCatalogDraft,
  serializeCatalogDraft,
} from "../src/client/draft";
import { normalizeQuestDataset } from "../src/shared/dataset";

describe("public catalog drafts", () => {
  const dataset = normalizeQuestDataset(demoCatalogSource.data)!;

  it("restores a draft only when its dataset version matches", () => {
    const raw = serializeCatalogDraft(
      demoCatalogSource.datasetVersion,
      dataset,
    );

    expect(
      readCatalogDraft(raw, demoCatalogSource.datasetVersion),
    ).toEqual({
      dataset,
      discarded: false,
    });
  });

  it("discards legacy and stale drafts", () => {
    expect(
      readCatalogDraft(
        JSON.stringify(demoCatalogSource.data),
        demoCatalogSource.datasetVersion,
      ),
    ).toEqual({ dataset: null, discarded: true });

    const stale = serializeCatalogDraft(
      "content-stale",
      dataset,
    );
    expect(
      readCatalogDraft(stale, demoCatalogSource.datasetVersion),
    ).toEqual({ dataset: null, discarded: true });
  });
});
