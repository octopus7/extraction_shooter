import { describe, expect, it } from "vitest";
import { pageFromPath } from "../src/client/page";

describe("Quest Flow page routing", () => {
  it("uses the chain viewer as the main page", () => {
    expect(pageFromPath("/")).toBe("quest-viewer");
    expect(pageFromPath("/quests/demo")).toBe("quest-viewer");
  });

  it("keeps simulation on an independent path", () => {
    expect(pageFromPath("/simulation")).toBe("simulation");
    expect(pageFromPath("/simulation/demo")).toBe("simulation");
  });
});
