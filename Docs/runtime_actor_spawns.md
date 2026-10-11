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
| `MemoDefinitions.json` | no transform; memo content | `UTunaSweeperMemoSubsystem` |

## Direct level actors

- `BP_Interact_LevelTravel` owns fixed entrances and return routes.
- `BP_ExtractionPoint` owns extraction radius, hold time, visual effect, and bunker destination.
- The map widget discovers directly placed level-travel actors whose destination is `Bunker` and directly placed extraction actors. Their actor transforms drive the start/extraction overlays.
- Directly placed world-progress actors keep their stable progress ids and restore state through the existing save flow.
- `BP_Mole` remains directly placed in `BunkerMap`; its dialogue, quest provider, and quest state are unchanged.
- Memos use the directly placed `/Game/Interaction/BP_MemoAnchor` with only MemoId; runtime placement JSON is not used.

## Enemy placement

Every `EnemySpawns.json` row requires `level_name`, positive `placement_id`, and `profile_id`. It may contain chance/condition data, but must not contain `location`, `rotation`, or `scale`. A matching level instance of `/RaidLevelKit/Placement/BP_RaidPlacementAnchor` with kind `Enemy` owns the transform.

The demo row uses logical level `DemoRaidMap` and `placement_id=1`. Both demo raid maps contain a matching anchor, so the build-flavor level alias can select either map without falling back to coordinates. `EnemySpawnProfiles.json` resolves the actor/loadout, while `EnemyCombatProfiles.json` remains the source of combat behavior.

Coordinate-authored enemy rows are invalid and cause placement-data validation to fail.

## Loot-container placement

`LootContainerSpawns.json` is intentionally retained. A row with `placement_id` is anchor-owned and must not include transform fields. Existing rows without `placement_id` may continue to use `location` and optional `rotation`; this is the only retained coordinate-authored runtime placement path.

Anchor rows use kind `LootContainer`. Container class, definition id, contents id, chance, and condition remain JSON-owned.

## Memo placement

Place `/Game/Interaction/BP_MemoAnchor` (native `ATunaSweeperMemoAnchor`) and set MemoId to an entry in `MemoDefinitions.json`.
The default prop is `SM_MemoStorageDevice_LowPoly`. No PlacementId or MemoSpawns.json is used.
Collection immediately saves `AcquiredMemoIds`; a failed save leaves the memo collectible. Collected/unknown IDs are hidden and noninteractive.
Save initialization and collection refresh existing instances, including duplicates of the same MemoId. Re-entering a map respects the acquired IDs in the active save slot.
See `Docs/raid_placement_anchors.md` and `Docs/save_persistence.md` for the complete contract.

## Editor authoring

Place `/RaidLevelKit/Placement/BP_RaidPlacementAnchor`, choose `Enemy`, `Loot Container`, or `AuthoredActor` in the Details panel, and assign a stable positive `PlacementId` unique across all three kinds in that level. The arrow, billboard, label, and loot preview are editor-only. Runtime actors spawn at the anchor transform and do not use preview components as gameplay authority.

`Tools/RaidPlacement/place_quadruped_enemy_spawn.py` maintains the demo quadruped anchor in both demo raid maps without removing directly placed actors.
