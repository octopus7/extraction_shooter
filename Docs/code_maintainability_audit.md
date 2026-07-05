# Code Maintainability Audit

이 문서는 에이전트로 빠르게 누적된 코드 중 사람이 유지보수하기 어려운 지점을 정적 검토 기준으로 정리한 것이다. 목표는 단순한 코드 미관 개선이 아니라, 새 기능을 붙일 때의 리드타임과 회귀 위험을 줄이는 것이다.

## 우선순위 기준

- P0: 현재 데이터 로드, 실행, 빌드, 패키징을 깨는 즉시 수정 대상.
- P1: 기능 추가 때 여러 도메인을 동시에 건드리게 만들어 리드타임과 회귀 위험을 크게 키우는 구조.
- P2: 당장 깨지지는 않지만 기능 수가 늘수록 유지보수 비용이 누적되는 구조.
- P3: 다른 정리와 함께 처리하면 좋은 국소 개선.

## 요약

| 우선순위 | 영역 | 핵심 문제 | 권장 해결 |
| --- | --- | --- | --- |
| P0 | 데이터 파일 | `LootContainerContents.json` 파싱 실패로 `LoadItemData()` 전체 실패 가능 | JSON 복구, `Content/Data` 파싱 검증 자동화 |
| P1 | 저장/런타임 상태 | `UTunaSweeperGameInstance`가 저장, 인벤토리, 상점, 작업대, 하우징, 경험치를 모두 소유 | 도메인별 save snapshot/export/import 분리 |
| P1 | 런타임 스폰 | `UTunaSweeperEnemySpawnSubsystem`과 `GameplayInteractionSpawns.json`이 다수 타입을 단일 union/parser로 처리 | spawn type별 parser/handler와 공통 row reader 분리 |
| P1 | 데이터 로더 | JSON/CSV 파서, alias, vector/rotator reader가 여러 파일에 중복 | 공용 data reader/schema validator 추가 |
| P1 | UI | HUD/Intro/ItemContainer/Workbench 위젯이 화면 생성, 상태 전환, 도메인 조회를 동시에 처리 | shell/presenter/view-model/하위 위젯 분리 |
| P1 | AI/전투 | RollingBomber, EnemyAIController, EnemyCharacter가 상태 머신, 비주얼, 전투 수치를 직접 소유 | combat profile/data asset, 행동/비주얼/피해 컴포넌트 분리 |
| P2 | 월드 진행/하우징 | 저장 상태, 액터 라이프사이클, 특수 해금 규칙이 여러 클래스에 분산 | WorldProgress/Housing runtime manager 역할 정리 |
| P2 | 런타임 데이터 | 검증용 퀘스트, debug piggy bank, test noise emitter, sample row가 런타임 데이터에 남아 있음 | dev fixture 분리와 packaging 검증 |
| P2 | 에셋 참조 | C++와 JSON에 `/Game/...` 경로와 generated asset 이름이 직접 박혀 있음 | asset manifest 또는 PrimaryAsset alias 검증 |
| P2 | 런타임 UMG/텍스트 | C++ `ConstructWidget`, 고정 색/여백/크기, 키 프롬프트/문구 하드코딩 반복 | WBP/스타일 data asset/StringTable/input glyph로 이동 |
| P3 | 효과/무기 디버그 | 작은 effect actor Tick, weapon debug draw/log가 런타임 경로에 섞임 | 공통 burst component/Niagara, debug 기본 off 및 shipping 제외 |

## P0. 데이터 파싱 실패

### 문제

`TunaSweeper/Content/Data/LootContainerContents.json`에 닫히지 않은 문자열이 있어 JSON 파싱이 실패한다. 현재 `UTunaSweeperItemDataSubsystem::LoadItemData()`는 아이템 테이블, 스택, 이름 CSV, 루팅, 상점, 작업대 데이터를 한 번에 로드하므로, 이 파일 하나의 문법 오류가 전체 아이템 데이터 로드 실패로 전파될 수 있다.

근거:

- `TunaSweeper/Content/Data/LootContainerContents.json`: `id` 8004 이후 여러 `memo_ko` 문자열의 닫는 따옴표 누락.
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp:140`: `LoadItemData()`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp:1134`: `LoadLootContainerContentsJson()`

직접 검증 결과:

```powershell
Get-ChildItem .\TunaSweeper\Content\Data -File -Filter *.json |
  ForEach-Object {
    Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json | Out-Null
  }
```

