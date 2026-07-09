import { Line, Text } from "react-konva";
import { useEditorStore } from "../store/editorStore";
import { getGridStep, getWorldBounds, worldToScreen } from "../utils/coordinates";

interface GridLayerProps {
  width: number;
  height: number;
}

export default function GridLayer({ width, height }: GridLayerProps) {
  const camera = useEditorStore((state) => state.editorState.camera);
  const bounds = getWorldBounds(width, height, camera);
  const step = getGridStep(camera.zoom);
  const fineStroke = "rgba(230, 235, 224, 0.12)";
  const majorStroke = "rgba(240, 232, 200, 0.26)";
  const axisStroke = "rgba(255, 197, 92, 0.65)";
  const lines = [];

  const startX = Math.floor(bounds.minX / step) * step;
  const endX = Math.ceil(bounds.maxX / step) * step;
  const startY = Math.floor(bounds.minY / step) * step;
  const endY = Math.ceil(bounds.maxY / step) * step;

  for (let x = startX; x <= endX; x += step) {
    const screen = worldToScreen({ x, y: 0 }, camera);
    const isAxis = Math.abs(x) < 0.0001;
    const isMajor = Math.abs(Math.round(x / step)) % 5 === 0;

    lines.push(
      <Line
        key={`x-${x}`}
        points={[0, screen.y, width, screen.y]}
        stroke={isAxis ? axisStroke : isMajor ? majorStroke : fineStroke}
        strokeWidth={isAxis ? 2 : 1}
      />
    );
  }

  for (let y = startY; y <= endY; y += step) {
    const screen = worldToScreen({ x: 0, y }, camera);
    const isAxis = Math.abs(y) < 0.0001;
    const isMajor = Math.abs(Math.round(y / step)) % 5 === 0;

    lines.push(
      <Line
        key={`y-${y}`}
        points={[screen.x, 0, screen.x, height]}
        stroke={isAxis ? axisStroke : isMajor ? majorStroke : fineStroke}
        strokeWidth={isAxis ? 2 : 1}
      />
    );
  }

  return (
    <>
      {lines}
      <Text
        x={12}
        y={12}
        text={`Grid ${formatStep(step)}m`}
        fill="rgba(244, 241, 229, 0.72)"
        fontSize={12}
      />
    </>
  );
}

function formatStep(step: number): string {
  return step >= 1 ? step.toFixed(0) : step.toFixed(1);
}
