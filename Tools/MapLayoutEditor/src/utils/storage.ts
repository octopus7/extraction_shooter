import type { ProjectSaveData } from "../types";
import { normalizeProjectSaveData } from "./exportImport";

export const AUTOSAVE_KEY = "ue5-map-layout-editor-autosave";

export function loadAutosave(): ProjectSaveData | null {
  if (!isLocalStorageAvailable()) {
    return null;
  }

  try {
    const raw = window.localStorage.getItem(AUTOSAVE_KEY);
    return raw ? normalizeProjectSaveData(JSON.parse(raw)) : null;
  } catch (error) {
    console.warn("Failed to load map layout autosave.", error);
    return null;
  }
}

export function saveAutosave(project: ProjectSaveData): void {
  if (!isLocalStorageAvailable()) {
    return;
  }

  window.localStorage.setItem(AUTOSAVE_KEY, JSON.stringify(project));
}

export function clearAutosave(): void {
  if (!isLocalStorageAvailable()) {
    return;
  }

  window.localStorage.removeItem(AUTOSAVE_KEY);
}

function isLocalStorageAvailable(): boolean {
  return typeof window !== "undefined" && "localStorage" in window;
}
