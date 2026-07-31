import type {
  MapPlace,
  PlaceShape,
  QuestDataset,
  QuestNode,
  QuestStep,
} from "./types";

type LegacyLocation = {
  id: string;
  name: string;
  mapId?: string;
  xMeters: number;
  yMeters: number;
};

type LegacyStep = Omit<QuestStep, "fromPlaceId" | "toPlaceId"> & {
  fromLocationId?: string;
  toLocationId?: string;
  fromPlaceId?: string;
  toPlaceId?: string;
};

export function normalizeQuestDataset(value: unknown): QuestDataset | null {
  if (!value || typeof value !== "object") return null;
  const source = value as Record<string, unknown>;
  const rawSteps = Array.isArray(source.steps) ? source.steps : [];
  const steps = rawSteps
    .map((step) => normalizeStep(step))
    .filter((step): step is QuestStep => step !== null);

  if (Array.isArray(source.questNodes) && Array.isArray(source.places)) {
    return {
      schemaVersion: 2,
      title: typeof source.title === "string" ? source.title : "Quest Flow",
      questNodes: source.questNodes
        .map((node, index) => normalizeQuestNode(node, index))
        .filter((node): node is QuestNode => node !== null),
      places: source.places
        .map((place) => normalizePlace(place))
        .filter((place): place is MapPlace => place !== null),
      steps,
      settings: normalizeSettings(source.settings),
      sourceNotes: Array.isArray(source.sourceNotes)
        ? source.sourceNotes.filter((note): note is string => typeof note === "string")
        : undefined,
    };
  }

  const legacyLocations = Array.isArray(source.locations)
    ? source.locations
        .map((location) => normalizeLegacyLocation(location))
        .filter((location): location is LegacyLocation => location !== null)
    : [];
  if (legacyLocations.length === 0) return null;

  const syntheticLocations = legacyLocations.some(
    (location) =>
      location.mapId === "approximation" ||
      /^location-[a-z0-9_-]+$/i.test(location.id),
  );
  const questNodes = deriveQuestNodes(steps, legacyLocations);
  const places = syntheticLocations
    ? [
        {
          id: "legacy-inferred-origin",
          name: "임시 기준 시작점",
          mapId: "inferred",
          shape: "point" as const,
          xMeters: 0,
          yMeters: 0,
          actorId: "inferred.origin",
        },
      ]
    : legacyLocations.map((location) => ({
        ...location,
        shape: "point" as const,
      }));

  return {
    schemaVersion: 2,
    title: typeof source.title === "string" ? source.title : "Quest Flow",
    questNodes,
    places,
    steps: syntheticLocations
      ? steps.map((step) => ({
          ...step,
          fromPlaceId: "legacy-inferred-origin",
          toPlaceId: "legacy-inferred-origin",
        }))
      : steps,
    settings: normalizeSettings(source.settings),
    sourceNotes: [
      "구버전 catalog를 호환 변환했습니다. 실제 맵 장소 데이터가 없으므로 임시 기준점만 사용합니다.",
      ...(Array.isArray(source.sourceNotes)
        ? source.sourceNotes.filter((note): note is string => typeof note === "string")
        : []),
    ],
  };
}

function normalizeQuestNode(value: unknown, index: number): QuestNode | null {
  if (!value || typeof value !== "object") return null;
  const source = value as Record<string, unknown>;
  const questId = String(source.questId ?? source.id ?? "").trim();
  if (!questId) return null;
  return {
    id: String(source.id ?? `quest-node-${questId}`).trim(),
    questId,
    title: String(source.title ?? source.questTitle ?? questId),
    x: finiteNumber(source.x, 100 + (index % 5) * 180),
    y: finiteNumber(source.y, 80 + Math.floor(index / 5) * 110),
    prerequisiteQuestIds: Array.isArray(source.prerequisiteQuestIds)
      ? source.prerequisiteQuestIds.filter(
          (id): id is string => typeof id === "string" && id.length > 0,
        )
      : [],
  };
}

