# Raid placement anchors

## Purpose and level ownership

`ATunaSweeperRaidPlacementAnchor` is the spatial source of truth for raid enemy and loot-container placements. A placed instance serializes only its actor Transform, a positive integer `PlacementId`, and `AnchorKind` (`Enemy` or `LootContainer`). Do not place a real enemy or loot-container actor to reserve one of these locations.

Place the native actor from the Class Viewer in a raid level, set a stable positive id, and choose the kind. `PlacementId` is unique across **both** anchor kinds in a level; its stable data key is `(LevelId, PlacementId)`. Never recycle an id for another location/type once external data has shipped.

The representative arrow/actor sprite and `ENEMY #id` label are editor-only enemy visualization. Loot anchors use the BP-referenced `/Game/Raid/Placement/DA_LootAnchorPreviews` catalog. The initial `Small`, `Medium`, and `Large` choices each use a distinct representative mesh; select one from the placed `BP_RaidPlacementAnchor` instance's `Loot Preview` combo. The DA's array can be extended without changing C++ or GameInstance configuration. These preview components have no collision, gameplay authority, or runtime spawn role, so actual chest class, appearance, contents, conditions, and probability remain external runtime data.

No anchors have been placed by this implementation. Designers must add the anchors to the intended raid maps.

## External data schemas

New anchor rows share the existing build-flavor-aware runtime placement files. They must not contain `location`, `rotation`, or `scale`; legacy coordinate rows remain supported by `UTunaSweeperEnemySpawnSubsystem` and are skipped by the anchor subsystem.

`Content/Data/EnemySpawnProfiles.json` owns enemy class and combat/loadout properties. Required fields are `profile_id`, `enemy_class`, and `combat_profile_id`. Optional fields mirror the existing enemy spawn values: `body_material`, loot ids, weapon/ammo ids, health/experience, bleed values, faction, and squad values.

```json
[
  {
    "profile_id": "enemy.rifle.standard",
    "enemy_class": "/Game/Characters/Enemy/BP_TunaSweeperEnemy.BP_TunaSweeperEnemy_C",
    "combat_profile_id": "enemy.rifle_anchor",
    "max_health": 40
  }
]
```

`Content/Data/EnemySpawns.json` anchor row:

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 101,
  "profile_id": "enemy.rifle.standard",
  "spawn_chance": 0.65,
  "condition_id": "always"
}
```

`Content/Data/LootContainerSpawns.json` anchor row (all actual container class, definition, and contents remain external):

```json
{
  "level_name": "DemoRaidMap",
  "placement_id": 201,
  "loot_container_class": "/Game/Interaction/BP_LootContainer.BP_LootContainer_C",
  "container_definition_id": 7001,
  "contents_id": 8001,
  "spawn_chance": 1.0,
  "condition_id": "always"
}
```

For Main, put the same files into the access-restricted runtime payload. The public `MainRuntimeDefaults` copies are empty fallbacks and must not receive Main authoring data.

## Runtime contract and validation

`UTunaSweeperRaidPlacementSubsystem` loads after a raid map is loaded. It validates invalid/nonpositive ids, duplicate anchor ids, duplicate data ids, absent anchors, unconnected anchors, missing enemy profiles/combat profiles, and anchor-kind mismatches with explicit `LogTunaSweeperRaidPlacement` errors/warnings. A bad individual row is skipped without spawning at a guessed transform.

`RaidSeed` is runtime-only and must be set with `SetRaidSeed` before the raid map loads. Each chance decision hashes only `(RaidSeed, PlacementId)`, so adding/reordering another placement cannot change its outcome. Until the raid-session owner supplies a seed, the subsystem warns and uses deterministic fallback `0` rather than hidden global randomness.

Initialization order is: load/validate data and anchors, evaluate condition and deterministic chance, resolve the external profile/class, spawn at the anchor Transform, apply combat/spawn state, then finish the enemy spawn. Loot containers are spawned at the anchor Transform and receive external definition/contents immediately before they can be interacted with. `PlacementId` identifies a level location, `ProfileId` / container `DefinitionId` identify external definitions, and the generated `raid_runtime_<level>_<placement>_<seed>` actor tag identifies only that runtime instance. None of these new placement decisions or runtime ids are persisted, so the save schema is unchanged.

Only empty `condition_id` and `always` are currently evaluable. Other condition ids are logged and fail closed until a condition evaluator is registered.
