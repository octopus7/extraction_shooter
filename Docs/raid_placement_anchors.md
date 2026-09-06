# Raid placement anchors

## Level contract

`ATunaSweeperRaidPlacementAnchor` is the spatial source of truth for data-owned enemy, loot-container, and memo placement. The reusable Blueprint is `/Game/Raid/Placement/BP_RaidPlacementAnchor`.

A placed instance serializes its actor transform, a positive `PlacementId`, and `AnchorKind`:

- `Enemy`
- `Loot Container`
- `Memo`

`PlacementId` must be unique across every anchor kind in one level. Its stable key is `(LevelId, PlacementId)`. Do not recycle a shipped id for another location or kind.

The Details panel exposes the kind and id. Editor-only arrows, billboards, labels, and loot preview meshes make anchors visible without owning runtime behavior. Loot preview choices come from `/Game/Raid/Placement/DA_LootAnchorPreviews`; memo anchors use the blue `MEMO` preview. Preview components have no collision or gameplay authority.

## Data schemas

Enemy placement:

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 101,
  "profile_id": "enemy.rifle.standard",
  "spawn_chance": 6500,
  "condition_id": "always"
}
```

Loot-container anchor placement:

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 201,
  "loot_container_class": "/Game/Interaction/BP_LootContainer.BP_LootContainer_C",
  "container_definition_id": 7001,
  "contents_id": 8001,
  "spawn_chance": 10000,
  "condition_id": "always"
}
```

`spawn_chance`는 `0..10000` 정수 확률이다. `0`은 0%, `1`은 0.01%, `10000`은 100%이며 생략 시 `10000`이다. `0.65` 같은 `0..1` 비율 표기는 지원하지 않는다.

Memo placement:

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 301,
  "memo_id": 1,
  "visual_scale": [0.85, 0.55, 0.08]
}
```

Anchor-owned rows must not contain `location`, `rotation`, or `scale`. `EnemySpawns.json` and `MemoSpawns.json` accept only anchor-owned placement. `LootContainerSpawns.json` alone retains its existing coordinate-row form for backward compatibility.

Enemy class/loadout data stays in `EnemySpawnProfiles.json`, combat behavior stays in `EnemyCombatProfiles.json`, memo content stays in `MemoDefinitions.json`, and loot class/definition/contents stay in the loot placement row. Main-only authoring belongs in the access-restricted runtime payload; public Main defaults are empty fallbacks.

## Runtime validation

`UTunaSweeperRaidPlacementSubsystem` validates enemy and loot anchors. `UTunaSweeperMemoSubsystem` validates memo anchors. Invalid/nonpositive ids, duplicate level ids, duplicate data ids, missing anchors, unconnected anchors, kind mismatches, missing definitions/profiles, and transform fields in anchor rows are logged explicitly. Invalid data never spawns at a guessed transform.

Enemy chance uses a deterministic hash of `(RaidSeed, PlacementId)`. Placement ordering therefore cannot change another placement's result. Only empty `condition_id` and `always` currently evaluate true; unknown conditions fail closed.

Memo collection persistence remains keyed by `memo_id` through `AcquiredMemoIds`; the anchor id and runtime actor tag are not persisted. Enemy and loot placement decisions are also not persisted.