현재 `LootContainerContents.json`만 실패한다.

### 해결

1. `LootContainerContents.json`의 깨진 `memo_ko` 문자열을 복구한다.
2. `Tools` 또는 editor commandlet에 `Content/Data/*.json`과 CSV 헤더/행 검증 스모크 테스트를 추가한다.
3. 에디터 시작 또는 CI에서 데이터 검증이 실패하면 명확히 fail-fast 한다.

### 검증

- 모든 `Content/Data/*.json`이 `ConvertFrom-Json` 또는 C++ 로더로 통과.
- `UTunaSweeperItemDataSubsystem::LoadItemData()`가 true 반환.
- 루팅 컨테이너 스폰 및 열기, 상점, 작업대 레시피 조회 확인.

## P1. GameInstance의 저장/인벤토리 god class

### 문제

`UTunaSweeperGameInstance`가 저장 슬롯, 인벤토리/장비/퀵슬롯, 창고, 상점 재고, 작업대, 하우징, 월드 진행, 메모, 지도 마커, 경험치까지 한 파일에서 처리한다. 저장 필드를 하나 추가할 때 `SaveGame`, load, save, default state, migration, UI 갱신까지 같이 추적해야 하므로 사람이 변경 범위를 예측하기 어렵다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstance.cpp:4121`: `LoadGameState()`
- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstance.cpp:4382`: `SaveGameStateInternal()`
- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstance.cpp:4647`: `GenerateDefaultInventoryState()`
- `TunaSweeper/Source/TunaSweeper/Public/Inventory/TunaSweeperSaveGame.h:84`: 저장 컨테이너가 다수 도메인 배열을 직접 보유

### 해결

1. `FTunaSweeperSaveSnapshot` 또는 도메인별 snapshot 구조를 둔다.
2. Inventory, Storage, Shop, WorldProgress, Housing, Workbench, Memo/MapMarker, Experience가 각각 `ExportForSave`, `ImportFromSave`, `ResetForNewGame`, `NormalizeAfterLoad`를 갖게 한다.
3. `GameInstance`는 active slot orchestration과 save 파일 입출력만 담당하게 축소한다.
4. `Docs/save_persistence.md`의 항목과 코드 export/import 항목을 1:1로 맞춘다.

### 검증

- 새 슬롯 생성, 이어하기, 슬롯 삭제.
- RaidMap 사망 저장, RaidMap to BunkerMap 추출 저장, Bunker UI 거래 저장 지연 flush.
- 인벤토리/장비/퀵슬롯/창고/상점/작업대/하우징/퀘스트/메모/월드 진행 저장-로드 회귀.

## P1. 런타임 스폰 시스템의 union parser

### 문제

`UTunaSweeperEnemySpawnSubsystem`이 적, 루팅 컨테이너, 투명 장애물, 월드 진행 오브젝트, 워프 포인트, gameplay interaction actor까지 한 클래스에서 로드하고 스폰한다. 특히 `GameplayInteractionSpawns.json`은 레벨 이동, 추출 지점, 픽업, 상점, 작업대, 자폭, rolling bomber spawner, sandbag, barrel, static mesh prop, practice dummy, noise emitter 같은 타입을 단일 struct와 거대 parser/switch로 처리한다.

근거:

- `TunaSweeper/Source/TunaSweeper/Public/Subsystem/TunaSweeperEnemySpawnSubsystem.h:66`: 스폰 데이터 로더가 한 subsystem에 집중
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:469`: 여러 스폰 데이터를 한 흐름에서 로드/스폰
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:1418`: gameplay interaction row 파싱
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:2119`: gameplay interaction actor 구성

### 해결

1. 공통 필드(`level_name`, `spawn_id`, transform, editor-only, map overlay)는 shared row reader로 분리한다.
2. `spawn_type`별 `ISpawnRowParser`와 `ISpawnActorConfigurator`를 등록형으로 둔다.
3. legacy alias와 default asset path는 타입별 config에 가두고, 새 타입 추가 시 기존 타입 parser를 건드리지 않게 한다.
4. 에디터 배치 cleanup은 runtime spawn과 분리해 editor migration command로 옮긴다.

### 검증

