# Runtime Actor Spawns

> 현재 상태(2026-08-28): 새 데모 구성을 위해 비웠던 런타임 스폰 데이터 중 `EnemySpawnProfiles.json`과 `EnemySpawns.json`에는 사족 총기 적 검증용 항목 1개가 다시 활성화되어 있다. 나머지 기존 배치 설명은 재작성 참고용 기록이며 현재 활성 배치가 아니다. 제거 목록은 `Docs/demo_runtime_data_cleanup_2026-08-24.md`, 재사용 절차는 `Docs/quest_and_runtime_actor_data_authoring_guide.md`를 참고한다.

이 문서는 레벨에 직접 배치하지 않고 데이터로 초기 배치하는 런타임 액터 목록과 JSON 파일을 정리한다.

## 공통 소유자

- 런타임 배치 소유자는 `UTunaSweeperEnemySpawnSubsystem`이다.
- 맵 로드 후 `EnsureRaidRuntimeActorsSpawnedForWorld`에서 현재 월드 이름과 각 JSON의 `level_name`을 비교해 스폰한다.
- `RaidMap`/`BunkerMap`에 직접 배치되어 있던 게임플레이/상호작용 `TS_` 액터와 추가 런타임 액터는 `GameplayInteractionSpawns.json`으로 옮겼다.
- 이 런타임 액터들은 레벨 자산에 직접 배치하지 않는다.

## GameplayInteractionSpawns.json

파일: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

현재 주요 초기 배치:

| spawn_id | spawn_type | 주요 데이터 |
| --- | --- | --- |
| `TS_ShootingPracticeDummy_01` | `shooting_practice_dummy` | `BunkerMap` 사격 연습용 허수아비. 체력 `100`, 크리티컬 `3x`, 헤드샷 `6x`, 2초 회복 |
| `TS_VendingMachine_Shop_01` | `vending_machine` | `BunkerMap` 상점 액터. `shop_id=1`, `ui.interaction.shop_open` |
| `TS_Travel_DeployToRaid` | `level_travel` | `BunkerMap` 레이드 출동 사다리. `RaidMap` 이동, `Deploy`, 사다리 메시와 전환 영상/문구 |
| `TS_Travel_ToBunker` | `level_travel` | 레이드 시작 위치의 직접 진입 복귀 루트. `BunkerMap` 이동, `To Bunker`, 전환 영상/문구 |
| `TS_ExtractionPoint_East` | `extraction_point` | 추출 포인트 복귀 루트. `BunkerMap` 이동, 4초 체류, 반경 `200`, 경계원 두께 `2` |
| `TS_PickupItem_Sample` | `pickup_item` | `item_id=1001`, `item_quantity=1` |
| `TS_Interact_ItemSpawn` | `item_spawn` | `BP_PickupItem` 랜덤 스폰, 반경 `160~420` |
| `TS_LootContainer_Sample` | `loot_container` | `container_definition_id=7001`, `contents_id=8001` |
| `TS_Interact_LootContainerSpawn` | `loot_container_spawn` | `BP_LootContainer` 랜덤 스폰, 반경 `180~440` |
| `TS_Interact_SelfDestruct` | `self_destruct` | 3초 카운트다운, 반경 `200`, 피해 `100` |
| `TS_RollingBomberSpawner_West` | `rolling_bomber_spawner` | 서쪽 롤링 봄버 스포너, 웨이브 스폰 |
| `TS_SandbagCover_Central` | `sandbag_cover` | 모래주머니 임시 엄폐물. `RaidMap` 위쪽 파란 메모보다 북쪽인 `[1120, 180, 0]` 배치, 내구도 `70`, 근접 통과 반경 `62.5` |
| `TS_ExplosiveBarrel_01~03` | `explosive_barrel` | 회색 폭발 드럼통. 내구도 `30`, 파괴 시 폭발 이펙트와 밑둥 메시로 전환 |

`TS_Travel_ToBunker`와 `TS_ExtractionPoint_East`는 서로 대체 관계가 아니다. 둘 다 `RaidMap`에서 `BunkerMap`으로 연결되는 복귀 루트이며, 직접 상호작용 복귀와 추출 지점 복귀를 동시에 지원한다.

