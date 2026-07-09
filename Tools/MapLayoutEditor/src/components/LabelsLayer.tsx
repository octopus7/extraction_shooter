import { Label, Tag, Text } from "react-konva";
import { useEditorStore } from "../store/editorStore";
import { worldToScreen } from "../utils/coordinates";

export default function LabelsLayer() {
  const objects = useEditorStore((state) => state.mapData.objects);
  const camera = useEditorStore((state) => state.editorState.camera);
  const showLabels = useEditorStore((state) => state.editorState.showLabels);

  if (!showLabels) {
    return null;
  }

  return (
    <>
      {objects
        .filter((object) => object.visible)
        .map((object) => {
          const screen = worldToScreen(object.position, camera);
          const tagLine = object.tags.length > 0 ? object.tags.join(", ") : "untagged";
          const labelText = `${object.name}\n${tagLine}`;

          return (
            <Label key={`${object.id}-label`} x={screen.x + 12} y={screen.y - 18}>
              <Tag
                fill="rgba(22, 23, 21, 0.82)"
                stroke="rgba(255,255,255,0.16)"
                strokeWidth={1}
                cornerRadius={4}
              />
              <Text
                text={labelText}
                fontSize={13}
                lineHeight={1.22}
                padding={6}
                fill="#f5f1e6"
              />
            </Label>
          );
        })}
    </>
  );
}
