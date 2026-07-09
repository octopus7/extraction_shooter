import { useEditorStore } from "../store/editorStore";

export default function StatusBar() {
  const camera = useEditorStore((state) => state.editorState.camera);
  const objects = useEditorStore((state) => state.mapData.objects);
  const selectedObjectId = useEditorStore(
    (state) => state.editorState.selectedObjectId
  );
  const autosaveStatus = useEditorStore((state) => state.autosaveStatus);
  const selectedObject = objects.find((object) => object.id === selectedObjectId);

  return (
    <footer className="status-bar">
      <span>Zoom {camera.zoom.toFixed(2)}x</span>
      <span>
        Camera {Math.round(camera.x)}, {Math.round(camera.y)}
      </span>
      <span>Objects {objects.length}</span>
      <span>Selected {selectedObject ? selectedObject.name : "None"}</span>
      <span className={`autosave-status ${autosaveStatus}`}>
        {formatAutosaveStatus(autosaveStatus)}
      </span>
    </footer>
  );
}

function formatAutosaveStatus(status: string): string {
  switch (status) {
    case "pending":
      return "Autosaving";
    case "saved":
      return "Autosaved";
    case "error":
      return "Autosave failed";
    case "idle":
    default:
      return "Ready";
  }
}
