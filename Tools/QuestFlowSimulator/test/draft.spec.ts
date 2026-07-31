import { describe, expect, it } from "vitest";
import demoCatalogSource from "../data/demo.json";
import {
  readCatalogDraft,
  serializeCatalogDraft,
} from "../src/client/draft";

describe("public catalog drafts", () => {
  it("restores a draft only when its dataset version matches", () => {
    const raw = serializeCatalogDraft(
      demoCatalogSource.datasetVersion,
      demoCatalogSource.data,
    );

    expect(
      readCatalogDraft(raw, demoCatalogSource.datasetVersion),
    ).toEqual({
      dataset: demoCatalogSource.data,
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
      demoCatalogSource.data,
    );
    expect(
      readCatalogDraft(stale, demoCatalogSource.datasetVersion),
    ).toEqual({ dataset: null, discarded: true });
  });
});
