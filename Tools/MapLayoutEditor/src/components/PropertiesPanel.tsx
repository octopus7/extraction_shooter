import type { ChangeEvent } from "react";
import type { MapObject, MapObjectType } from "../types";
import { useEditorStore } from "../store/editorStore";
import { worldToUnreal } from "../utils/coordinates";

export default function PropertiesPanel() {
  const objects = useEditorStore((state) => state.mapData.objects);
  const selectedObjectId = useEditorStore(
    (state) => state.editorState.selectedObjectId
  );
  const selectObject = useEditorStore((state) => state.selectObject);
  const updateObject = useEditorStore((state) => state.updateObject);

  const selectedObject =
    objects.find((object) => object.id === selectedObjectId) ?? null;

  return (
    <aside className="properties-panel">
      <div className="panel-section panel-heading">
        <div>
          <h2>Properties</h2>
          <p>{selectedObject ? selectedObject.id : "선택 없음"}</p>
        </div>
      </div>

      {selectedObject ? (
        <ObjectFields
          object={selectedObject}
          onChange={(patch) => updateObject(selectedObject.id, patch)}
        />
      ) : (
        <div className="empty-state">오브젝트를 선택하세요.</div>
      )}

      <div className="panel-section object-list-section">
        <h3>Objects</h3>
        <div className="object-list">
          {objects.map((object) => (
            <button
              key={object.id}
              type="button"
              className={
                object.id === selectedObjectId
                  ? "object-list-item is-selected"
                  : "object-list-item"
              }
              onClick={() => selectObject(object.id)}
            >
              <span className="object-color" style={{ background: object.color }} />
              <span className="object-list-name">{object.name}</span>
              <span className="object-list-type">{object.type}</span>
            </button>
          ))}
        </div>
      </div>
    </aside>
  );
}

interface ObjectFieldsProps {
  object: MapObject;
  onChange: (patch: Partial<MapObject>) => void;
}

function ObjectFields({ object, onChange }: ObjectFieldsProps) {
  const locked = object.locked;
  const uePosition = worldToUnreal(object.position);

  function updatePosition(axis: "x" | "y" | "z", value: number) {
    onChange({
      position: {
        ...object.position,
        [axis]: value
      }
    });
  }

  function updateSize(axis: "x" | "y", value: number) {
    onChange({
      size: {
        ...object.size,
        [axis]: value
      }
    });
  }

  function updateType(type: MapObjectType) {
    if (type === object.type) {
      return;
    }

    onChange({
      type,
      points:
        type === "path"
          ? [
              { x: object.position.x, y: object.position.y },
              { x: object.position.x + 6, y: object.position.y + 8 }
            ]
          : undefined
    });
  }

  return (
    <div className="panel-section fields-section">
      <Field label="Name">
        <input
          type="text"
          value={object.name}
          onChange={(event) => onChange({ name: event.target.value })}
        />
      </Field>

      <Field label="Type">
        <select
          value={object.type}
          disabled={locked}
          onChange={(event) => updateType(event.target.value as MapObjectType)}
        >
          <option value="point">point</option>
          <option value="rect">rect</option>
          <option value="path">path</option>
        </select>
      </Field>

      <div className="field-grid">
        <Field label="X north">
          <NumberInput
            value={object.position.x}
            disabled={locked}
            onChange={(value) => updatePosition("x", value)}
          />
        </Field>
        <Field label="Y right">
          <NumberInput
            value={object.position.y}
            disabled={locked}
            onChange={(value) => updatePosition("y", value)}
          />
        </Field>
        <Field label="Z up">
          <NumberInput
            value={object.position.z}
            disabled={locked}
            onChange={(value) => updatePosition("z", value)}
          />
        </Field>
      </div>

      <div className="field-grid two">
        <Field label="Width Y">
          <NumberInput
            value={object.size.y}
            disabled={locked || object.type === "path"}
            min={0.1}
            onChange={(value) => updateSize("y", value)}
          />
        </Field>
        <Field label="Depth X">
          <NumberInput
            value={object.size.x}
            disabled={locked || object.type === "path"}
            min={0.1}
            onChange={(value) => updateSize("x", value)}
          />
        </Field>
      </div>

      <Field label="Rotation">
        <NumberInput
          value={object.rotation}
          disabled={locked || object.type === "path"}
          step={1}
          onChange={(value) => onChange({ rotation: value })}
        />
      </Field>

      <Field label="Tags">
        <input
          type="text"
          value={object.tags.join(", ")}
          onChange={(event) => onChange({ tags: parseTags(event.target.value) })}
        />
      </Field>

      <Field label="Note">
        <textarea
          rows={4}
          value={object.note}
          onChange={(event) => onChange({ note: event.target.value })}
        />
      </Field>

      <div className="field-grid two">
        <Field label="Color">
          <input
            type="color"
            value={object.color}
            onChange={(event) => onChange({ color: event.target.value })}
          />
        </Field>
        <Field label="Flags">
          <div className="check-row">
            <label>
              <input
                type="checkbox"
                checked={object.visible}
                onChange={(event) => onChange({ visible: event.target.checked })}
              />
              Visible
            </label>
            <label>
              <input
                type="checkbox"
                checked={object.locked}
                onChange={(event) => onChange({ locked: event.target.checked })}
              />
              Locked
            </label>
          </div>
        </Field>
      </div>

      <div className="ue-position">
        UE: X {uePosition.x.toFixed(0)} / Y {uePosition.y.toFixed(0)} / Z{" "}
        {uePosition.z.toFixed(0)}
      </div>
    </div>
  );
}

function Field({
  label,
  children
}: {
  label: string;
  children: React.ReactNode;
}) {
  return (
    <label className="field">
      <span>{label}</span>
      {children}
    </label>
  );
}

function NumberInput({
  value,
  disabled,
  min,
  step = 0.1,
  onChange
}: {
  value: number;
  disabled?: boolean;
  min?: number;
  step?: number;
  onChange: (value: number) => void;
}) {
  function handleChange(event: ChangeEvent<HTMLInputElement>) {
    const nextValue = Number(event.target.value);
    if (Number.isFinite(nextValue)) {
      onChange(nextValue);
    }
  }

  return (
    <input
      type="number"
      value={Number.isFinite(value) ? value : 0}
      disabled={disabled}
      min={min}
      step={step}
      onChange={handleChange}
    />
  );
}

function parseTags(value: string): string[] {
  return value
    .split(",")
    .map((tag) => tag.trim())
    .filter((tag) => tag.length > 0);
}
