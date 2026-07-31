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

  it("keeps the existing seeded coordinates unchanged until auto-layout is requested", () => {
    const nodes = demoCatalogSource.data.questNodes;
    expect(nodes.map((node) => [node.x, node.y])).toEqual([
      [100, 100],
      [290, 100],
      [480, 100],
      [100, 230],
      [290, 230],
    ]);
  });

  it("keeps the existing main graph coordinates unchanged until auto-layout is requested", () => {
    expect(mainCatalogSource.data.questNodes[0]).toMatchObject({
      questId: "M01",
      x: 100,
      y: 100,
    });
    expect(mainCatalogSource.data.questNodes[19]).toMatchObject({
      questId: "M20",
      x: 780,
      y: 430,
    });
  });
});
