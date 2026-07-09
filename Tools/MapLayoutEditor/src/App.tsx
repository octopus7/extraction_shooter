import { useEffect, useRef } from "react";
import MapCanvas from "./components/MapCanvas";
import PropertiesPanel from "./components/PropertiesPanel";
import StatusBar from "./components/StatusBar";
import Toolbar from "./components/Toolbar";
import { useEditorStore } from "./store/editorStore";
import { createProjectSaveData } from "./utils/exportImport";
import { saveAutosave } from "./utils/storage";

export default function App() {
  const initialized = useRef(false);
  const initializeProject = useEditorStore((state) => state.initializeProject);
  const hasLoaded = useEditorStore((state) => state.hasLoaded);
  const mapData = useEditorStore((state) => state.mapData);
  const editorState = useEditorStore((state) => state.editorState);
  const setAutosaveStatus = useEditorStore((state) => state.setAutosaveStatus);

  useEffect(() => {
    if (initialized.current) {
      return;
    }

    initialized.current = true;
    initializeProject();
  }, [initializeProject]);

  useEffect(() => {
    if (!hasLoaded) {
      return;
    }

    setAutosaveStatus("pending");
    const handle = window.setTimeout(() => {
      try {
        saveAutosave(createProjectSaveData(mapData, editorState));
        setAutosaveStatus("saved");
      } catch (error) {
        console.warn("Failed to autosave map layout project.", error);
        setAutosaveStatus("error");
      }
    }, 300);

    return () => window.clearTimeout(handle);
  }, [editorState, hasLoaded, mapData, setAutosaveStatus]);

  return (
    <main className="app-shell">
      <Toolbar />
      <section className="workspace-shell">
        <div className="canvas-column">
          <MapCanvas />
          <StatusBar />
        </div>
        <PropertiesPanel />
      </section>
    </main>
  );
}
