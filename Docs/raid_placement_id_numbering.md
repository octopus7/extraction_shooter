# Raid placement ID numbering

This document is the source of truth for assigning `PlacementId` values to `/RaidLevelKit/Placement/BP_RaidPlacementAnchor` instances and matching runtime placement JSON rows.

## Number ranges

| Range | Anchor kind | Current use |
| --- | --- | --- |
| `1-999` | `Enemy` | Enemy spawn locations |
| `1000-1999` | `Loot Container` | Data-owned loot-container locations |
| `2000-2999` | Retired reservation | Do not allocate; memos now use only MemoId |
| `3000+` | `AuthoredActor` (enum 3) | Game-owned actor profiles for the exact physical map |

The range identifies the owning anchor kind for authoring and review. Runtime behavior still uses `AnchorKind` and the corresponding data file; it must not infer the kind from the number.

## Identity rules

- A `PlacementId` must be a positive integer and unique across every raid-placement anchor kind in one logical level. Enemy anchors may share an id only when every anchor in that duplicate group is `Enemy` and explicitly enables `Allow Duplicate Placement Id`; loot and authored-actor anchors never allow duplicates.
- The stable identity is `(LevelId, PlacementId)`. Keep the number stable when moving or rotating an existing anchor.
- Allocate the next unused number in the kind's range. Do not fill a gap by reusing an ID that previously identified another placement.
- Do not renumber existing placements merely to make a sequence contiguous.
- A deleted or retired placement keeps its number retired. This preserves deterministic spawn rolls, runtime instance IDs, logs, and references made by saved or external data.
- Maps selected through the same logical level alias must use the same ID for the same conceptual placement and the same `AnchorKind`. If an aliased map does not support that placement, its data must not resolve to that map.

## Data ownership

The level anchor owns the transform, `PlacementId`, and `AnchorKind`. Runtime data owns what is spawned:

| Anchor kind | Runtime data |
| --- | --- |
| `Enemy` | `TunaSweeper/Content/Data/EnemySpawns.json` and `EnemySpawnProfiles.json` |
| `Loot Container` | `TunaSweeper/Content/Data/LootContainerSpawns.json` |
| `AuthoredActor` | `/Game/RaidRuntime/Catalogs/DA_<MapId>_Actors` and game-owned profile Blueprints |

An anchor-owned JSON row must use the same logical `level_name` and `placement_id` as the level instance. It must not contain `location`, `rotation`, or `scale`.

## Authoring procedure

1. Identify the logical level and anchor kind.
2. Inspect the level anchors and every placement data file for that logical level.
3. Choose the next unused ID in the kind's range. Check uniqueness across enemy, loot-container, and authored-actor placements together. For intentionally repeated enemy locations, enable `Allow Duplicate Placement Id` on every duplicate enemy anchor.
4. Set the level instance's `AnchorKind` and `PlacementId`.
5. For enemy/loot, update the runtime row with matching logical `level_name` and `placement_id`. For `AuthoredActor`, update the game-owned catalog for the exact physical long-package `MapId`; its filename uses the physical map short name. Catalog entries map `PlacementId` to a profile and never duplicate anchor transforms.
6. Validate that the anchor exists, its kind matches the data, and no duplicate ID exists across kinds. A duplicate enemy group is valid only when every member opted in.

When changing an existing ID, update the map anchor and all matching JSON in the same change. If a placement changes kind, retire its old ID and allocate a new ID from the destination kind's range.

## Current allocations

For logical level `DemoRaidMap`:

- Enemy IDs `1-6` are allocated in public runtime data. `DemoBoxRaidMap` has one Enemy `1` anchor. `DemoRaidMap` has twelve Enemy `1` anchors, all with duplicate ID opt-in enabled, plus one Enemy `2` anchor. IDs `3-6` have no anchors in either physical map and cannot spawn there until matching anchors are authored.
- Loot Container ID `1000` is allocated in public runtime data and has a matching anchor in both `DemoBoxRaidMap` and `DemoRaidMap`.
- Memo is no longer a raid-placement kind. Place `/Game/Interaction/BP_MemoAnchor` and assign only `MemoId`; no raid PlacementId is allocated.

The audited `/Game/MainRaid/RaidMap` contains no raid-placement anchors. Across all three saved maps, no `3000+` ID is in use. This range is now assigned to `AuthoredActor = 3`; Existing-map migration was cancelled by the user; new-level authoring records allocations per physical map. Authored catalogs use the exact physical long package MapId rather than the logical alias. Original actor names remain stable because existing saves may use their FName as the object ID.

This section is a review aid, not an allocation registry. The maps and runtime data remain authoritative for whether an individual ID is in use.

## Review checklist

- The ID belongs to the documented range for its `AnchorKind`.
- The ID is unused across all three anchor kinds in the applicable physical level, except explicitly opted-in enemy duplicates.
- Map and enemy/loot JSON agree on logical level, ID, and kind; authored catalogs agree on exact physical MapId, ID, and profile.
- No transform fields were added to an anchor-owned JSON row.
- Aliased physical maps were checked where the logical placement applies.
- Existing IDs were not recycled or renumbered for cosmetic reasons.

See `Docs/raid_placement_anchors.md` for the anchor/runtime contract and JSON schema examples.

For logical level `RaidForest` (`/Game/MainRaid/RaidForest`), Enemy IDs `1` (gray robot), `2` (blue robot), and `3` (red robot) are allocated near the primary power facility. Each has one anchor and a guaranteed spawn row in `Content/Data/MainRuntimeDefaults/EnemySpawns.json`; the corresponding profiles are in `EnemySpawnProfiles.json`. These public Main defaults apply when no dataset-specific placement override exists.

For logical level `RaidVillage` (`/Game/MainRaid/RaidVillage`), Enemy IDs `1–2` are marine delivery robots, `3–4` are pirate delivery robots, and `5–6` are rabbit trikes. Each has one anchor in the `Region2/EnemyAnchors` folder and a guaranteed spawn row in `Content/Data/MainRuntimeDefaults/EnemySpawns.json`. The map owns their transforms; matching profiles are in `EnemySpawnProfiles.json`. These IDs are independent of the same numbers in other logical levels.

RaidVillage Loot Container IDs `1000` and `1001` are retired after converting their parcel boxes to directly placed dedicated pickup Blueprints. Do not reuse those IDs. Parcel pickup Blueprints do not use raid placement IDs or loot-container spawn rows.
