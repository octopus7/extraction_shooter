import Konva from "konva";
import { useCallback, useEffect, useRef, useState } from "react";
import { Layer, Stage } from "react-konva";
import { useEditorStore } from "../store/editorStore";
import {
  screenToWorld,
  zoomCameraAtScreenPoint
} from "../utils/coordinates";
import GridLayer from "./GridLayer";
import LabelsLayer from "./LabelsLayer";
import ObjectsLayer from "./ObjectsLayer";

interface CanvasSize {
  width: number;
  height: number;
}

interface PanState {
  pointer: { x: number; y: number };
}

export default function MapCanvas() {
  const shellRef = useRef<HTMLDivElement | null>(null);
  const stageRef = useRef<Konva.Stage | null>(null);
  const panState = useRef<PanState | null>(null);
  const [size, setSize] = useState<CanvasSize>({ width: 800, height: 600 });

  const camera = useEditorStore((state) => state.editorState.camera);
  const activeTool = useEditorStore((state) => state.editorState.activeTool);
  const setCamera = useEditorStore((state) => state.setCamera);
  const panCamera = useEditorStore((state) => state.panCamera);
  const addObject = useEditorStore((state) => state.addObject);
  const selectObject = useEditorStore((state) => state.selectObject);
  const deleteSelectedObject = useEditorStore(
    (state) => state.deleteSelectedObject
  );

  useEffect(() => {
    const shell = shellRef.current;
    if (!shell) {
      return;
    }

    const observer = new ResizeObserver(([entry]) => {
      const rect = entry.contentRect;
      setSize({
        width: Math.max(320, rect.width),
        height: Math.max(240, rect.height)
      });
    });

    observer.observe(shell);
    return () => observer.disconnect();
  }, []);

  useEffect(() => {
    const handleKeyDown = (event: KeyboardEvent) => {
      if (event.key === "Delete" || event.key === "Backspace") {
        const target = event.target as HTMLElement | null;
        const isTyping =
          target?.tagName === "INPUT" ||
          target?.tagName === "TEXTAREA" ||
          target?.tagName === "SELECT";

        if (!isTyping) {
          deleteSelectedObject();
        }
      }
    };

    window.addEventListener("keydown", handleKeyDown);
    return () => window.removeEventListener("keydown", handleKeyDown);
  }, [deleteSelectedObject]);

  const getPointer = useCallback(() => {
    const stage = stageRef.current;
    const pointer = stage?.getPointerPosition();
    return pointer ? { x: pointer.x, y: pointer.y } : null;
  }, []);

  function handleWheel(event: Konva.KonvaEventObject<WheelEvent>) {
    event.evt.preventDefault();
    const pointer = getPointer();
    if (!pointer) {
      return;
    }

    const scaleBy = 1.08;
    const nextZoom =
      event.evt.deltaY > 0 ? camera.zoom / scaleBy : camera.zoom * scaleBy;
    setCamera(zoomCameraAtScreenPoint(camera, pointer, nextZoom));
  }

  function handlePointerDown(event: Konva.KonvaEventObject<PointerEvent>) {
    const pointer = getPointer();
    if (!pointer) {
      return;
    }

    const clickedStage = event.target === event.target.getStage();
    const isMiddleMouse = event.evt.button === 1;
    const canPan = isMiddleMouse || (activeTool === "select" && clickedStage);

    if (canPan) {
      panState.current = { pointer };
      event.evt.preventDefault();
    }
  }

  function handlePointerMove() {
    const pointer = getPointer();
    if (!pointer || !panState.current) {
      return;
    }

    const previous = panState.current.pointer;
    panCamera({
      x: pointer.x - previous.x,
      y: pointer.y - previous.y
    });
    panState.current = { pointer };
  }

  function handlePointerUp() {
    panState.current = null;
  }

  function handleClick(event: Konva.KonvaEventObject<MouseEvent>) {
    if (event.target !== event.target.getStage()) {
      return;
    }

    const pointer = getPointer();
    if (!pointer) {
      return;
    }

    if (activeTool === "select") {
      selectObject(null);
      return;
    }

    addObject(toolToObjectType(activeTool), screenToWorld(pointer, camera));
  }

  const cursor =
    activeTool === "select"
      ? panState.current
        ? "grabbing"
        : "default"
      : "crosshair";

  return (
    <div ref={shellRef} className="map-canvas-shell" style={{ cursor }}>
      <Stage
        ref={stageRef}
        width={size.width}
        height={size.height}
        onWheel={handleWheel}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerCancel={handlePointerUp}
        onClick={handleClick}
      >
        <Layer listening={false}>
          <GridLayer width={size.width} height={size.height} />
        </Layer>
        <Layer>
          <ObjectsLayer />
        </Layer>
        <Layer listening={false}>
          <LabelsLayer />
        </Layer>
      </Stage>
    </div>
  );
}

function toolToObjectType(tool: string) {
  switch (tool) {
    case "add-rect":
      return "rect";
    case "add-path":
      return "path";
    case "add-point":
    default:
      return "point";
  }
}
