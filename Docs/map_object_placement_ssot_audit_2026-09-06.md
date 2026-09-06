# 맵 오브젝트 배치 SSOT 정리 분석

분석일: 2026-09-06. 상태: 분석 및 정리 제안이며 구현 완료된 SSOT 규칙이 아니다.

## 범위와 확인 수준

공개 저장소의 런타임 C++ 배치 진입점, Interaction 액터, 배치 JSON, 공개 MainRuntimeDefaults, 관련 작성 도구와 문서를 정적으로 조사했다. `.umap`을 에디터로 열어 모든 BP 인스턴스와 개별 오버라이드를 전수 덤프하거나 PIE를 실행하지는 않았다. 따라서 아래 행 수는 JSON의 현재 상태이며 실제 맵 액터 수가 아니다. Main 전용 payload 내용은 열람하지 않았으며 공개 코드의 경로 선택만 확인했다.

## 사용자 기준을 적용한 소유권

| 대상 | 배치 위치·형상 | 교체 가능한 내용·설정 | 플레이 진행 상태 |
| --- | --- | --- | --- |
| 적·아이템 상자 출현 위치 | 레벨 BP 앵커의 Transform, PlacementId, Kind | JSON의 출현 확률, 프로필, 실제 클래스, 전리품 정의 | 레이드 런타임 상태 |
| 취수시설·다리·본부 입구·추출 지점 | 레벨 BP 및 인스턴스의 고정 설정 | 별도 배치 JSON을 만들지 않음. 공통 문구·전환 연출 등 기존 참조는 유지 | 필요한 진행 상태만 세이브 |
| 상점·제작·연구·메모 등 내용이 데이터인 기능 | 고정 위치라면 레벨 BP가 공간과 식별자 보유 | 기존 기능 JSON/CSV가 상품·레시피·연구·본문 소유 | 구매·해금·수집 결과는 기존 저장 경로 |
| 플레이어가 건설한 시설 | 세이브의 배치 상태와 레벨 하우징 영역 | HousingFacilityDefinitions.json | 시설 인스턴스·배치·보관 상태 |
| 테스트용 직접 배치 | 기존 BP 직접 배치 허용 | 테스트에 필요한 BP 설정 허용 | 해당 기능에 따름 |

핵심은 설정의 가변성과 플레이 중 상태 변화를 구분하는 것이다. 다리가 수리되거나 취수시설 이물질이 사라지는 것은 고정된 규칙에 따른 상태 변화이며, 그 이유만으로 별도 배치 JSON이 필요하지 않다. 반대로 적 구성·드롭·웨이브 수치처럼 데이터로 교체할 설정은 레벨 인스턴스에 복제하지 않는다.

JSON 전용 좌표 배치를 제거해도 런타임 SpawnActor 자체를 제거하는 것은 아니다. 앵커에서 실제 적을 생성하거나 사망 전리품, 투사체, 파편, 건설 시설을 생성하는 것은 필요한 게임 동작이다.

## 현재 배치 파일 전수 목록

경로 기준: `TunaSweeper/Content/Data/`.

| 파일 | 공개 Demo 행 수 | 실제 소비자 | 현재 지원 방식 | 정리 방향 |
| --- | ---: | --- | --- | --- |
| EnemySpawns.json | 1 | RaidPlacementSubsystem / EnemySpawnSubsystem | placement_id 앵커 행 + 옛 location 좌표 행 | 앵커 행 유지, 좌표 행 로더 제거 |
| LootContainerSpawns.json | 0 | RaidPlacementSubsystem / EnemySpawnSubsystem | 앵커 행 + 옛 좌표 행 | 앵커 행 유지, 좌표 행 로더 제거 |
| GameplayInteractionSpawns.json | 0 | EnemySpawnSubsystem | spawn_type, spawn_id, 좌표, 타입별 설정을 합쳐 런타임 생성 | 좌표 배치 체계 폐기 후보. 필요한 설정은 기능 프로필로 이전 |
| WorldProgressObjectSpawns.json | 0 | EnemySpawnSubsystem | 다리 등 진행 오브젝트의 위치·재료·완성 클래스 일괄 지정 | 직접 배치로 통일, 배치 파일·로더 제거 후보 |
| WarpPointSpawns.json | 0 | EnemySpawnSubsystem | 위치 및 워프 ID/대상 ID | 연결이 고정이면 레벨 BP로 통일 |
| TransparentObstacleSpawns.json | 0 | EnemySpawnSubsystem | 위치 및 장애물 ID·크기 | 레벨 BP로 통일 |
| BunkerCharacterSpawns.json | 0 | BunkerRuntimeSpawnSubsystem | 사실상 두더지 캐릭터 클래스·좌표·더미 외형 | 고정 NPC는 직접 배치, 캐릭터 배치 서브시스템 제거 후보 |
| MemoSpawns.json | 0 | MemoSubsystem | 좌표·MemoId·외형 | 고정 위치 BP + MemoId로 전환, 본문 정의·수집 기능 보존 |