- `GameplayInteractionSpawns.json` 전체 row schema validation.
- BunkerMap/RaidMap runtime spawn 수와 stable spawn id 중복 방지.
- 맵 오버레이, level travel, extraction, loot, shop, workbench, static mesh prop, noise emitter 동작.
- 정리된 맵과 구버전 맵에서 중복 스폰/오삭제가 없는지 확인.

## P1. 공용 데이터 로더와 schema validator 부재

### 문제

JSON/CSV 파일 로드, 필드 alias, vector/rotator/color 파싱, level name 정규화가 여러 subsystem에 복제되어 있다. 에이전트가 기능을 붙일 때마다 가까운 파일에 로더를 하나 더 추가한 흔적이 강하고, 오류 처리 기준이 파일마다 달라질 수 있다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:105`: vector/rotator reader
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperMemoSubsystem.cpp:41`: 유사 vector/rotator reader
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperBunkerRuntimeSpawnSubsystem.cpp:36`: 유사 vector/rotator/color reader
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp:634`: item table JSON parser
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperQuestSubsystem.cpp:303`: quest data loader

### 해결

1. `TunaSweeperDataReader` 같은 공용 C++ 유틸을 추가한다.
2. 필수 필드, 선택 필드, alias, default, clamp, enum 변환을 한 곳에서 선언적으로 읽게 한다.
3. 각 JSON/CSV 파일별 validator를 두고 cross-reference까지 검사한다.
4. editor/dev에서는 데이터 오류를 fallback으로 숨기지 말고 실패를 명확히 표시한다.

### 검증

- 모든 JSON/CSV 문법 검증.
- 필수 필드 누락, 타입 오류, alias 충돌, 음수/범위 오류 테스트.
- item id/name key/icon/recipe/loot/shop/quest/memo/spawn class 참조 무결성 검사.

## P1. UI 위젯의 과도한 책임

### 문제

HUD, Intro, ItemContainer, Workbench 위젯이 화면 생성, 상태 전환, 입력 처리, 도메인 조회, 스타일 적용을 동시에 수행한다. 런타임 `ConstructWidget`, `FindWidget`, 하드코딩 색/여백/크기, 직접 `GameInstance` 조회가 섞여 있어 디자이너나 개발자가 화면 구조를 한 곳에서 파악하기 어렵다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp:397`: 매 tick HUD 하위 상태 갱신
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp:1907`: HUD mode visibility 통합 처리
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidget.cpp:167`: intro menu construct
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidget.cpp:2626`: 난이도 JSON 로드가 intro widget 내부에 존재
- `TunaSweeper/Source/TunaSweeper/Private/UI/ItemContainerWidget.cpp:1617`: 컨테이너 모드별 처리 집중
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperWorkbenchPanelWidget.cpp:1047`: 작업대 모드/드롭 처리 집중

### 해결

1. `GameHudWidget`은 HUD shell로 축소하고 panel별 presenter/view-model을 둔다.
2. Intro는 SaveSlot, Difficulty, Settings, Credits 하위 위젯과 서비스로 분리한다.
3. `ItemContainerWidget`은 source별 provider를 받아 렌더링만 담당하게 한다.
4. style 값과 icon path는 theme/data asset으로 중앙화한다.
5. 키 프롬프트는 문자열 상수가 아니라 input binding/platform glyph에서 생성한다.

### 검증

- HUD 모드 전환: inventory, shop, storage, workbench, quest, memo, map.
- reload/crosshair, damage number, headphone noise ripple.
- 새 게임/이어하기/슬롯 삭제/난이도/언어/해상도/크레딧.
- 한/영/일 긴 문구와 720p/1080p/울트라와이드 스크린샷.

## P1. AI/전투 actor의 상태 머신과 수치 하드코딩

### 문제

`ATunaSweeperRollingBomber`, `ATunaSweeperEnemyAIController`, `ATunaSweeperEnemyCharacter`가 상태 전이, 이동, 공격, 비주얼, IK, 드롭, 무기/탄약 선택을 코드에서 직접 처리한다. 빠르게 프로토타입을 만드는 데는 유리하지만 새 적 타입이나 밸런스 변경 때 코드 변경 범위가 커진다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperRollingBomber.cpp:224`: Tick 기반 상태 업데이트
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperRollingBomber.cpp:624`: projectile attack mode
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperRollingBomber.cpp:814`: leg IK
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyAIController.cpp:217`: ranged combat state
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp:303`: enemy weapon runtime 초기화
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp:653`: 공격/드롭 계열 로직

