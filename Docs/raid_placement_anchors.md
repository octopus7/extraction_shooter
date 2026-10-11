# Raid placement anchors

Read `Docs/raid_placement_id_numbering.md` before assigning or changing a `PlacementId`. It defines the reserved ranges, allocation procedure, and retirement rules for every anchor kind.

## Level contract

`ATunaSweeperRaidPlacementAnchor` is the spatial source of truth for data-owned enemy and loot-container placement, plus authored actor profiles. The reusable Blueprint is `/RaidLevelKit/Placement/BP_RaidPlacementAnchor`.

A placed instance serializes its actor transform, a positive `PlacementId`, and `AnchorKind`:

- `Enemy`
- `Loot Container`
- `AuthoredActor`

`PlacementId` must be unique across every anchor kind in one level. Enemy anchors may intentionally share an id only when every member of the duplicate group is an `Enemy` anchor with `Allow Duplicate Placement Id` enabled. Loot and authored-actor anchors never allow duplicates. Its stable key is `(LevelId, PlacementId)`. Do not recycle a shipped id for another location or kind.

The Details panel exposes the kind and id. Editor-only arrows, billboards, labels, and loot preview meshes make anchors visible without owning runtime behavior. Loot preview choices come from `/RaidLevelKit/Placement/DA_LootAnchorPreviews`. Preview components have no collision or gameplay authority.

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
  "placement_id": 1100,
  "loot_container_class": "/Game/Interaction/BP_LootContainer.BP_LootContainer_C",
  "container_definition_id": 7001,
  "contents_id": 8001,
  "spawn_chance": 10000,
  "condition_id": "always"
}
```

`spawn_chance`는 `0..10000` 정수 확률이다. `0`은 0%, `1`은 0.01%, `10000`은 100%이며 생략 시 `10000`이다. `0.65` 같은 `0..1` 비율 표기는 지원하지 않는다.

### 퀘스트 진행 중 등장하는 루팅 상자

택배상자도 `Loot Container` 앵커를 사용한다. 연결된 `LootContainerSpawns.json` 행에 `required_active_quest_id`를 지정한다. 값은 지역 접두나 0채움 없이 양수 int32 퀘스트 번호를 사용한다.

이 필드가 있는 행은 `spawn_chance`와 일반 `condition_id` 대신 퀘스트 상태로 등장 여부를 결정한다. 실제 저작 행에서는 두 기존 조건 필드를 생략한다.

- 해당 퀘스트 정의가 있고 상태가 `Accepted`이면 확률 없이 등장한다.
- 미수락(`Available`), 목표 달성 후 보상 대기(`RewardAvailable`), 보상 완료(`RewardCompleted`)에서는 나타나지 않는다.
- 잘못된 번호 형식(0, 음수, 소수, 문자열, null, int32 범위 초과)은 오류로 해당 행을 제외한다. 존재하지 않는 퀘스트 번호는 등장시키지 않는다.
- 레벨 진입뿐 아니라 퀘스트 변경 및 저장 로드 이벤트에서도 즉시 재판정한다. 숨길 때 충돌과 신규 상호작용도 차단된다.
- 같은 레벨에서 조건이 다시 활성화되면 기존 상자를 재사용한다. 반복 이벤트로 상자를 중복 생성하거나 내용물을 다시 채우지 않는다.
- 필드가 없는 일반 상자와 적은 기존 확률·조건 방식을 유지한다.

앵커의 위치/방향/PlacementId 소유 규칙은 같다. 퀘스트 조건은 게임 데이터에 두며 앵커 자체에 퀘스트 실행 로직을 넣지 않는다. 이 조건은 택배 아이템의 기지 귀환 시 소멸이나 도착지 전달 처리를 대신하지 않는다.

## Independent memo placement

Place `/Game/Interaction/BP_MemoAnchor` and set **MemoId** to a positive ID in `MemoDefinitions.json`.
Its native parent is `ATunaSweeperMemoAnchor`, directly derived from `AActor`.
The actor owns its transform and uses `SM_MemoStorageDevice_LowPoly` with its four mesh materials and unit scale.
There is no `AnchorKind`, `PlacementId`, spawn probability, or `MemoSpawns.json` mapping for memos.
The shared raid anchor no longer offers Memo; serialized kind value 2 is retired, while AuthoredActor remains 3.

The placed mesh is visible in the editor. At runtime, an unassigned/unknown MemoId or an already acquired memo is hidden, noncolliding, and unavailable for interaction.
Collection immediately saves the existing `AcquiredMemoIds` state before publishing success. A failed save rolls back collection and leaves the memo available for retry.
All live instances of the same MemoId hide together. Loading an acquired save or revisiting the map keeps the memo hidden; activating an uncollected save restores the placed instance.
The runtime actor stays hidden rather than being destroyed so save-slot changes in the same world work correctly.
Titles and bodies still use `MemoDefinitions.json` string keys; the interaction label uses `ui.interaction.memo`.

Anchor-owned rows must not contain `location`, `rotation`, or `scale`. `EnemySpawns.json` accepts only anchor-owned placement. `LootContainerSpawns.json` alone retains its existing coordinate-row form for backward compatibility.

Enemy class/loadout data stays in `EnemySpawnProfiles.json`, combat behavior stays in `EnemyCombatProfiles.json`, memo content stays in `MemoDefinitions.json`, and loot class/definition/contents stay in the loot placement row. Main-only authoring belongs in the access-restricted runtime payload; public Main defaults are empty fallbacks.

## Runtime validation

`UTunaSweeperRaidPlacementSubsystem` validates enemy and loot anchors. The independent memo actor validates its MemoId through `UTunaSweeperMemoSubsystem`, which only manages memo definitions and the collection UI list. Invalid/nonpositive ids, duplicate level ids, duplicate data ids, missing anchors, unconnected anchors, kind mismatches, missing definitions/profiles, and transform fields in anchor rows are logged explicitly. Invalid data never spawns at a guessed transform.

Enemy chance uses a deterministic hash of `(RaidSeed, PlacementId)`. Placement ordering therefore cannot change another placement's result. Only empty `condition_id` and `always` currently evaluate true; unknown conditions fail closed.

Memo collection persistence remains keyed by `memo_id` through `AcquiredMemoIds`; there is no secondary placement identity. Enemy and loot placement decisions are also not persisted.