EnemySpawnProfiles.json은 좌표 배치 파일이 아닌 내용 프로필이며 현재 1행이다. EnemySpawns.json의 유일한 행은 DemoRaidMap / placement_id=1 / enemy.quadruped_gun_test / spawn_chance=1.0 / condition_id=always다. 공개 좌표 배치 행은 위 8개 파일 전체에서 0건이다.

공개 `Content/Data/MainRuntimeDefaults/`의 위 8개 배치 파일과 EnemySpawnProfiles.json도 모두 빈 배열이다. Main 전용 데이터가 비었다는 의미는 아니다. BuildFlavor는 Main에서 활성 payload의 같은 파일이 존재하면 우선 사용하고, 없으면 MainRuntimeDefaults를 사용한다.

## 범용 GameplayInteractionSpawns의 지원 타입

현재 코드는 다음 타입을 해석한다. 파일이 비어 있으므로 지원 코드가 있다는 사실과 현재 사용 중이라는 사실을 구분해야 한다.

| 타입 | 현재 설정 내용 | 사용자 기준에 따른 처리 제안 |
| --- | --- | --- |
| level_travel | 과거 타입/클래스 해석 잔재 | 로더 마지막에서 이미 거부. BP 직접 배치 유지, 잔재 정리 |
| extraction_point | 대상 맵, 반경·체류 시간, 연출 | BP 직접 배치와 고정 설정으로 통일. JSON 생성 분기 제거 |
| pickup_item | ItemId·수량·획득 후 제거 | 테스트 BP 허용. 제품용 가변 획득물은 데이터 ID를 참조하는 배치 지점 필요 |
| loot_container | 컨테이너 정의·내용 ID | 정식 확률 배치는 LootContainer 앵커로 통일. 테스트 직접 BP 유지 |
| item_spawn / loot_container_spawn | 상호작용 시 주변에 임의 아이템/상자 생성 | 테스트 도구로 직접 배치 유지. 정식 앵커 배치와 구분 |
| shop | ShopId 및 상호작용 표시 | 레벨 BP + ShopDefinitions.json |
| workbench | WorkbenchId 및 표시 | 레벨 BP/건설 시설 + WorkbenchRecipes.json 등 |
| piggy_bank | 식별자·상호작용 | 직접 BP/건설 시설 유지. 잔액 저장 기능은 별개 |
| periodic_noise_emitter | 메시 정의 ID·경로, 소음 간격·크기·범위 | 위치는 BP. 정식 가변 소음 설정이 필요하면 프로필화 |
| rolling_bomber_spawner | 수량·웨이브·간격·발사·체력 등 | 위치 BP + 웨이브/전투 프로필로 분리할 대상 |
| self_destruct | 지연·범위·피해 | 테스트 직접 배치 유지. 정식 밸런스 값이면 별도 프로필 |
| sandbag_cover | 내구도·통과 거리·형상 | 위치·형상 BP, 정식 가변 내구도 등은 프로필화 검토 |
| explosive_barrel | 체력·파괴 메시·폭발 연출 | 위치·고정 외형 BP, 정식 가변 전투 값은 프로필화 검토 |
| static_mesh_prop | 메시·재질·충돌·좌표 | 고정 소품 직접 배치, 범용 JSON 배치 필요 없음 |
| shooting_practice_dummy | 체력·피해 배율·회복 | 테스트 직접 BP 유지 |
| difficulty_adjustment | 난이도 변경 상호작용 | 테스트/기능 BP 유지, 난이도 정의 JSON은 별개 |

