# JSON 기반 게임 요소 제어 조사

분석일: 2026-09-06. 현재 기본 작업 폴더의 공개 소스·데이터를 정적으로 확인한 결과다. 별도 worktree에서 진행 중인 배치 제거·메모 앵커 전환·하우징 제거·연구 문자열 전환을 완료된 것으로 간주하지 않았다. 게임 코드와 데이터는 수정하지 않았다.

## 조사 범위

이미 조사한 8개 배치 JSON을 제외하면 `TunaSweeper/Content/Data/` 바로 아래에 JSON 22개가 있다. 그중 21개는 게임 실행 경로의 내용·규칙·표시·형상 데이터이고, `FMSoundPresets.json` 1개는 에디터 도구용이다. 아래 개수는 파일의 현재 정의 수이며, 실제 맵에서 사용되는 인스턴스 수나 모든 정의의 게임플레이 검증 결과가 아니다.

## 현재 파일과 연결 키

| 파일 | 정의 수 | 조회 키 | 제어하는 요소 | 소비 경로 |
| --- | ---: | --- | --- | --- |
| ItemTable.json | 48 | id (ItemId) | 이름/설명 키, 등급, 아이콘, 기준 가격, 경험치, 무게, 장비 슬롯, 탄종·부착물 호환, 탄창·재장전, 방어·탄약 피해 보정, 소비 회복량·사용 시간 등 | ItemDataSubsystem → 인벤토리·장비·사용·무기·상점 |
| ItemStackDefinitions.json | 8 | stack category key | 범주별 최대 겹치기 수량 | ItemTable의 max_stack_category_key 또는 C++의 범주 기본 매핑 → ItemDataSubsystem |
| WeaponActorClassMappings.json | 6 | item_id | 아이템에 대응하는 실제 무기 BP 클래스 | ItemDataSubsystem::TryGetWeaponActorClassPath → 캐릭터 무기 선택 |
| LootContainerTable.json | 9 | id (ContainerDefinitionId) | 상자 이름 키, 용량, 메시·재질·메시 스케일 | LootContainerActor → ItemDataSubsystem |
| LootContainerContents.json | 12 | id (ContentsId) | 지급 아이템, 고정 또는 최소/최대 수량, 항목별 드롭 확률 | ItemDataSubsystem의 상자 내용 생성. 일반 상자와 적 전리품이 참조 |
| ShopDefinitions.json | 1 | shop_id | 판매 목록, 초기 재고, 개별 가격 오버라이드, 상점명 키 | ShopActor의 ShopId → ItemDataSubsystem → GameInstanceShop |
| WorkbenchRecipes.json | 2 | recipe_id / workbench_id | 제작 재료·수량, 산출 아이템·수량, 기본 해금 여부, 이름 키 | ItemDataSubsystem → 제작 UI/게임 로직 |
| WorkbenchDismantleRecipes.json | 2 | source_item_id | 분해 대상과 결과 재료·수량 | ItemDataSubsystem → 분해 로직 |
| EnemySpawnProfiles.json | 1 | profile_id | 적 클래스·외형 참조, 체력, 무기·탄약, 전리품, 경험치, 진영·분대, combat_profile_id | 배치 앵커 JSON → RaidPlacementSubsystem |
| EnemyCombatProfiles.json | 4 | profile_id | 공격 방식/역할, 이동·추적·교전 거리, 조준·관찰·재장전 대기, 회전, 발사 허용 각도, 정확도, 연사·재배치 패턴 | EnemySpawnSubsystem의 프로필 조회 → 적 전투 설정 |
| DebuffDefinitions.json | 3 + 전역 설정 | debuff_id | 적용 확률·지속시간·틱 피해·아이콘·카메라 반응, 전역 틱 간격, 근력/중량 과다·이동 불가 임계치 | DebuffDataSubsystem → 캐릭터 상태/중량 처리 |
| ExperienceLevelTable.json | 30 | level | 레벨별 누적 경험치 문턱 | GameInstanceExperience |
| ExperienceLevelRewards.json | 29 | level | 레벨별 최대 체력·음식·수분·스태미나 추가량 및 근력 추가량 | GameInstanceExperience → 캐릭터 능력치 |
| StatResearchNodes.json | 13 | node_id | 연구 트리 행·열·연결, 개방에 필요한 적용 노드 수, 소요 시간, 효과, 아이콘·문구 | ResearchSubsystem → 연구 화면/능력치 |
| QuestDefinitions.json | 1 | quest_id / objective_id / provider_id | 제공자·선행 조건·목표 종류/필터·필요 횟수·보상·수락/보상 대사 연출 | QuestSubsystem. 본문은 QuestTextStrings.csv |
| ScenarioDefinitions.json | 1 | scenario_id / trigger / completion_flag | 트리거, 대상 레벨, 필수/차단 진행 플래그, 퀘스트 상태 조건, 우선순위·지연·일회성, 대사 및 카메라 초점 | ScenarioSubsystem. 본문은 ScenarioTextStrings.csv |
| MemoDefinitions.json | 20 | memo_id | 메모 제목·본문 | MemoSubsystem → 메모 UI |
| AchievementDefinitions.json | 0 | achievement_id / target_id | 달성 조건 종류·대상·횟수, 플랫폼 업적 ID 매핑 | AchievementSubsystem. 로더/평가기는 존재하지만 현재 정의는 없음 |
| DifficultyDefinitions.json | 3 | difficulty_stage | 난이도 선택 화면의 제목·설명 | IntroMenuWidgetDifficulty만 JSON을 읽음 |
| PeriodicNoiseEmitterMeshes.json | 1 | mesh definition id | 소음기 메시의 박스/혼 파트 구성·치수·상대 위치·색·재질 | PeriodicNoiseEmitterActor의 메시 정의 로더 |
| HousingFacilityDefinitions.json | 4 | facility_id | 시설 종류·클래스·격자 크기·제작 재료·해금·표시 | HousingSubsystem. 사용자 지시에 따라 별도 작업에서 제거 대상 |
| FMSoundPresets.json | 6 | preset | FM 합성 소리 생성 프리셋 | TunaSweeperEditor의 FMSoundTool. 게임 런타임 JSON 로더가 아님 |

