import type {
  EditorState,
  EditorTool,
  MapData,
  MapObject,
  MapObjectType,
  ProjectSaveData,
  Vector2,
  Vector3
} from "../types";
import { PROJECT_SAVE_VERSION } from "../types";
import { createObjectId } from "./ids";
import { DEFAULT_EDITOR_STATE } from "./sampleData";

const editorTools: EditorTool[] = ["select", "add-point", "add-rect", "add-path"];
const objectTypes: MapObjectType[] = ["point", "rect", "path"];

export function createProjectSaveData(
  mapData: MapData,
  editorState: EditorState
): ProjectSaveData {
  return {
    version: PROJECT_SAVE_VERSION,
    unit: "meter",
    mapData,
    editorState
  };
}

export function normalizeProjectSaveData(value: unknown): ProjectSaveData {
  const source = asRecord(value);
  const mapData = normalizeMapData(source.mapData);
  const editorState = normalizeEditorState(source.editorState);

  return {
    version: normalizeNumber(source.version, PROJECT_SAVE_VERSION),
    unit: "meter",
    mapData,
    editorState
  };
}

export function downloadProjectJson(project: ProjectSaveData): void {
  const blob = new Blob([JSON.stringify(project, null, 2)], {
    type: "application/json;charset=utf-8"
  });
  const url = URL.createObjectURL(blob);
  const anchor = document.createElement("a");
  anchor.href = url;
  anchor.download = "map-layout-project.json";
  document.body.appendChild(anchor);
  anchor.click();
  anchor.remove();
  URL.revokeObjectURL(url);
}

export async function readProjectFile(file: File): Promise<ProjectSaveData> {
  const text = await file.text();
  return normalizeProjectSaveData(JSON.parse(text));
}

function normalizeMapData(value: unknown): MapData {
  const source = asRecord(value);
  const objects = Array.isArray(source.objects)
    ? source.objects.map(normalizeMapObject)
    : [];

  return {
    objects
  };
}

function normalizeMapObject(value: unknown): MapObject {
  const source = asRecord(value);
  const type = normalizeObjectType(source.type);
  const position = normalizeVector3(source.position, { x: 0, y: 0, z: 0 });
  const size = normalizeVector2(source.size, { x: 1, y: 1 });
  const points = normalizePathPoints(source.points, position, type);

  return {
    id: normalizeString(source.id, createObjectId(type)),
    type,
    name: normalizeString(source.name, defaultNameForType(type)),
    position,
    size,
    rotation: normalizeNumber(source.rotation, 0),
    tags: normalizeTags(source.tags),
    note: normalizeString(source.note, ""),
    color: normalizeColor(source.color),
    visible: normalizeBoolean(source.visible, true),
    locked: normalizeBoolean(source.locked, false),
    ...(type === "path" ? { points } : {})
  };
}

function normalizeEditorState(value: unknown): EditorState {
  const source = asRecord(value);
  const base = DEFAULT_EDITOR_STATE;
  const camera = asRecord(source.camera);

  return {
    camera: {
      x: normalizeNumber(camera.x, base.camera.x),
      y: normalizeNumber(camera.y, base.camera.y),
      zoom: normalizeNumber(camera.zoom, base.camera.zoom)
    },
    showLabels: normalizeBoolean(source.showLabels, base.showLabels),
    selectedObjectId: null,
    activeTool: editorTools.includes(source.activeTool as EditorTool)
      ? (source.activeTool as EditorTool)
      : "select"
  };
}

function normalizePathPoints(
  value: unknown,
  position: Vector3,
  type: MapObjectType
): Vector2[] | undefined {
  if (type !== "path") {
    return undefined;
  }

  if (!Array.isArray(value)) {
    return [
      { x: position.x, y: position.y },
      { x: position.x + 6, y: position.y + 8 }
    ];
  }

  const points = value
    .map((point) => normalizeVector2(point, { x: position.x, y: position.y }))
    .filter((point) => Number.isFinite(point.x) && Number.isFinite(point.y));

  if (points.length >= 2) {
    return points;
  }

  return [
    { x: position.x, y: position.y },
    { x: position.x + 6, y: position.y + 8 }
  ];
}

function normalizeObjectType(value: unknown): MapObjectType {
  return objectTypes.includes(value as MapObjectType)
    ? (value as MapObjectType)
    : "point";
}

function normalizeVector2(value: unknown, fallback: Vector2): Vector2 {
  const source = asRecord(value);
  return {
    x: normalizeNumber(source.x, fallback.x),
    y: normalizeNumber(source.y, fallback.y)
  };
}

function normalizeVector3(value: unknown, fallback: Vector3): Vector3 {
  const source = asRecord(value);
  return {
    x: normalizeNumber(source.x, fallback.x),
    y: normalizeNumber(source.y, fallback.y),
    z: normalizeNumber(source.z, fallback.z)
  };
}

function normalizeTags(value: unknown): string[] {
  if (!Array.isArray(value)) {
    return [];
  }

  return value
    .map((tag) => String(tag).trim())
    .filter((tag) => tag.length > 0);
}

function normalizeColor(value: unknown): string {
  const color = normalizeString(value, "#ffcc00");
  return /^#[0-9a-fA-F]{6}$/.test(color) ? color : "#ffcc00";
}

function normalizeNumber(value: unknown, fallback: number): number {
  const numberValue = typeof value === "number" ? value : Number(value);
  return Number.isFinite(numberValue) ? numberValue : fallback;
}

function normalizeBoolean(value: unknown, fallback: boolean): boolean {
  return typeof value === "boolean" ? value : fallback;
}

function normalizeString(value: unknown, fallback: string): string {
  return typeof value === "string" && value.trim().length > 0
    ? value
    : fallback;
}

function defaultNameForType(type: MapObjectType): string {
  switch (type) {
    case "rect":
      return "New Area";
    case "path":
      return "New Path";
    case "point":
    default:
      return "New Point";
  }
}

function asRecord(value: unknown): Record<string, unknown> {
  return value !== null && typeof value === "object"
    ? (value as Record<string, unknown>)
    : {};
}