rolling_bomber_spawner, 소음기, 파괴 가능 오브젝트의 제품용 가변 값은 현재 범용 스폰 로더가 Configure 함수로 주입한다. 배치 로더만 삭제하면 직접 배치 액터는 BP/C++ 기본값을 쓰게 되므로, 데이터 제어가 필요한 항목은 먼저 설정 조회 경로를 마련해야 한다.

## 배치 JSON 밖에서 확인한 액터와 생성 경로

- 취수시설: BlockedIntakeScreenActor는 처음부터 레벨 배치용이다. 퀘스트/목표 이벤트 ID, 재료 조건, 외형, 두 진행 상태 ID를 BP가 설정하고 WorldProgressStates로 복원한다. 별도 취수시설 배치 JSON이 없다.
- 다리: WorldProgressActor는 BP 직접 배치와 JSON 생성 모두 가능하다. ProgressObjectId/InfoId, 재료, 수량, 완성 액터 클래스를 직접 설정할 수 있고 완료 시 같은 위치에 완성 액터를 생성한다. 이 교체 동작은 보존해야 한다.
- 문: DoorActor, SlidingDoorActor, PersistentDoorActor는 액터 설정으로 동작한다. 영구 개방 문은 DoorObjectId로 WorldProgressStates를 사용한다. 별도 문 배치 JSON 로더는 확인되지 않았다.
- 본부 입구/출동 지점: LevelTravelInteractableActor 인스턴스는 Destination만 선택하고 실제 맵과 전환 연출은 GameInstance 및 LevelTravelPresentationDataAsset에서 가져온다. 별도 배치 JSON을 제거한다는 이유로 이 공통 연출 데이터까지 없앨 필요는 없다.
- 추출 지점: ExtractionPointActor 자체에 반경·체류 시간·대상·연출 설정이 있으나 범용 JSON 생성 지원은 아직 남아 있다. 따라서 입구와 추출 지점은 현재 정리 수준이 다르다.
- 메모: MemoActor는 직접 배치할 수 있고 BeginPlay에서 획득 여부를 검사해 자신을 제거한다. MemoSpawns 로더를 없애도 MemoDefinitions.json 조회와 AcquiredMemoIds는 보존해야 한다.
- 두더지: 직접 배치 액터가 대화/퀘스트 상호작용을 소유한다. BunkerRuntimeSpawnSubsystem은 별도 좌표 생성 경로이며 캐릭터 기능 자체는 이 서브시스템을 제거하는 것과 별개다.
- 창고·연구 단말·하우징 관리 단말: 각 상호작용 액터 및 기능 서브시스템으로 처리한다. 연구 노드·제작법·아이템 정의는 배치 데이터가 아니다.
- 업적 위치 트리거: 레벨 액터의 LocationId로 위치 이벤트를 보고한다. AchievementDefinitions.json의 업적 조건·보상과 공간 트리거를 분리하는 기존 구조를 유지할 수 있다.
- 크로바 벽걸이: CrowbarWallRackActor의 아이템 ID와 취수시설 진행 ID가 BP/C++ 기본값에 있다. 고정 퀘스트 장치로 둘지, 재사용 가능한 데이터 기반 보급 장치로 둘지는 정식 용도에 따라 결정할 항목이다.
- 사과 상자·토마토·익는 닭·물리 사과·상자 파편: 해당 액터/컴포넌트가 내구도·외형 변화·물리 생성을 처리한다. 전용 배치 JSON 경로는 확인되지 않았다. 테스트/고정 연출물은 유지하고, 정식 가변 전리품·밸런스에 쓰는 경우에만 해당 값을 데이터 프로필로 분리한다.
- 하우징 시설: HousingFacilityDefinitions.json + 세이브의 시설 인스턴스/좌표/회전으로 생성·복원한다. 사용자 건설 위치를 레벨 자산으로 옮기면 안 된다.
- 하우징 기반 액터: 레벨에 영역이 없으면 C++가 [0,0,4], yaw=18에 영역을 생성하고 관리 단말이 없으면 [-420,-560,60]에 생성한다. JSON 외에도 존재하는 초기 배치 우회 경로이므로 정식 맵에서는 직접 BP 배치로 정리할 대상이다.
- 시설 NPC, 반려동물, 적 사망 전리품, 플레이어가 버린 아이템, 투사체·폭발·파편·카메라 등은 플레이/표현 과정에서 생성된다. 초기 맵 좌표 배치 제거 범위에 포함하지 않는다.