`TS_Travel_DeployToRaid`는 더 이상 에디터 셋업 코드가 `BunkerMap`에 직접 저장 배치하지 않는다. 같은 이름의 과거 레벨 배치 액터가 남아 있더라도 런타임 스포너가 `spawn_id`, 에디터 라벨, 오브젝트 이름, 또는 동일 클래스/좌표를 기준으로 제거한 뒤 JSON 행을 기준으로 다시 스폰한다.

## Bunker Mole

File: `TunaSweeper/Content/Data/BunkerCharacterSpawns.json`

`TS_Bunker_LED_Robot` spawns `BP_Mole` through `UTunaSweeperBunkerRuntimeSpawnSubsystem`. `ATunaSweeperMoleCompanionActor` owns the Mole dialogue and quest interactables directly. Its quest provider is `provider.mole`, with `quest_first_outing` as the fallback quest.

## 기존 데이터 파일

- `EnemySpawns.json`: 적 초기 배치.
- `LootContainerSpawns.json`: 고정 루트 컨테이너 초기 배치.
- `TransparentObstacleSpawns.json`: 반투명 장애물 초기 배치.
- `WorldProgressObjectSpawns.json`: 월드 진행 오브젝트 초기 배치.
- `WarpPointSpawns.json`: 워프 포인트 초기 배치.

`LootContainerSpawns.json`의 `BunkerMap` 개발자 상자는 `container_definition_id=7008`, `contents_id=8007`이며 현재 위치는 `[441.322, 548.9291, 40.0]`이다.

## Demo Quadruped Gun Enemy

- `/Game/Maps/DemoRaidMap`에는 실제 적 Blueprint가 아니라 `BP_RaidPlacementAnchor` 인스턴스 `TS_EnemySpawn_QuadrupedGun_01`만 배치한다.
- 앵커는 `PlacementId=1`, 종류 `Enemy`, 위치 `[5200, 1040, 90]`, 회전 `[0, 180, 0]`을 사용한다.
- `EnemySpawns.json`의 `DemoRaidMap`/`placement_id=1` 행이 `enemy.quadruped_gun_test` 프로필을 선택한다. 좌표·회전·스케일은 JSON에 두지 않고 레벨 앵커 Transform이 소유한다.
- `EnemySpawnProfiles.json`의 `enemy.quadruped_gun_test`는 `/Game/Blueprints/BP_QuadrupedGunEnemy`와 `enemy.rifle_anchor` 전투 프로필, 라이플·탄약 및 임시 체력·전리품 값을 연결한다.
- 게임 월드가 시작되면 `UTunaSweeperRaidPlacementSubsystem`이 앵커와 두 JSON 행을 결합해 `BP_QuadrupedGunEnemy`를 런타임 생성한다. 따라서 적 BP 인스턴스 자체는 레이드 맵에 저장하지 않는다.

## MemoSpawns.json

File: `TunaSweeper/Content/Data/MemoSpawns.json`

Memo actors are spawned by `UTunaSweeperMemoSubsystem` after map load. Each row uses `level_name`, `location`, and `memo_id`; optional fields include `actor_class`, `visual_mesh`, `visual_material`, `visual_scale`, `visual_relative_location`, `rotation`, `scale`, `spawn_id`, `interaction_display_name`, and `marker_widget_class`.

The subsystem skips rows whose `memo_id` is already present in `UTunaSweeperGameInstance::AcquiredMemoIds`, so collected memos do not respawn in the same runtime session or after a saved reload.

Definitions are read from `TunaSweeper/Content/Data/MemoDefinitions.json`.

## RollingBomber Spawner

File: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

`rolling_bomber_spawner` rows create `ATunaSweeperRollingBomberSpawner` at runtime. The first row is `TS_RollingBomberSpawner_West` on `RaidMap`, placed west of the main view at `[220.0, -3600.0, 90.0]`.

The spawner launches `RollingBomber` enemies one at a time with `spawn_interval_seconds=0.2`. It starts at `initial_spawn_count=2`, doubles the wave count every `wave_interval_seconds=10.0`, clamps at `max_spawn_count=8`, and keeps spawning 8 per later wave. Destroying the spawner stops both wave and burst timers.

Newly launched `RollingBomber` enemies enter a short bouncy physics state, then blend through a standing-up-from-spawn state before enabling normal projectile attack behavior.

Spawner visuals are generated by `ATunaSweeperRollingBomberSpawner` as a square pillar with a large six-sided tower head. The generated source texture is stored at `TunaSweeper/Content/SourceArt/RollingBomber/T_RollingBomberSpawner_Mechanic_Source.png`; the editor import command creates `/Game/Interaction/T_RollingBomberSpawner_Mechanic` and `/Game/Interaction/M_RollingBomberSpawner_Mechanic`.

