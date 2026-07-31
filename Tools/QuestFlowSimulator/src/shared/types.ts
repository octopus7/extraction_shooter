export type ViewMode = "desktop" | "pro";

export interface Session {
  authenticated: boolean;
  authConfigured: boolean;
  subject?: string;
  displayName?: string;
}

export interface CatalogSummary {
  id: string;
  slug: string;
  title: string;
  visibility: "public" | "authenticated";
  datasetVersion?: string;
  description?: string;
}

export interface LocationNode {
  id: string;
  name: string;
  mapId?: string;
  xMeters: number;
  yMeters: number;
}

export interface QuestStep {
  id: string;
  questId: string;
  questTitle: string;
  name: string;
  fromLocationId: string;
  toLocationId: string;
  actionSeconds: number;
  actionVariancePercent: number;
  moveSpeedMps: number;
  enabled: boolean;
}

export interface QuestDataset {
  schemaVersion: number;
  title: string;
  locations: LocationNode[];
  steps: QuestStep[];
  settings: {
    runs: number;
    mapTransitionSeconds?: number;
  };
  sourceNotes?: string[];
}

export interface Workspace {
  id: string;
  title: string;
  catalogId: string;
  state: QuestDataset;
  revision: number;
  updatedAt?: string;
}

export interface SimulationOptions {
  iterations: number;
  seed: number;
}

export interface SimulationResult {
  iterations: number;
  averageSeconds: number;
  p10Seconds: number;
  p50Seconds: number;
  p90Seconds: number;
  minSeconds: number;
  maxSeconds: number;
  totalDistanceMeters: number;
  averageTravelSeconds: number;
  averageActionSeconds: number;
}

export interface SimulationRequest {
  dataset: QuestDataset;
  options: SimulationOptions;
}