## 정리 전에 해결할 결합과 불일치

1. **테스트 BP를 런타임에서 지우는 코드.** EnemySpawnSubsystem.cpp:844 부근의 범용 로더는 동일 spawn_id 태그, 액터 이름/접두어, 에디터 라벨, 동일 클래스의 2cm 이내 좌표를 기준으로 기존 액터를 숨기고 충돌을 끈 뒤 Destroy한다. JSON 좌표를 권위로 삼던 이 동작은 테스트 직접 배치 예외와 충돌한다. 단순히 JSON을 빈 배열로 두는 것으로는 미래 재활성화를 막지 못한다.
2. **전투 프로필 의존.** RaidPlacementSubsystem은 기존 EnemySpawnSubsystem::TryGetEnemyCombatProfile을 호출한다. EnemySpawnSubsystem을 통째로 삭제하면 새 앵커도 망가진다. EnemyCombatProfiles 로더를 남기거나 별도 데이터 서브시스템으로 분리해야 한다.
3. **지도 마커 의존.** MapWidget::RefreshMapOverlayData는 EnemySpawnSubsystem::GetMapOverlaysForWorld만 조회하며 이 함수는 GameplayInteractionSpawns의 mapOverlay에서만 결과를 만든다. 고정 입구·추출 액터/컴포넌트에서 마커를 등록하도록 옮겨야 한다. 마커만 남기려고 좌표 배치 JSON을 존치하면 위치 SSOT가 다시 갈린다.
4. **레벨 전환 호출부.** LevelTransitionSubsystem.cpp:262에서 기존 런타임 스폰 Ensure 함수를 직접 호출한다. 맵 로드 delegate와 함께 정리해야 한다.
5. **식별자와 저장.** 다리의 ProgressObjectId가 비면 GetFName으로 대체한다. 배치 변경/복제/이름 변경 시에도 기존 저장을 유지하려면 명시적인 안정 ID를 사용해야 한다. 취수시설의 두 상태 ID, 문 ID, MemoId, 퀘스트 이벤트 참조를 보존한다. InfoId는 현재 JSON 행을 자동 조회하는 키가 아니라 상태 정보 식별자다.
6. **앵커 확률 단위 불일치.** RaidPlacementSubsystem::ReadSpawnChance는 0~1 float를 읽고 현재 행도 1.0이다. Docs/SSOT/data_authoring_conventions.md는 확률을 0~10000 정수로 정의한다. 6500을 현재 코드에 넣으면 1로 clamp되어 100%가 된다. 데이터와 로더를 동시에 전환해야 한다.
7. **레이드 시드 연결 누락.** 공개 소스 검색에서 SetRaidSeed의 정의 외 호출이 확인되지 않았다. 미설정 시 seed=0을 사용하므로 확률을 낮춰도 레이드마다 새 추첨이 된다고 볼 수 없다. 레이드 세션 소유자의 시드 전달이 필요하다.
8. **조건 평가 미완성.** condition_id는 비어 있거나 always만 통과하고 그 외는 경고 후 생성하지 않는다. 퀘스트 조건 지원이 이미 구현된 것으로 작성하면 안 된다.
9. **앵커의 적용 월드 범위.** 새 앵커 서브시스템은 현재 선택된 레이드 맵에만 적용된다. 벙커·독립 테스트 맵까지 전환하려면 이 범위를 결정해야 하며 현재 모든 맵에서 작동한다고 가정하면 안 된다.
10. **중복 스키마의 부수 오류.** 기존 LootContainerSpawns 로더는 앵커 행을 skip하지만 유효 좌표 행이 하나도 없으면 비어 있지 않은 파일을 실패로 판정한다. 새 앵커는 별도로 처리하더라도 레거시 오류 로그가 생길 수 있다. 좌표 로더 제거로 함께 없앨 수 있다.
11. **기존 안내서 충돌.** runtime_actor_spawns.md에는 상호작용은 레벨에 놓지 않는다는 옛 설명과 이미 폐지한 level_travel 예시가 남아 있다. quest_and_runtime_actor_data_authoring_guide.md는 좌표 JSON 복구를 안내하며, raid_placement_anchors.md에는 앵커 미배치 설명이 남아 있다. 정리 시 현행 안내를 하나로 모으고 과거 기록과 분리해야 한다.