## 기존 배치 방식과 같은 ID 연결 사례

- 상점: 레벨의 ShopActor에 ShopId → ShopDefinitions → 상품 ItemId → ItemTable → 이름 CSV.
- 상자: 레벨의 배치 앵커 ID → LootContainerSpawns → ContainerDefinitionId / ContentsId → 상자 형상·용량 / 지급 아이템.
- 적: 레벨의 배치 앵커 ID → EnemySpawns.profile_id → EnemySpawnProfiles → combat_profile_id 및 전리품 IDs → 전투 프로필 / 상자 내용.
- 제작: WorkbenchId → 해당 제작법 → 입력/출력 ItemId. 설계도 아이템의 blueprint_recipe_id는 해금할 recipe_id를 참조한다.
- 메모: 사용자가 요청한 새 방식은 배치 앵커 ID → MemoSpawns.memo_id → MemoDefinitions의 제목·본문. 배치 ID와 수집 ID는 서로 다른 역할이다.
- 업적: 레벨의 AchievementLocationTrigger.LocationId → 업적 정의 target_id. 이미 공간 트리거와 조건 정의가 구분되어 있다.

따라서 설정마다 별도 좌표 배치 JSON을 만드는 대신, 위치가 고정된 액터가 해당 기능의 안정 ID를 참조하는 방식으로 정리할 수 있다. 기존 기능 정의 JSON은 배치 JSON 제거와 함께 지우는 대상이 아니다.

## SSOT 관점에서 추가 확인한 항목

### 1. 전리품 확률에 서로 다른 숫자 표기가 혼재한다

ItemDataSubsystem.cpp:45의 NormalizeDropChanceValue는 값이 1보다 크면 10000분율로 나누고, 0~1 값은 그대로 확률로 사용한다.

| JSON 원본값 | 현재 해석 | 10000분율 SSOT 기준 |
| ---: | --- | --- |
| 0 | 0% | 0% |
| 1 | 100% | 0.01% |
| 2 | 0.02% | 0.02% |
| 5000 | 50% | 50% |
| 10000 | 100% | 100% |