function normalizePlace(value: unknown): MapPlace | null {
  if (!value || typeof value !== "object") return null;
  const source = value as Record<string, unknown>;
  const id = String(source.id ?? "").trim();
  if (!id) return null;
  const shape = normalizeShape(source.shape);
  return {
    id,
    name: String(source.name ?? id),
    mapId: typeof source.mapId === "string" ? source.mapId : undefined,
    shape,
    xMeters: finiteNumber(source.xMeters, 0),
    yMeters: finiteNumber(source.yMeters, 0),
    radiusMeters: finiteOptionalNumber(source.radiusMeters),
    widthMeters: finiteOptionalNumber(source.widthMeters),
    heightMeters: finiteOptionalNumber(source.heightMeters),
    actorId: typeof source.actorId === "string" ? source.actorId : undefined,
  };
}

function normalizeStep(value: unknown): QuestStep | null {
  if (!value || typeof value !== "object") return null;
  const source = value as LegacyStep;
  const fromPlaceId = String(
    source.fromPlaceId ?? source.fromLocationId ?? "",
  ).trim();
  const toPlaceId = String(source.toPlaceId ?? source.toLocationId ?? "").trim();
  const questId = String(source.questId ?? "").trim();
  if (!fromPlaceId || !toPlaceId || !questId) return null;
  return {
    id: String(source.id ?? `step-${questId}`),
    questId,
    questTitle: String(source.questTitle ?? questId),
    name: String(source.name ?? source.questTitle ?? questId),
    fromPlaceId,
    toPlaceId,
    actionSeconds: finiteNumber(source.actionSeconds, 0),
    actionVariancePercent: finiteNumber(source.actionVariancePercent, 0),
    moveSpeedMps: Math.max(0.1, finiteNumber(source.moveSpeedMps, 4)),
    enabled: source.enabled !== false,
  };
}

function normalizeLegacyLocation(value: unknown): LegacyLocation | null {
  if (!value || typeof value !== "object") return null;
  const source = value as Record<string, unknown>;
  const id = String(source.id ?? "").trim();
  if (!id) return null;
  return {
    id,
    name: String(source.name ?? id),
    mapId: typeof source.mapId === "string" ? source.mapId : undefined,
    xMeters: finiteNumber(source.xMeters, 0),
    yMeters: finiteNumber(source.yMeters, 0),
  };
}

function deriveQuestNodes(
  steps: QuestStep[],
  legacyLocations: LegacyLocation[],
): QuestNode[] {
  const byQuest = new Map<string, QuestStep>();
  for (const step of steps) {
    if (!byQuest.has(step.questId)) byQuest.set(step.questId, step);
  }
  const questIds = [...byQuest.keys()];
  return [...byQuest.values()].map((step, index) => {
    const previousQuestId = questIds[index - 1];
    const legacyLocation = legacyLocations.find(
      (location) => location.id === step.toPlaceId,
    );
    return {
      id: `quest-node-${step.questId}`,
      questId: step.questId,
      title: step.questTitle,
      x: legacyLocation?.xMeters ?? 100 + (index % 5) * 180,
      y: legacyLocation?.yMeters ?? 80 + Math.floor(index / 5) * 110,
      prerequisiteQuestIds: previousQuestId ? [previousQuestId] : [],
    };
  });
}

function normalizeSettings(value: unknown): QuestDataset["settings"] {
  const source = value && typeof value === "object"
    ? value as Record<string, unknown>
    : {};
  return {
    runs: Math.max(1, Math.floor(finiteNumber(source.runs, 1_000))),
    mapTransitionSeconds: Math.max(
      0,
      finiteNumber(source.mapTransitionSeconds, 0),
    ),
  };
}

function normalizeShape(value: unknown): PlaceShape {
  return value === "circle" || value === "rectangle" ? value : "point";
}

function finiteNumber(value: unknown, fallback: number): number {
  return typeof value === "number" && Number.isFinite(value) ? value : fallback;
}

function finiteOptionalNumber(value: unknown): number | undefined {
  return typeof value === "number" && Number.isFinite(value) ? value : undefined;
}