### 해결

1. enemy archetype 또는 combat profile data asset으로 공격 타입, 무기, 탄약, 드롭, XP, VFX, debuff 값을 정의한다.
2. RollingBomber는 행동 상태, 폭발/피해, 비주얼/IK를 컴포넌트 또는 helper로 분리한다.
3. EnemyAIController는 전이 테이블 또는 Behavior Tree/전략 컴포넌트로 이동한다.
4. projectile은 damageable/presentation interface를 통해 대상별 분기를 줄인다.

### 검증

- RollingBomber projectile mode, roll charge, self-destruct, explosion damage, leg IK.
- 총기 적/근접 적의 추격, 후퇴, 엄폐물 line-of-fire, 재장전, 사망 드롭.
- projectile이 적, 플레이어, 배럴, 샌드백, 더미에 주는 피해와 HUD damage number.

## P2. 월드 진행/하우징 저장 책임 분산

### 문제

다리 수리, 영구 문, housing facility가 모두 저장 대상이지만 상태 전이와 저장 요청이 액터, subsystem, GameInstance에 나뉘어 있다. 특히 월드 진행은 동일한 save container를 공유하면서 각 액터가 키, 초기값, 완료 처리, replacement spawn을 직접 판단한다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperWorldProgressActor.cpp:201`
- `TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperPersistentDoorActor.cpp:107`
- `TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstance.cpp:3606`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperHousingSubsystem.cpp:880`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperHousingSubsystem.cpp:1490`

### 해결

1. `WorldProgressSubsystem`이 상태 전이와 저장 요청을 소유한다.
2. 액터는 progress id와 interaction command만 전달한다.
3. Housing은 definition 기반 factory와 actor lifecycle을 subsystem/manager에 모으고, GameInstance는 저장 snapshot만 보유한다.
4. 임시 해금 규칙은 dev/progression rule로 표시하거나 퀘스트 보상 기반으로 이동한다.

### 검증

- 다리 수리, 영구 문 열기, 저장-로드, 맵 전환.
- housing 배치/보관/재배치, NPC 표시, workbench/piggy bank id 유지.

## P2. 런타임 데이터에 남은 테스트/디버그 흔적

### 문제

검증용 퀘스트, debug piggy bank, test noise emitter, sample 이름 등이 런타임 데이터에 남아 있다. 개발 중에는 유용하지만 패키징 기준에서 노출 여부를 사람이 매번 기억해야 한다.

근거:

- `TunaSweeper/Content/Data/GameplayInteractionSpawns.json`
- `TunaSweeper/Content/Data/QuestDefinitions.json`
- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:294`

### 해결

1. `Content/Data/Dev` 또는 fixtures로 분리한다.
2. runtime loader에 build config/dev flag 필터를 둔다.
3. 패키징 검증에서 dev-only row가 포함되면 실패하게 한다.

### 검증

- Development PIE에서는 필요한 test fixture가 동작.
- Shipping/package 데이터에는 test/debug row가 포함되지 않음.

## P2. 에셋 경로 하드코딩과 generated asset 결합

### 문제

C++와 JSON에 `/Game/...` asset path가 직접 박혀 있고, editor run-once가 만든 BP/mesh/material 이름을 runtime JSON과 문서가 직접 참조한다. 에셋 이름 변경이나 재생성 실패가 컴파일 단계가 아니라 런타임 로드 단계에서 드러난다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp:55`
- `TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacter.cpp:142`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp:2904`
- `Docs/runtime_actor_spawns.md:73`

### 해결

1. generated asset manifest를 만들고 editor 작업이 생성 결과와 manifest를 함께 갱신한다.
2. runtime은 manifest key 또는 PrimaryAsset alias를 읽는다.
3. asset registry 검증으로 class/material/mesh/widget path 로드 가능 여부를 사전 검사한다.

### 검증

- 전체 spawn row, UI widget class, material/static mesh/texture soft reference load validation.
- cook/package에서 missing asset warning이 없는지 확인.

## P2. 런타임 UMG/텍스트 하드코딩

### 문제

