# Runtime actor placement

## Current ownership

World-space placement is level-authored by default. Level travel, extraction points, world-progress actors, warp points, transparent obstacles, Mole, and ordinary gameplay-interaction actors are placed as native actors or Blueprint instances in their levels. Loading runtime placement data never searches for or destroys an existing level actor.

Legacy generic-interaction, world-progress, warp, transparent-obstacle, and bunker-character spawn files have no loader, cache, delegate, build-flavor fallback, or runtime spawn path.

The remaining runtime placement files are:

| File | Spatial source | Runtime owner |
| --- | --- | --- |
| `EnemySpawns.json` | `BP_RaidPlacementAnchor` only | `UTunaSweeperRaidPlacementSubsystem` |
| `EnemySpawnProfiles.json` | no transform; enemy class/loadout/profile | `UTunaSweeperRaidPlacementSubsystem` |
| `EnemyCombatProfiles.json` | no transform; combat behavior | `UTunaSweeperEnemySpawnSubsystem` |
| `LootContainerSpawns.json` | anchor rows or retained coordinate rows | raid placement and enemy-spawn subsystems |
| `MemoSpawns.json` | `BP_RaidPlacementAnchor` only | `UTunaSweeperMemoSubsystem` |
| `MemoDefinitions.json` | no transform; memo content | `UTunaSweeperMemoSubsystem` |

## Direct level actors

- `BP_Interact_LevelTravel` owns fixed entrances and return routes.
- `BP_ExtractionPoint` owns extraction radius, hold time, visual effect, and bunker destination.
- The map widget discovers directly placed level-travel actors whose destination is `Bunker` and directly placed extraction actors. Their actor transforms drive the start/extraction overlays.
- Directly placed world-progress actors keep their stable progress ids and restore state through the existing save flow.
- `BP_Mole` remains directly placed in `BunkerMap`; its dialogue, quest provider, and quest state are unchanged.
- Directly placed memo actors remain supported independently of `MemoSpawns.json`.

## Enemy placement

Every `EnemySpawns.json` row requires `level_name`, positive `placement_id`, and `profile_id`. It may contain chance/condition data, but must not contain `location`, `rotation`, or `scale`. A matching level instance of `/Game/Raid/Placement/BP_RaidPlacementAnchor` with kind `Enemy` owns the transform.

The demo row uses logical level `DemoRaidMap` and `placement_id=1`. Both demo raid maps contain a matching anchor, so the build-flavor level alias can select either map without falling back to coordinates. `EnemySpawnProfiles.json` resolves the actor/loadout, while `EnemyCombatProfiles.json` remains the source of combat behavior.

Coordinate-authored enemy rows are invalid and cause placement-data validation to fail.

## Loot-container placement

`LootContainerSpawns.json` is intentionally retained. A row with `placement_id` is anchor-owned and must not include transform fields. Existing rows without `placement_id` may continue to use `location` and optional `rotation`; this is the only retained coordinate-authored runtime placement path.

Anchor rows use kind `LootContainer`. Container class, definition id, contents id, chance, and condition remain JSON-owned.

## Memo placement

Every `MemoSpawns.json` row requires:

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 301,
  "memo_id": 1
}
```

The matching anchor uses kind `Memo` and owns the transform. JSON may additionally select `actor_class`, `marker_widget_class`, `visual_mesh`, `visual_material`, `visual_scale`, `visual_relative_location`, and `interaction_display_name`. Transform fields are rejected.

Validation rejects duplicate `(level_name, placement_id)`, duplicate memo ids within a level, missing memo definitions, missing anchors, duplicate anchor ids, and anchor-kind mismatches. `MemoDefinitions.json` continues to own memo content. `AcquiredMemoIds` remains the save/runtime collection gate, so acquired memos do not respawn.

## Editor authoring

Place `/Game/Raid/Placement/BP_RaidPlacementAnchor`, choose `Enemy`, `Loot Container`, or `Memo` in the Details panel, and assign a stable positive `PlacementId` unique across all three kinds in that level. The arrow, billboard, label, and loot preview are editor-only. Runtime actors spawn at the anchor transform and do not use preview components as gameplay authority.

`Tools/RaidPlacement/place_quadruped_enemy_spawn.py` maintains the demo quadruped anchor in both demo raid maps without removing directly placed actors.
