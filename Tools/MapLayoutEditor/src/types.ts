export const PROJECT_SAVE_VERSION = 1;
export const METER_TO_UNREAL_UNIT = 100;

export interface Vector2 {
  x: number;
  y: number;
}

export interface Vector3 {
  x: number;
  y: number;
  z: number;
}

export interface CameraState {
  x: number;
  y: number;
  zoom: number;
}

export type MapObjectType = "point" | "rect" | "path";

export interface MapObject {
  id: string;
  type: MapObjectType;
  name: string;
  position: Vector3;
  size: Vector2;
  rotation: number;
  tags: string[];
  note: string;
  color: string;
  visible: boolean;
  locked: boolean;
  points?: Vector2[];
}

export interface MapData {
  objects: MapObject[];
}

export type EditorTool = "select" | "add-point" | "add-rect" | "add-path";

export interface EditorState {
  camera: CameraState;
  showLabels: boolean;
  selectedObjectId: string | null;
  activeTool: EditorTool;
}

export interface ProjectSaveData {
  version: number;
  unit: "meter";
  mapData: MapData;
  editorState: EditorState;
}

export type AutosaveStatus = "idle" | "pending" | "saved" | "error";
