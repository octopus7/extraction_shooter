import { create } from "zustand";
import type {
  AutosaveStatus,
  CameraState,
  EditorState,
  EditorTool,
  MapData,
  MapObject,
  MapObjectType,
  ProjectSaveData,
  Vector2,
  Vector3
} from "../types";
import { createProjectSaveData } from "../utils/exportImport";
import { createObjectId } from "../utils/ids";
import { createSampleProjectData, DEFAULT_EDITOR_STATE } from "../utils/sampleData";
import { clearAutosave, loadAutosave } from "../utils/storage";

interface EditorStore {
  mapData: MapData;
  editorState: EditorState;
  autosaveStatus: AutosaveStatus;
  hasLoaded: boolean;
  initializeProject: () => void;
  setAutosaveStatus: (status: AutosaveStatus) => void;
  setActiveTool: (tool: EditorTool) => void;
  setCamera: (camera: CameraState) => void;
  panCamera: (delta: Vector2) => void;
  resetView: () => void;
  toggleLabels: () => void;
  selectObject: (id: string | null) => void;
  addObject: (type: MapObjectType, position: Vector2) => void;
  updateObject: (id: string, patch: Partial<MapObject>) => void;
  moveObject: (id: string, position: Vector2) => void;
  deleteSelectedObject: () => void;
  resetProject: () => void;
  importProject: (project: ProjectSaveData) => void;
  getProjectSaveData: () => ProjectSaveData;
}

const initialProject = createSampleProjectData();

export const useEditorStore = create<EditorStore>((set, get) => ({
  mapData: initialProject.mapData,
  editorState: initialProject.editorState,
  autosaveStatus: "idle",
  hasLoaded: false,

  initializeProject: () => {
    const project = loadAutosave() ?? createSampleProjectData();
    set({
      mapData: project.mapData,
      editorState: {
        ...project.editorState,
        selectedObjectId: null
      },
      hasLoaded: true,
      autosaveStatus: "idle"
    });
  },

  setAutosaveStatus: (status) => set({ autosaveStatus: status }),

  setActiveTool: (tool) =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        activeTool: tool
      }
    })),

  setCamera: (camera) =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        camera
      }
    })),

  panCamera: (delta) =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        camera: {
          ...state.editorState.camera,
          x: state.editorState.camera.x + delta.x,
          y: state.editorState.camera.y + delta.y
        }
      }
    })),

  resetView: () =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        camera: DEFAULT_EDITOR_STATE.camera
      }
    })),

  toggleLabels: () =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        showLabels: !state.editorState.showLabels
      }
    })),

  selectObject: (id) =>
    set((state) => ({
      editorState: {
        ...state.editorState,
        selectedObjectId: id
      }
    })),

  addObject: (type, position) => {
    const object = createMapObject(type, position);

    set((state) => ({
      mapData: {
        objects: [...state.mapData.objects, object]
      },
      editorState: {
        ...state.editorState,
        selectedObjectId: object.id
      }
    }));
  },

  updateObject: (id, patch) =>
    set((state) => ({
      mapData: {
        objects: state.mapData.objects.map((object) =>
          object.id === id ? normalizePatchResult({ ...object, ...patch }) : object
        )
      }
    })),

  moveObject: (id, position) =>
    set((state) => ({
      mapData: {
        objects: state.mapData.objects.map((object) => {
          if (object.id !== id || object.locked) {
            return object;
          }

          const delta = {
            x: position.x - object.position.x,
            y: position.y - object.position.y
          };

          return {
            ...object,
            position: {
              ...object.position,
              x: position.x,
              y: position.y
            },
            points:
              object.type === "path" && object.points
                ? object.points.map((point) => ({
                    x: point.x + delta.x,
                    y: point.y + delta.y
                  }))
                : object.points
          };
        })
      }
    })),

  deleteSelectedObject: () => {
    const selectedObjectId = get().editorState.selectedObjectId;
    if (!selectedObjectId) {
      return;
    }

    const selected = get().mapData.objects.find(
      (object) => object.id === selectedObjectId
    );
    if (!selected || selected.locked) {
      return;
    }

    set((state) => ({
      mapData: {
        objects: state.mapData.objects.filter(
          (object) => object.id !== selectedObjectId
        )
      },
      editorState: {
        ...state.editorState,
        selectedObjectId: null
      }
    }));
  },

  resetProject: () => {
    clearAutosave();
    const project = createSampleProjectData();
    set({
      mapData: project.mapData,
      editorState: project.editorState,
      autosaveStatus: "idle"
    });
  },

  importProject: (project) =>
    set({
      mapData: project.mapData,
      editorState: {
        ...project.editorState,
        selectedObjectId: null
      },
      autosaveStatus: "pending"
    }),

  getProjectSaveData: () => {
    const state = get();
    return createProjectSaveData(state.mapData, state.editorState);
  }
}));

function createMapObject(type: MapObjectType, position: Vector2): MapObject {
  const id = createObjectId(type);
  const basePosition: Vector3 = { x: position.x, y: position.y, z: 0 };

  switch (type) {
    case "rect":
      return {
        id,
        type,
        name: "New Area",
        position: basePosition,
        size: { x: 4, y: 3 },
        rotation: 0,
        tags: [],
        note: "",
        color: "#4b8f7a",
        visible: true,
        locked: false
      };
    case "path":
      return {
        id,
        type,
        name: "New Path",
        position: basePosition,
        size: { x: 1, y: 1 },
        rotation: 0,
        tags: [],
        note: "",
        color: "#d06f2f",
        visible: true,
        locked: false,
        points: [
          { x: position.x, y: position.y },
          { x: position.x + 6, y: position.y + 8 }
        ]
      };
    case "point":
    default:
      return {
        id,
        type: "point",
        name: "New Point",
        position: basePosition,
        size: { x: 1, y: 1 },
        rotation: 0,
        tags: [],
        note: "",
        color: "#d8a72f",
        visible: true,
        locked: false
      };
  }
}

function normalizePatchResult(object: MapObject): MapObject {
  if (object.type !== "path") {
    const { points: _points, ...withoutPoints } = object;
    return withoutPoints;
  }

  return {
    ...object,
    points:
      object.points && object.points.length >= 2
        ? object.points
        : [
            { x: object.position.x, y: object.position.y },
            { x: object.position.x + 6, y: object.position.y + 8 }
          ]
  };
}
