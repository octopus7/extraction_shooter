import {
  Download,
  Eye,
  EyeOff,
  MapPinPlus,
  MousePointer2,
  RefreshCcw,
  RotateCcw,
  Route,
  SquarePlus,
  Trash2,
  Upload
} from "lucide-react";
import { useRef } from "react";
import type { EditorTool } from "../types";
import { useEditorStore } from "../store/editorStore";
import { downloadProjectJson, readProjectFile } from "../utils/exportImport";

const toolButtons: Array<{
  tool: EditorTool;
  label: string;
  title: string;
  icon: typeof MousePointer2;
}> = [
  {
    tool: "select",
    label: "Select",
    title: "Select and pan",
    icon: MousePointer2
  },
  {
    tool: "add-point",
    label: "Point",
    title: "Add point",
    icon: MapPinPlus
  },
  {
    tool: "add-rect",
    label: "Rect",
    title: "Add rectangular area",
    icon: SquarePlus
  },
  {
    tool: "add-path",
    label: "Path",
    title: "Add path",
    icon: Route
  }
];

export default function Toolbar() {
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const activeTool = useEditorStore((state) => state.editorState.activeTool);
  const showLabels = useEditorStore((state) => state.editorState.showLabels);
  const selectedObjectId = useEditorStore(
    (state) => state.editorState.selectedObjectId
  );
  const selectedObject = useEditorStore((state) =>
    state.mapData.objects.find((object) => object.id === selectedObjectId)
  );
  const setActiveTool = useEditorStore((state) => state.setActiveTool);
  const deleteSelectedObject = useEditorStore(
    (state) => state.deleteSelectedObject
  );
  const toggleLabels = useEditorStore((state) => state.toggleLabels);
  const resetView = useEditorStore((state) => state.resetView);
  const resetProject = useEditorStore((state) => state.resetProject);
  const importProject = useEditorStore((state) => state.importProject);
  const getProjectSaveData = useEditorStore((state) => state.getProjectSaveData);

  const canDelete = Boolean(selectedObject && !selectedObject.locked);
  const LabelIcon = showLabels ? Eye : EyeOff;

  function handleExport() {
    downloadProjectJson(getProjectSaveData());
  }

  async function handleImport(file: File | undefined) {
    if (!file) {
      return;
    }

    try {
      importProject(await readProjectFile(file));
    } catch (error) {
      console.warn("Failed to import map layout project.", error);
      window.alert("JSON 파일을 불러오지 못했습니다.");
    } finally {
      if (fileInputRef.current) {
        fileInputRef.current.value = "";
      }
    }
  }

  function handleResetProject() {
    if (window.confirm("현재 작업을 초기 샘플 데이터로 되돌릴까요?")) {
      resetProject();
    }
  }

  return (
    <header className="toolbar">
      <div className="toolbar-title">
        <strong>TunaSweeper Map Layout</strong>
        <span>meter / +X north / +Y right</span>
      </div>

      <div className="toolbar-group" role="toolbar" aria-label="Editor tools">
        {toolButtons.map(({ tool, label, title, icon: Icon }) => (
          <button
            key={tool}
            type="button"
            className={activeTool === tool ? "tool-button is-active" : "tool-button"}
            title={title}
            aria-label={title}
            onClick={() => setActiveTool(tool)}
          >
            <Icon size={17} />
            <span>{label}</span>
          </button>
        ))}
      </div>

      <div className="toolbar-group" role="toolbar" aria-label="Project actions">
        <button
          type="button"
          className="tool-button"
          title="Delete selected"
          aria-label="Delete selected"
          disabled={!canDelete}
          onClick={deleteSelectedObject}
        >
          <Trash2 size={17} />
          <span>Delete</span>
        </button>

        <button
          type="button"
          className="tool-button"
          title="Toggle labels"
          aria-label="Toggle labels"
          onClick={toggleLabels}
        >
          <LabelIcon size={17} />
          <span>Labels</span>
        </button>

        <button
          type="button"
          className="tool-button"
          title="Export JSON"
          aria-label="Export JSON"
          onClick={handleExport}
        >
          <Download size={17} />
          <span>Export</span>
        </button>

        <button
          type="button"
          className="tool-button"
          title="Import JSON"
          aria-label="Import JSON"
          onClick={() => fileInputRef.current?.click()}
        >
          <Upload size={17} />
          <span>Import</span>
        </button>

        <button
          type="button"
          className="tool-button"
          title="Reset view"
          aria-label="Reset view"
          onClick={resetView}
        >
          <RotateCcw size={17} />
          <span>View</span>
        </button>

        <button
          type="button"
          className="tool-button danger"
          title="Reset project"
          aria-label="Reset project"
          onClick={handleResetProject}
        >
          <RefreshCcw size={17} />
          <span>Reset</span>
        </button>
      </div>

      <input
        ref={fileInputRef}
        type="file"
        accept="application/json,.json"
        className="hidden-file-input"
        onChange={(event) => void handleImport(event.target.files?.[0])}
      />
    </header>
  );
}