따라서 특히 값 1이 충돌한다. 이전 분석의 앵커 spawn_chance는 또 0~1 전용이므로, 확률 데이터 계약과 각 로더를 함께 정리할 필요가 있다. 현재 LootContainerContents에서 확률을 생략하면 기본 100%이며, 이 표는 기존 데이터의 실제 아이템 드롭 오류를 재현한 결과가 아니라 코드의 입력 해석 결과다.

### 2. 난이도 파일은 밸런스 정의가 아니다

DifficultyDefinitions.json은 title/description만 제공한다. 현재 C++에서 선택 단계는 저장과 메뉴 표시로 이어지며, 이 JSON에서 적 체력·피해·전리품 배율을 읽는 경로는 확인되지 않았다. 따라서 파일에 임의로 enemy_health_multiplier 같은 값을 추가해도 현 로더에는 반영되지 않는다. 실제 난이도 효과의 전체 존재 여부를 확정하려면 BP 그래프까지 별도로 확인해야 한다.

난이도 제목/설명은 CSV 키를 거치지 않고 FText::FromString으로 표시된다. CSV 문자열 시스템 정리의 추가 대상이다.

### 3. 메모 제목·본문도 아직 문자열 키 방식이 아니다

MemoSubsystem.cpp:130 부근은 memo_id, title, body를 읽고 제목과 본문을 직접 FText::FromString으로 만든다. MemoId는 수집/조회 식별자이며 title_string_key/body_string_key 조회를 현재 대신 수행하지 않는다. 메모 앵커 전환과 본문 현지화는 별도의 변경 항목으로 구분해야 한다.

연구 문구의 CSV 전환은 이미 별도 대화에 요청된 작업이며 이번 조사에서 중복 구현하지 않았다.

### 4. 카메라 초점 위치가 콘텐츠 JSON에 직접 들어 있다

QuestSubsystem.cpp:229와 ScenarioSubsystem.cpp:298은 camera_focus_location을 월드 좌표로 읽는다. 대사·트리거가 JSON인 것은 기존 설계와 맞지만, 맵의 고정 초점 위치는 레벨 배치 변경과 어긋날 수 있다. 공간의 SSOT를 레벨로 확장하려면 카메라 초점 앵커 ID나 대상 액터 ID 참조로 바꿀 후보이다. 아직 변경을 요청받거나 구현한 상태는 아니다.

### 5. 소음기 JSON은 소음 규칙보다 형상을 정의한다

PeriodicNoiseEmitterMeshes.json의 parts는 메시를 구성하는 박스/혼의 치수·방향·색을 직접 정의한다. NoiseIntervalSeconds, NoiseLoudness, NoiseMaxRange는 액터 속성이며, 옛 GameplayInteractionSpawns 로더도 이 값을 주입할 수 있다. 파일명과 실제 역할대로 형상 데이터와 소음 설정을 구분해야 한다.

고정 외형은 BP/메시 에셋으로 옮길 후보이고, 소음 간격·강도·범위를 정식 밸런스로 바꿀 필요가 있으면 별도 역할별 프로필에 두는 편이 사용자 기준에 맞다. 파일을 단순히 소음 밸런스 설정이라고 설명하면 부정확하다.

### 6. 상점 가격과 성장량에는 코드가 결정하는 해석 규칙이 있다

- ShopDefinitions의 price가 있으면 플레이어 구매 가격으로 사용하고, 없으면 ItemTable.shop_sell_price를 사용한다(ItemDataSubsystem.cpp:491).
- 플레이어가 상점에 파는 금액은 ItemTable.shop_sell_price × 수량 ÷ 2의 정수 계산이다(GameInstanceShop.cpp:200). 필드명만 보고 최종 판매 가격을 뜻한다고 해석하면 안 된다. 절반 판매 규칙은 JSON에 없다.
- ExperienceLevelRewards의 max_health_increase 등은 ToRatioFloat로 10000을 나눈 값을 누적해서 능력치에 더한다(GameInstanceExperience.cpp:90). 10500은 현 코드에서 추가량 1.05로 계산되며 현재 최대치에 1.05배를 곱하는 방식이 아니다. carry_strength_increase는 나누지 않는 원본 추가량이다.
- 연구 시간은 JSON에서 읽어도 현재 로더가 1~3600초로 제한하며 행당 노드가 3개를 넘으면 실패한다(ResearchSubsystem.cpp:81,110). 데이터에 큰 값을 적는 것만으로 코드 제약을 바꿀 수 없다.

