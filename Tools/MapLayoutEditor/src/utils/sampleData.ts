import type { EditorState, MapData, ProjectSaveData } from "../types";
import { PROJECT_SAVE_VERSION } from "../types";

export const DEFAULT_EDITOR_STATE: EditorState = {
  camera: {
    x: 520,
    y: 360,
    zoom: 22
  },
  showLabels: true,
  selectedObjectId: null,
  activeTool: "select"
};

export const SAMPLE_MAP_DATA: MapData = {
  objects: [
    {
      id: "bunker_entrance",
      type: "rect",
      name: "벙커 입구",
      position: { x: 0, y: 0, z: 0 },
      size: { x: 8, y: 10 },
      rotation: 0,
      tags: ["bunker", "entry"],
      note: "BunkerMap 기준 출입구 후보",
      color: "#3f7d5f",
      visible: true,
      locked: false
    },
    {
      id: "water_facility",
      type: "rect",
      name: "취수시설",
      position: { x: 28, y: 18, z: 0 },
      size: { x: 12, y: 16 },
      rotation: 8,
      tags: ["landmark", "facility"],
      note: "초반 시야 기준점으로 쓰는 시설 구역",
      color: "#427da3",
      visible: true,
      locked: false
    },
    {
      id: "spawn_early_enemy",
      type: "point",
      name: "초반 몬스터 스폰",
      position: { x: 15, y: -12, z: 0 },
      size: { x: 1.2, y: 1.2 },
      rotation: 0,
      tags: ["enemy", "early"],
      note: "플레이어 진입 후 3초 뒤 활성화 후보",
      color: "#d8a72f",
      visible: true,
      locked: false
    },
    {
      id: "extraction_east",
      type: "point",
      name: "탈출 지점",
      position: { x: 54, y: 42, z: 0 },
      size: { x: 1.6, y: 1.6 },
      rotation: 0,
      tags: ["extract", "east"],
      note: "레이드 종료 후보 지점",
      color: "#d45a4f",
      visible: true,
      locked: false
    },
    {
      id: "main_route",
      type: "path",
      name: "주요 이동 동선",
      position: { x: 0, y: 0, z: 0 },
      size: { x: 1, y: 1 },
      rotation: 0,
      tags: ["route", "player"],
      note: "벙커 입구에서 취수시설을 거쳐 탈출 지점으로 이어지는 1차 동선",
      color: "#d06f2f",
      visible: true,
      locked: false,
      points: [
        { x: 0, y: 0 },
        { x: 15, y: -4 },
        { x: 29, y: 18 },
        { x: 44, y: 31 },
        { x: 54, y: 42 }
      ]
    },
    {
      id: "danger_marsh",
      type: "rect",
      name: "위험 구역",
      position: { x: 36, y: -24, z: 0 },
      size: { x: 18, y: 20 },
      rotation: -14,
      tags: ["danger", "noise"],
      note: "소음 발생기나 매복을 배치할 후보 구역",
      color: "#8a4f8f",
      visible: true,
      locked: false
    }
  ]
};

export function createSampleProjectData(): ProjectSaveData {
  return {
    version: PROJECT_SAVE_VERSION,
    unit: "meter",
    mapData: structuredClone(SAMPLE_MAP_DATA),
    editorState: structuredClone(DEFAULT_EDITOR_STATE)
  };
}
