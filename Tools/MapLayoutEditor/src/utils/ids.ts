import type { MapObjectType } from "../types";

const prefixes: Record<MapObjectType, string> = {
  point: "point",
  rect: "area",
  path: "path"
};

export function createObjectId(type: MapObjectType): string {
  const timestamp = Date.now().toString(36);
  const random = Math.random().toString(36).slice(2, 8);
  return `${prefixes[type]}_${timestamp}_${random}`;
}
