import Konva from "konva";
import { Circle, Group, Line, Rect } from "react-konva";
import type { MapObject, Vector2 } from "../types";
import { useEditorStore } from "../store/editorStore";
import {
  screenToWorld,
  worldDeltaToScreen,
  worldToScreen
} from "../utils/coordinates";

export default function ObjectsLayer() {
  const objects = useEditorStore((state) => state.mapData.objects);
  const camera = useEditorStore((state) => state.editorState.camera);
  const selectedObjectId = useEditorStore(
    (state) => state.editorState.selectedObjectId
  );
  const selectObject = useEditorStore((state) => state.selectObject);
  const moveObject = useEditorStore((state) => state.moveObject);

  return (
    <>
      {objects
        .filter((object) => object.visible)
        .map((object) => {
          const screenPosition = worldToScreen(object.position, camera);
          const selected = object.id === selectedObjectId;

          return (
            <Group
              key={object.id}
              x={screenPosition.x}
              y={screenPosition.y}
              draggable={!object.locked}
              onClick={(event) => {
                event.cancelBubble = true;
                selectObject(object.id);
              }}
              onTap={(event) => {
                event.cancelBubble = true;
                selectObject(object.id);
              }}
              onDragStart={(event) => {
                event.cancelBubble = true;
                selectObject(object.id);
              }}
              onDragEnd={(event) => {
                const target = event.target;
                const nextWorld = screenToWorld(
                  { x: target.x(), y: target.y() },
                  camera
                );
                moveObject(object.id, nextWorld);
              }}
            >
              {renderObject(object, selected, camera.zoom)}
            </Group>
          );
        })}
    </>
  );
}

function renderObject(object: MapObject, selected: boolean, zoom: number) {
  switch (object.type) {
    case "rect":
      return renderRect(object, selected, zoom);
    case "path":
      return renderPath(object, selected, zoom);
    case "point":
    default:
      return renderPoint(object, selected, zoom);
  }
}

function renderPoint(object: MapObject, selected: boolean, zoom: number) {
  const radius = Math.max(5, (Math.max(object.size.x, object.size.y) * zoom) / 2);

  return (
    <>
      <Circle
        radius={radius}
        fill={object.color}
        stroke={selected ? "#ffffff" : "rgba(20, 20, 20, 0.82)"}
        strokeWidth={selected ? 3 : 1.5}
        shadowColor="black"
        shadowOpacity={selected ? 0.35 : 0.18}
        shadowBlur={selected ? 12 : 4}
      />
      <Circle radius={Math.max(2, radius * 0.28)} fill="rgba(255,255,255,0.75)" />
    </>
  );
}

function renderRect(object: MapObject, selected: boolean, zoom: number) {
  const width = Math.max(4, object.size.y * zoom);
  const height = Math.max(4, object.size.x * zoom);

  return (
    <Rect
      x={-width / 2}
      y={-height / 2}
      width={width}
      height={height}
      rotation={object.rotation}
      fill={`${object.color}99`}
      stroke={selected ? "#ffffff" : object.color}
      strokeWidth={selected ? 3 : 2}
      dash={object.locked ? [8, 5] : undefined}
      cornerRadius={2}
      shadowColor="black"
      shadowOpacity={selected ? 0.28 : 0.12}
      shadowBlur={selected ? 12 : 4}
    />
  );
}

function renderPath(object: MapObject, selected: boolean, zoom: number) {
  const points = object.points ?? [
    { x: object.position.x, y: object.position.y },
    { x: object.position.x + 6, y: object.position.y + 8 }
  ];
  const linePoints = points.flatMap((point) =>
    pointToRelativeScreen(point, object.position, zoom)
  );

  return (
    <>
      <Line
        points={linePoints}
        stroke="rgba(0,0,0,0.4)"
        strokeWidth={selected ? 8 : 6}
        lineCap="round"
        lineJoin="round"
      />
      <Line
        points={linePoints}
        stroke={object.color}
        strokeWidth={selected ? 5 : 3}
        lineCap="round"
        lineJoin="round"
        dash={object.locked ? [10, 6] : undefined}
      />
      {selected &&
        points.map((point, index) => {
          const [x, y] = pointToRelativeScreen(point, object.position, zoom);
          return (
            <Circle
              key={`${object.id}-point-${index}`}
              x={x}
              y={y}
              radius={5}
              fill="#ffffff"
              stroke={object.color}
              strokeWidth={2}
              listening={false}
            />
          );
        })}
    </>
  );
}

function pointToRelativeScreen(
  point: Vector2,
  origin: MapObject["position"],
  zoom: number
): [number, number] {
  const delta = {
    x: point.x - origin.x,
    y: point.y - origin.y
  };
  const screen = worldDeltaToScreen(delta, { x: 0, y: 0, zoom });
  return [screen.x, screen.y];
}
