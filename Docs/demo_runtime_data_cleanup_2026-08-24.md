# 데모 초기화: 퀘스트 및 JSON 런타임 액터 제거 목록

정리일: 2026-08-24

## 범위

이번 정리는 데모를 새로 구성하기 위해 게임이 `TunaSweeper/Content/Data`에서 직접 읽는 공개 퀘스트 콘텐츠와 JSON 런타임 배치 행을 비운 작업이다. C++/Blueprint 기능 코드, 클래스, 메시·머티리얼·UI 에셋, 아이템·전투·메모 본문 정의, 맵에 직접 배치된 액터는 삭제하지 않았다.

빈 데이터 파일은 삭제하지 않고 유효한 JSON 배열인 `[]`로 유지했다. 따라서 같은 파일에 새 행을 추가하면 기존 기능을 다시 사용할 수 있다.

## 요약

| 구분 | 제거 수량 | 현재 상태 |
| --- | ---: | --- |
| 공개 퀘스트 정의 | 5 | `QuestDefinitions.json` = `[]` |
| 퀘스트 콘텐츠 문자열 | 42 | 공용 `quest.ui.*` 26행만 보존 |
| JSON 런타임 액터 배치 | 53 | 스폰 JSON 8개가 모두 `[]` |

## 제거한 퀘스트 정의

파일: `TunaSweeper/Content/Data/QuestDefinitions.json`

| quest_id | 기존 제목 | provider_id |
| --- | --- | --- |
| `demo_q1_water_intake_check` | 취수 시설 확인 | `provider.mole` |
| `demo_q2_clear_water_screen` | 크로우바와 스크린 청소 | `provider.mole` |
| `demo_q3a_repair_valve` | 부러진 밸브 손잡이 | `provider.mole` |
| `demo_q3b_repair_bunker_pipe` | 새는 벙커 배관 | `provider.mole` |
| `demo_q4_todays_reward` | 오늘의 보상 | `provider.mole` |

## 제거한 퀘스트 콘텐츠 문자열

파일: `TunaSweeper/Content/Data/QuestTextStrings.csv`

| 키 그룹 | 제거 행 수 |
| --- | ---: |
| `quest.speaker.mole` | 1 |
| `quest.first_outing.*` | 5 |
| `quest.lumberjack_first_kill.*` | 3 |
| `quest.signalbot_map_check.*` | 3 |
| `quest.ricepotbot_supply_check.*` | 3 |
| `quest.rescue_cart_return.*` | 3 |
| `quest.warp_point_used_test.*` | 3 |
| `quest.demo_q1.*` | 5 |
| `quest.demo_q2.*` | 5 |
| `quest.demo_q3a.*` | 3 |
| `quest.demo_q3b.*` | 3 |
| `quest.demo_q4.*` | 5 |

`quest.ui.*` 공용 UI 문자열은 퀘스트 기능을 다시 사용할 때 필요하므로 보존했다.

## 제거한 JSON 런타임 액터

### 벙커 캐릭터 1개

파일: `BunkerCharacterSpawns.json`

| spawn_id | 레벨 | 클래스 | 위치 |
| --- | --- | --- | --- |
| `TS_Bunker_Mole` | `BunkerMap` | `BP_Mole` | `[700, -320, 0]` |

### 적 5개

파일: `EnemySpawns.json`

고유 `spawn_id`가 없는 기존 행은 전투 프로필과 위치로 식별한다.

| 행 | enemy_id | combat_profile_id | 위치 |
| ---: | --- | --- | --- |
| 1 | 없음 | `enemy.pistol_flanker` | `[5200, 1040, 90]` |
| 2 | 없음 | `enemy.rifle_anchor` | `[6200, 1520, 90]` |
| 3 | `enemy.lumberjack` | `enemy.elite_rifle_anchor` | `[7200, 1040, 90]` |
| 4 | `enemy.lumberjack` | `enemy.melee_lumberjack` | `[-2600, 120, 90]` |
| 5 | 없음 | `enemy.melee_lumberjack` | `[-2600, 320, 90]` |

### 고정 루트 컨테이너 3개

파일: `LootContainerSpawns.json`

| 레벨 | container_definition_id | contents_id | 위치 |
| --- | ---: | ---: | --- |
| `RaidMap` | 7005 | 8004 | `[1500, 220, 40]` |
| `RaidMap` | 7006 | 8005 | `[1750, 220, 40]` |
| `BunkerMap` | 7008 | 8007 | `[441.322, 548.9291, 40]` |

### 게임플레이/상호작용 액터 19개

파일: `GameplayInteractionSpawns.json`

