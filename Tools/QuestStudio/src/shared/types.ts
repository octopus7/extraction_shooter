export interface QuestDefinition {
  quest_id: number;
  title_string_key: string;
  description_string_key: string;
  required_completed_quest_ids: number[];
  authoring_tags: string[];
  [key: string]: unknown;
}
export interface QuestNode {
  definition: QuestDefinition;
  x: number;
  y: number;
  authoring: {
    prerequisitesStatus: 'confirmed' | 'unspecified';
    sourceReference?: string;
    [key: string]: unknown;
  };
  [key: string]: unknown;
}
export interface LocalizationEntry { key: string; locale: string; value: string; [key: string]: unknown }
export interface QuestPack { schemaVersion: 1; nodes: QuestNode[]; strings: LocalizationEntry[]; [key: string]: unknown }
export interface SnapshotMetadata { id: string; alias: string; memo: string; createdAt: string; nodeCount: number; stringCount: number }
export interface GraphProjection {
  nodes: { id: number; questId: number; x: number; y: number; external: boolean; titleKey: string }[];
  edges: { source: number; target: number }[];
}
