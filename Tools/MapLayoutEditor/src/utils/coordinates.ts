import type { CameraState, Vector2, Vector3 } from "../types";
import { METER_TO_UNREAL_UNIT } from "../types";

export const MIN_ZOOM = 0.1;
export const MAX_ZOOM = 8;

export function clampZoom(zoom: number): number {
  return Math.min(MAX_ZOOM, Math.max(MIN_ZOOM, zoom));
}

export function worldToScreen(world: Vector2, camera: CameraState): Vector2 {
  return {
    x: world.y * camera.zoom + camera.x,
    y: -world.x * camera.zoom + camera.y
  };
}

export function screenToWorld(screen: Vector2, camera: CameraState): Vector2 {
  return {
    x: (camera.y - screen.y) / camera.zoom,
    y: (screen.x - camera.x) / camera.zoom
  };
}

export function zoomCameraAtScreenPoint(
  camera: CameraState,
  pointer: Vector2,
  nextZoom: number
): CameraState {
  const zoom = clampZoom(nextZoom);
  const worldPoint = screenToWorld(pointer, camera);

  return {
    zoom,
    x: pointer.x - worldPoint.y * zoom,
    y: pointer.y + worldPoint.x * zoom
  };
}

export function worldDeltaToScreen(delta: Vector2, camera: CameraState): Vector2 {
  return {
    x: delta.y * camera.zoom,
    y: -delta.x * camera.zoom
  };
}

export function getWorldBounds(
  width: number,
  height: number,
  camera: CameraState
): { minX: number; maxX: number; minY: number; maxY: number } {
  const corners = [
    screenToWorld({ x: 0, y: 0 }, camera),
    screenToWorld({ x: width, y: 0 }, camera),
    screenToWorld({ x: 0, y: height }, camera),
    screenToWorld({ x: width, y: height }, camera)
  ];

  return {
    minX: Math.min(...corners.map((corner) => corner.x)),
    maxX: Math.max(...corners.map((corner) => corner.x)),
    minY: Math.min(...corners.map((corner) => corner.y)),
    maxY: Math.max(...corners.map((corner) => corner.y))
  };
}

export function getGridStep(zoom: number): number {
  const targetPx = 86;
  const rawStep = targetPx / zoom;
  const exponent = Math.floor(Math.log10(rawStep));
  const base = 10 ** exponent;
  const normalized = rawStep / base;

  if (normalized <= 1) {
    return base;
  }

  if (normalized <= 2) {
    return base * 2;
  }

  if (normalized <= 5) {
    return base * 5;
  }

  return base * 10;
}

export function worldToUnreal(world: Vector3): Vector3 {
  return {
    x: world.x * METER_TO_UNREAL_UNIT,
    y: world.y * METER_TO_UNREAL_UNIT,
    z: world.z * METER_TO_UNREAL_UNIT
  };
}