| spawn_id | spawn_type | 레벨 | 위치 |
| --- | --- | --- | --- |
| `TS_ShootingPracticeDummy_01` | `shooting_practice_dummy` | `BunkerMap` | `[1040, -120, 0]` |
| `TS_VendingMachine_Shop_01` | `vending_machine` | `BunkerMap` | `[820, 180, 0]` |
| `TS_DifficultyAdjustment_01` | `difficulty_adjustment` | `BunkerMap` | `[620, 180, 0]` |
| `TS_DebugPiggyBank_01` | `piggy_bank` | `BunkerMap` | `[180, -1000, 40]` |
| `TS_Travel_DeployToRaid` | `level_travel` | `BunkerMap` | `[577.426, 359.909, 4]` |
| `TS_Travel_ToBunker` | `level_travel` | `RaidMap` | `[220, 220, 80]` |
| `TS_ExtractionPoint_East` | `extraction_point` | `RaidMap` | `[529.091385, 2009.645971, 90]` |
| `TS_PickupItem_Sample` | `pickup_item` | `RaidMap` | `[950, 50, 8]` |
| `TS_Interact_ItemSpawn` | `item_spawn` | `RaidMap` | `[950, -200, 80]` |
| `TS_LootContainer_Sample` | `loot_container` | `RaidMap` | `[1220, 50, 40]` |
| `TS_Interact_LootContainerSpawn` | `loot_container_spawn` | `RaidMap` | `[1220, -220, 80]` |
| `TS_Interact_SelfDestruct` | `self_destruct` | `RaidMap` | `[1520, -220, 80]` |
| `TS_PeriodicNoiseEmitter_TestHorn` | `periodic_noise_emitter` | `RaidMap` | `[2220, -1690, 0]` |
| `TS_PeriodicNoiseEmitter_TestHorn_East20m` | `periodic_noise_emitter` | `RaidMap` | `[2220, 310, 0]` |
| `TS_RollingBomberSpawner_West` | `rolling_bomber_spawner` | `RaidMap` | `[220, -3600, 90]` |
| `TS_SandbagCover_Central` | `sandbag_cover` | `RaidMap` | `[1120, 180, 0]` |
| `TS_ExplosiveBarrel_01` | `explosive_barrel` | `RaidMap` | `[1780, 40, 0]` |
| `TS_ExplosiveBarrel_02` | `explosive_barrel` | `RaidMap` | `[1900, -90, 0]` |
| `TS_ExplosiveBarrel_03` | `explosive_barrel` | `RaidMap` | `[2020, 70, 0]` |

### 메모 액터 20개

파일: `MemoSpawns.json`

`RaidMap`의 `memo_id` 1부터 20까지 모든 배치 행을 제거했다. 메모 본문 데이터인 `MemoDefinitions.json`은 보존했다.

### 반투명 장애물 2개

파일: `TransparentObstacleSpawns.json`

| obstacle_id | 레벨 | 위치 |
| --- | --- | --- |
| `raid_creek_blocker_01` | `RaidMap` | `[1120, 740, 120]` |
| `raid_creek_blocker_02` | `RaidMap` | `[1620, 960, 120]` |

### 워프 포인트 2개

파일: `WarpPointSpawns.json`

| warp_point_id | target_warp_point_id | 레벨 | 위치 |
| --- | --- | --- | --- |
| `raid_warp_point_a` | `raid_warp_point_b` | `RaidMap` | `[420, -520, 90]` |
| `raid_warp_point_b` | `raid_warp_point_a` | `RaidMap` | `[2600, 1280, 90]` |

### 월드 진행 오브젝트 1개

파일: `WorldProgressObjectSpawns.json`

| object_id | info_id | 레벨 | 위치 |
| --- | --- | --- | --- |
| `raid_creek_broken_bridge_01` | `obstacle.broken_bridge` | `RaidMap` | `[1370, 850, 120]` |

## 보존한 항목

- 퀘스트 서브시스템, 목표 이벤트 연결, 보상·UI·저장 기능
- 모든 런타임 스폰 서브시스템과 액터 클래스/Blueprint/시각 에셋
- `EnemyCombatProfiles.json`, `LootContainerTable.json`, `LootContainerContents.json`, `MemoDefinitions.json` 등 스폰되지 않는 정의 데이터
- 맵에 직접 배치된 액터 및 맵 자산
- 설계·스토리·과거 구현 기록 문서

## 복구 방법

삭제된 행은 Git 변경 이력에서 복구할 수 있다. 새 콘텐츠를 만드는 방법은 `Docs/quest_and_runtime_actor_data_authoring_guide.md`를 따른다. 과거 행을 그대로 되살릴 때도 전체 파일을 되돌리기보다 필요한 행만 현재 빈 배열 안에 복사하는 편이 안전하다.
