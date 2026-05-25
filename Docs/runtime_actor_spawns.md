# Runtime Actor Spawns

이 문서는 레벨에 직접 배치하지 않고 데이터로 초기 배치하는 런타임 액터 목록과 JSON 파일을 정리한다.

## 공통 소유자

- 런타임 배치 소유자는 `UTunaSweeperEnemySpawnSubsystem`이다.
- 맵 로드 후 `EnsureRaidRuntimeActorsSpawnedForWorld`에서 현재 월드 이름과 각 JSON의 `level_name`을 비교해 스폰한다.
- `RaidMap`에 직접 배치되어 있던 게임플레이/상호작용 `TS_` 액터 6개는 `GameplayInteractionSpawns.json`으로 옮겼다.
- 이 6개는 레벨 자산에 직접 배치하지 않는다.

## GameplayInteractionSpawns.json

파일: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

현재 `RaidMap` 초기 배치:

| spawn_id | spawn_type | 주요 데이터 |
| --- | --- | --- |
| `TS_Travel_ToBunker` | `level_travel` | `BunkerMap` 이동, `To Bunker`, 전환 영상/문구 |
| `TS_PickupItem_Sample` | `pickup_item` | `item_id=1001`, `item_quantity=1` |
| `TS_Interact_ItemSpawn` | `item_spawn` | `BP_PickupItem` 랜덤 스폰, 반경 `160~420` |
| `TS_LootContainer_Sample` | `loot_container` | `container_definition_id=7001`, `contents_id=8001` |
| `TS_Interact_LootContainerSpawn` | `loot_container_spawn` | `BP_LootContainer` 랜덤 스폰, 반경 `180~440` |
| `TS_Interact_SelfDestruct` | `self_destruct` | 3초 카운트다운, 반경 `200`, 피해 `100` |

## 기존 데이터 파일

- `EnemySpawns.json`: 적 초기 배치.
- `LootContainerSpawns.json`: 고정 루트 컨테이너 초기 배치.
- `TransparentObstacleSpawns.json`: 반투명 장애물 초기 배치.
- `WorldProgressObjectSpawns.json`: 월드 진행 오브젝트 초기 배치.
- `WarpPointSpawns.json`: 워프 포인트 초기 배치.