HUD, 인트로 메뉴, 아이템 컨테이너, 작업대 위젯이 C++에서 직접 위젯 트리, 텍스트, 색, 여백, 키 프롬프트를 만든다. 화면이 빠르게 늘어난 현재 구조에서는 작은 문구나 스타일 변경도 C++ 빌드를 요구하고, 한/영/일 텍스트 길이와 입력 장치별 표시 차이를 코드 리뷰로만 확인해야 한다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp:397`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidget.cpp:1907`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidget.cpp:167`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidget.cpp:2626`
- `TunaSweeper/Source/TunaSweeper/Private/UI/ItemContainerWidget.cpp:1617`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperWorkbenchPanelWidget.cpp:1047`

### 해결

1. 반복 화면 조각은 WBP 또는 작고 명확한 C++ 하위 위젯으로 분리한다.
2. 색, 여백, 폰트 크기, 아이콘 크기는 UI theme data asset으로 옮긴다.
3. 사용자 노출 문구는 StringTable 또는 프로젝트의 텍스트 로더로 이동한다.
4. 키보드/마우스, 게임패드 프롬프트는 input glyph resolver를 통해 표시한다.

### 검증

- HUD, 인트로 메뉴, 인벤토리/컨테이너, 작업대 주요 상태 스크린샷 비교.
- 한국어/영어/일본어 텍스트 길이 확인.
- 키보드/마우스와 게임패드 입력 프롬프트 전환 확인.

## P3. 효과/디버그 코드 정리

### 문제

무기 레이저 디버그 draw/log, 작은 hit burst actor들의 Tick/lifetime 코드, C++ 기반 explosion timeline이 런타임 코드에 섞여 있다. 지금의 기능 위험은 낮지만 아트 수정이나 성능 검증 때 코드를 다시 열게 만든다.

근거:

- `TunaSweeper/Source/TunaSweeper/Private/Weapon/TunaSweeperWeapon.cpp:33`
- `TunaSweeper/Source/TunaSweeper/Private/Weapon/TunaSweeperWeapon.cpp:376`
- `TunaSweeper/Source/TunaSweeper/Private/Effect/TunaSweeperLocalExplosionEffectActor.cpp:41`
- `TunaSweeper/Source/TunaSweeper/Private/Effect/TunaSweeperProjectileHitBurstActor.cpp:59`
- `TunaSweeper/Source/TunaSweeper/Private/Component/TunaSweeperWeaponCombatComponent.cpp:7`

### 해결

1. debug draw/log 기본값 off, shipping 제외.
2. burst actor 반복 로직을 공통 component 또는 Niagara로 이동.
3. explosion VFX 타이밍/색/크기는 data asset 또는 Niagara user parameter로 이동.
4. `WeaponCombatComponent` Tick은 재장전 또는 spread recovery가 필요할 때만 켠다.

### 검증

- 레이저 sight debug on/off.
- projectile hit, melee swing/impact, explosion 수명과 색.
- 재장전 progress broadcast와 spread 회복.

## 권장 실행 순서

1. P0 데이터 복구와 데이터 검증 명령 추가.
2. 공용 data reader/schema validator 추가. 이후 새 JSON/CSV 로더는 반드시 이 경로를 사용한다.
3. `GameInstance` 저장 export/import를 도메인별 snapshot으로 분리한다. 이 단계는 `Docs/save_persistence.md`와 같이 진행한다.
4. `EnemySpawnSubsystem`에서 gameplay interaction spawn type별 parser/handler를 분리한다.
5. HUD/Intro/ItemContainer/Workbench UI를 panel별 presenter/view-model로 쪼개고, 런타임 UMG/텍스트 하드코딩을 WBP/StringTable/theme data asset으로 이동한다.
6. AI/전투는 데이터 자산과 컴포넌트 분리를 먼저 하고, Behavior Tree 전환은 그 다음에 판단한다.
7. asset manifest, dev fixture 분리, VFX/debug 정리는 위 작업의 회귀가 안정된 뒤 진행한다.

## 작업 원칙

- 큰 파일을 한 번에 줄이는 리팩터링은 피한다. 먼저 검증을 추가하고, public behavior를 고정한 뒤 도메인별로 이동한다.
- 저장 구조 변경은 항상 기존 save migration과 `Docs/save_persistence.md` 갱신을 포함한다.
- 데이터 스키마 변경은 Content/Data validator와 함께 들어가야 한다.
- UI 변경은 주요 해상도와 한/영/일 텍스트 길이 스크린샷으로 확인한다.
- AI/전투 변경은 수치 조정과 구조 변경을 같은 PR에 섞지 않는다.
