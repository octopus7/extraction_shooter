import { describe, expect, it } from "vitest";
import demoCatalogSource from "../data/demo.json";

describe("public demo fallback", () => {
  it("uses only locations generated from the demo design documents", () => {
    expect(
      demoCatalogSource.data.locations.map((location) => location.name),
    ).toEqual([
      "벙커",
      "공구 보관 장소",
      "취수 시설",
      "북쪽 파밍 지역",
      "외부 식량 창고",
    ]);
  });
});