### 7. JSON이 무기·전투 설정 전체를 소유하지는 않는다

ItemTable은 탄창·재장전·탄약 피해 보정 등을, WeaponActorClassMappings는 클래스 선택을 소유한다. 반면 발사 간격 FireCooldown, 산탄 수 ShotgunProjectileCount 등은 Weapon 액터의 EditDefaultsOnly 값이다. 확산·리코일은 WeaponSpreadRecoilDataAsset이 총종 태그로 조회한다. 메시·총구·사운드 연출은 WeaponPresentationDataAsset 등 별도 에셋 경로다.

SSOT 정리는 같은 값을 여러 곳에서 덮어쓰지 않게 역할을 명확히 하는 것이 우선이다. 기존 DataAsset 전체를 JSON으로 변환하는 작업이 이미 요청된 것으로 취급하지 않는다. 전투 밸런스를 모두 JSON으로 통일하려면 무기 BP/DA와 적용 순서를 추가 조사해야 한다.

### 8. 내용 데이터와 런타임 결과를 구분해야 한다

상점의 초기 재고, 전리품 후보, 제작 재료, 퀘스트 정의는 JSON 원본이다. 구매 후 남은 재고, 추첨된 상자 내용, 연구 진행, 퀘스트 완료, 메모 획득은 런타임 또는 세이브 상태다. 플레이 결과를 정의 JSON에 다시 기록하는 구조가 아니다.

대부분의 정의 로더는 캐시와 bForceReload 방식이며, 에디터 실행 중 파일 저장만으로 모든 시스템이 즉시 갱신된다고 볼 수 없다. 실제 변경 작업에서는 데이터 갱신 시점과 이미 생성된 인스턴스의 적용 범위를 별도로 검증해야 한다.

## 게임 규칙 JSON과 구분할 것

- FMSoundPresets.json: 에디터 FM 소리 제작 도구 설정. 생성한 오디오 에셋이 게임에 들어간다.
- Tools/CombatMovementSimulator/combat_tuning.json: 별도 시뮬레이터 설정. UE 런타임 전투 프로필 원본과 동일 파일이 아니다.
- Tools/QuestStudio/data의 authoring pack: 작성 도구의 원본/동기화 대상. UE 런타임은 Content/Data의 QuestDefinitions 및 문자열 CSV 등 지정된 산출물을 읽는다.
- 아이콘 cropInfo, package.json, tsconfig.json 등: 제작 메타데이터와 개발 도구 설정이며 게임 규칙이 아니다.
- LedExpressionPresets.txt와 문자열 CSV 4개도 데이터 기반 제어이지만 JSON은 아니다.
- Main payload manifest는 빌드 플레이버별 시작/레이드 맵 경로 선택에 관여한다. Main 전용 원본 내용은 이번 조사에서 열람하지 않았다.
- 설정 INI, 전환/무기/발자국/피격/가림 처리 DataAsset은 별도의 설정 원본이다.

## 후속 점검 우선순위 제안

1. 아이템·상자·상점·제작의 ID 연결과 확률/가격 단위: 현재 실제 사용 데이터가 많고 여러 시스템이 공유한다.
2. 무기·적 전투 설정의 JSON/BP/DataAsset 소유권: 한 변경이 어느 값을 최종 적용하는지 명확히 해야 한다.
3. 메모·난이도 문구의 CSV 연결: 연구 문자열 외에도 남아 있는 직접 문구 경로다.
4. 퀘스트·시나리오 카메라 초점 앵커화와 고정 소음기 형상: 앞서 정한 공간·외형 소유권과 같은 기준으로 판단한다.

## 주요 근거

- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceShop.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceExperience.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperRaidPlacementSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperDebuffDataSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperResearchSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperQuestSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperScenarioSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperMemoSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperAchievementSubsystem.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidgetDifficulty.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperPeriodicNoiseEmitterActor.cpp`
- `TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperFMSoundTool.cpp`
- `TunaSweeper/Source/TunaSweeper/Public/Weapon/TunaSweeperWeapon.h`
- `TunaSweeper/Source/TunaSweeper/Public/Weapon/TunaSweeperWeaponSpreadRecoilDataAsset.h`
- `Docs/SSOT/data_authoring_conventions.md`
