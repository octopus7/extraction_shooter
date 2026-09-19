# Raid placement ID numbering

This document is the source of truth for assigning `PlacementId` values to `/Game/Raid/Placement/BP_RaidPlacementAnchor` instances and matching runtime placement JSON rows.

## Number ranges

| Range | Anchor kind | Current use |
| --- | --- | --- |
| `1-999` | `Enemy` | Enemy spawn locations |
| `1000-1999` | `Loot Container` | Data-owned loot-container locations |
| `2000-2999` | `Memo` | Collectible memo locations |
| `3000+` | Reserved | Future anchor kinds; do not allocate without updating this document |

The range identifies the owning anchor kind for authoring and review. Runtime behavior still uses `AnchorKind` and the corresponding data file; it must not infer the kind from the number.

## Identity rules

- A `PlacementId` must be a positive integer and unique across every raid-placement anchor kind in one logical level.
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
| `Memo` | `TunaSweeper/Content/Data/MemoSpawns.json` and `MemoDefinitions.json` |

An anchor-owned JSON row must use the same logical `level_name` and `placement_id` as the level instance. It must not contain `location`, `rotation`, or `scale`.

## Authoring procedure

1. Identify the logical level and anchor kind.
2. Inspect the level anchors and every placement data file for that logical level.
3. Choose the next unused ID in the kind's range. Check uniqueness across enemy, loot-container, and memo rows together.
4. Set the level instance's `AnchorKind` and `PlacementId`.
5. Add or update the matching runtime data row with the same `level_name` and `placement_id`.
6. Validate that the anchor exists, its kind matches the data, and no duplicate ID exists across kinds.

When changing an existing ID, update the map anchor and all matching JSON in the same change. If a placement changes kind, retire its old ID and allocate a new ID from the destination kind's range.

## Current allocations

For logical level `DemoRaidMap`:

- Enemy IDs `1-6` are allocated in runtime data. ID `1` has matching anchors in `DemoRaidMap` and `DemoBoxRaidMap`; IDs `2-6` are reserved for the color variants and require matching `Enemy` anchors before those placements can spawn.
- Loot Container ID `1000` is allocated in runtime data and has a matching anchor in `DemoBoxRaidMap`. Any other physical map that resolves this logical-level row must add the same placement or exclude the row from that map.
- No Memo ID is currently allocated.

This section is a review aid, not an allocation registry. The maps and runtime data remain authoritative for whether an individual ID is in use.

## Review checklist

- The ID belongs to the documented range for its `AnchorKind`.
- The ID is unused across all three anchor kinds in the logical level.
- Map and JSON use the same logical level, ID, and kind.
- No transform fields were added to an anchor-owned JSON row.
- Aliased physical maps were checked where the logical placement applies.
- Existing IDs were not recycled or renumbered for cosmetic reasons.

See `Docs/raid_placement_anchors.md` for the anchor/runtime contract and JSON schema examples.