## Sandbag Cover

File: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

`sandbag_cover` rows create `ATunaSweeperSandbagCoverActor` through `BP_SandbagCover`. The current default cover uses `box_extent=[37.5, 160.0, 60.0]` and a four-layer visible sandbag stack, so bullet collision height matches the raised visual cover instead of relying on invisible blocking above the mesh. The actor blocks pawns and projectiles, takes projectile damage, and starts a scripted collapse when health reaches zero. Collision is disabled at collapse start, so the cover is immediately passable while the visible bags slide down and spread out for a short hold before the actor destroys itself.

The visible stack uses repeated `/Game/Interaction/SM_Sandbag_LowPoly` static mesh components rather than runtime procedural boxes. The generated mesh shares each ring vertex and assigns matching smooth vertex normals to duplicate vertex instances, so normal-based outline expansion does not split the bag into separate face shells. When the player is within `passthrough_radius` of the cover bounds, each visible sandbag component writes CustomDepth/CustomStencil value `3` and gets `/Game/Interaction/M_SandbagCover_OverlayOutline` as its overlay material. That translucent unlit material expands the rendered shell along `VertexNormalWS` by `OutlineThickness`, masks the visible-facing side with `TwoSidedSign`, samples `CustomStencil`, and hides pixels whose screen-space stencil is `3`, so the outline remains outside the original sandbag silhouette instead of covering the interior. Player-fired projectiles ignore that cover for the shot. Enemy projectiles still collide with and damage the cover until collapse starts, so shots blocked by the sandbags do not continue into the player.

The source texture generated with imagegen is stored at `TunaSweeper/Content/SourceArt/SandbagCover/T_SandbagCover_Burlap_Source.png`; the editor setup command creates `/Game/Interaction/T_SandbagCover_Burlap`, `/Game/Interaction/M_SandbagCover_Burlap`, `/Game/Interaction/M_SandbagCover_OverlayOutline`, `/Game/Interaction/SM_Sandbag_LowPoly`, and `/Game/Interaction/BP_SandbagCover`.

## Explosive Barrel

File: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

`explosive_barrel` rows create `ATunaSweeperExplosiveBarrelActor` through `BP_ExplosiveBarrel`. The default barrel has `barrel_max_health=30`, so three default player rifle projectile hits at `10` damage each destroy it.

On destruction the actor spawns the configured `explosion_effect_actor_class`, swaps from `intact_mesh` to `destroyed_mesh`, shrinks collision to the remaining base, and starts `destroyed_loop_effect` if one is configured. The destroyed mesh and loop effect are JSON/BP-configurable so they can be replaced later without changing the actor behavior.

## Map Overlay

`GameplayInteractionSpawns.json` rows can optionally include a `mapOverlay` object. If present, the map widget projects the row's world `location` plus `mapOverlay.world_offset` into map space and draws the requested label/icon.

Supported fields:

- `text_string_key`: UI text string key for the map label.
- `icon`: Icon id. `green_inverted_triangle` draws a green downward triangle.
- `world_offset`: Optional world-space offset `[x, y, z]` in UE centimeters before projection.
- `text_offset`: Optional map-widget pixel offset `[x, y]` for the label.
- `icon_offset`: Optional map-widget pixel offset `[x, y]` for the icon.

Current overlays mark `TS_Travel_ToBunker` as the start location and `TS_ExtractionPoint_East` as the extraction point.

## Static Mesh Prop

File: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

`static_mesh_prop` rows create an `AStaticMeshActor` at runtime. Required fields are `level_name`, `spawn_id`, `spawn_type`, `location`, and `static_mesh`. Optional fields include `actor_class`, `static_mesh_materials`, `static_mesh_relative_location`, `static_mesh_relative_rotation`, `static_mesh_relative_scale`, and `collision_enabled`.

## Shooting Practice Dummy

File: `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`

`shooting_practice_dummy` rows create `ATunaSweeperShootingPracticeDummyActor`. The dummy is non-lethal and never reaches zero health; it clamps to a minimum health value, then regenerates back to full over `health_recovery_seconds` while not taking new damage. Point-damage hit components choose the multiplier: body is normal, the orange plate is critical, and the red center plate/head is headshot.