## 권장 정리 순서와 검증

1. 본 문서의 소유권 구분을 합의한 뒤 SSOT로 확정한다. 테스트 BP 배치 예외는 유지한다.
2. 에디터에서 실제 대상 맵의 액터 클래스·Transform·PlacementId·진행 ID·데이터 참조를 읽기 전용으로 덤프하여 중복과 누락을 확인한다. Main 전용 데이터는 보호 경계 안에서 별도 점검한다.
3. 전투 프로필과 지도 마커 소비자를 먼저 분리한다. 고정 공간을 C++가 보충 생성하는 하우징 fallback도 정식 레벨 배치로 옮긴다.
4. 필요한 가변 설정을 기존 기능 JSON 또는 역할별 프로필로 이전한다. 범용 JSON을 없애면서 모든 설정을 BP 인스턴스로 옮기지 않는다.
5. 적·상자 좌표 행 지원과 범용 초기 배치·메모 배치·벙커 캐릭터 배치·장애물/워프/진행 오브젝트 배치 로더를 제거한다. 대상 액터, 테스트용 Configure API, 내용 데이터와 저장 기능은 사용처를 확인해 보존한다.
6. 사용되지 않는 배치 파일, 공개 fallback 목록, 문서·테스트·도구의 참조를 함께 정리한다. EnemySpawns/LootContainerSpawns와 내용 프로필은 앵커 계약을 위해 유지한다.
7. 검증은 직접 배치 BP가 삭제되지 않는지, 앵커/프로필 일치 및 누락 오류, 확률 0/10000/중간값과 레이드 시드, 다리·취수·문의 저장 복원, 메모 재등장 차단, 입구/추출 이동과 지도 마커, 건설 시설 복원을 포함한다. UE 5.7 빌드 후 해당 맵 PIE와 패키지의 데이터 포함 상태도 확인한다.

현재 공개 배치가 거의 비어 있어 좌표 방식의 제거 비용은 낮다. 다만 활성 데이터가 없다는 사실만으로 소비 기능까지 미사용이라고 판단해서는 안 된다. 분석 단계에서는 게임 코드·JSON·레벨 자산을 수정하지 않았다.

## 주요 근거 파일

- `TunaSweeper/Source/TunaSweeper/Public/Raid/TunaSweeperRaidPlacementAnchor.h`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperRaidPlacementSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperBunkerRuntimeSpawnSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperMemoSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperHousingSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperMapWidget.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Settings/TunaSweeperBuildFlavor.cpp`
- `TunaSweeper/Source/TunaSweeper/Public/Interaction/` 및 대응 `Private/Interaction/` 액터 구현
- `Tools/RaidPlacement/place_quadruped_enemy_spawn.py` (읽기만 수행, 실행하지 않음)
- `Docs/save_persistence.md`, `Docs/runtime_actor_spawns.md`, `Docs/raid_placement_anchors.md`, `Docs/SSOT/data_authoring_conventions.md`
