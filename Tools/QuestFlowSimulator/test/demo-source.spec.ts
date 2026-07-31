import { describe, expect, it } from "vitest";
import demoCatalogSource from "../data/demo.json";
import mainCatalogSource from "../data/main-m01-m20.json";

describe("public demo fallback", () => {
  it("uses only map places generated from the demo design documents", () => {
    expect(
      demoCatalogSource.data.places.map((place) => place.name),
    ).toEqual([
      "벙커",
      "공구 보관 장소",
      "취수 시설",
      "북쪽 파밍 지역",
      "외부 식량 창고",
    ]);
  });

  it("keeps quest graph nodes separate from map places", () => {
    expect(demoCatalogSource.data.schemaVersion).toBe(2);
    expect(demoCatalogSource.data.questNodes.map((node) => node.questId)).toEqual([
      "q1",
      "q2",
      "q3-1",
      "q3-2",
      "q4",
    ]);
    expect(
      demoCatalogSource.data.places.some((place) => place.name.includes("Q1")),
    ).toBe(false);
    expect(
      demoCatalogSource.data.steps.every(
        (step) => "fromPlaceId" in step && "toPlaceId" in step,
      ),
    ).toBe(true);
  });

  it("auto-layouts quests by prerequisite depth before wrapping rows", () => {
    const nodes = demoCatalogSource.data.questNodes;
    const nodeById = new Map(nodes.map((node) => [node.questId, node]));

    expect(nodeById.get("q1")!.x).toBeLessThan(nodeById.get("q2")!.x);
    expect(nodeById.get("q2")!.x).toBeLessThan(nodeById.get("q3-1")!.x);
    expect(nodeById.get("q3-1")!.x).toBe(nodeById.get("q3-2")!.x);
    expect(nodeById.get("q4")!.x).toBeLessThan(nodeById.get("q3-1")!.x);
    expect(nodeById.get("q4")!.y).toBeGreaterThan(nodeById.get("q3-2")!.y);
  });

  it("wraps the long main chain without losing prerequisite order within a row", () => {
    const nodes = mainCatalogSource.data.questNodes;
    const nodeById = new Map(nodes.map((node) => [node.questId, node]));

    expect(nodeById.get("M01")!.x).toBeLessThan(nodeById.get("M05")!.x);
    expect(nodeById.get("M06")!.y).toBeGreaterThan(nodeById.get("M05")!.y);
    expect(nodeById.get("M06")!.x).toBe(nodeById.get("M07")!.x);
    expect(nodeById.get("M07")!.x).toBe(nodeById.get("M08")!.x);
    expect(nodeById.get("M13")!.x).toBeLessThan(nodeById.get("M17")!.x);
    expect(nodeById.get("M18")!.x).toBeLessThan(nodeById.get("M20")!.x);
  });
});
