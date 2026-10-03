# 레이드 레벨 독립화와 제작 프로젝트 구현 계획

> **2026-09-30 범위 변경:** 사용자가 기존 세 맵 이식을 모두 취소하고 새 레벨 제작 기반만 남긴 뒤 현재 작업을 커밋하도록 요청했다. Task1~3은 구현·검토 완료 상태로 보존한다. Task4 시험 변경은 복구 패치 보관 후 제외했으며, 아래 Task4~7의 기존 맵 이전 절차를 자동 재개하지 않는다. 새 제작 프로젝트와 독립 cook은 후속 범위로 다시 계획해야 한다. 현재는 전체 계획 완료가 아닌 공용 기반 중간 커밋이다.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 사용자가 단계별 하위 에이전트 구현·검토 방식을 선택했다.

**Goal:** 세 레이드 맵을 게임 모듈 없이 열고 cook할 수 있는 플러그인 콘텐츠로 옮기고, 본 게임의 배치·동작·저장 호환성을 유지하며 UE 5.7 제작 프로젝트를 제공한다.

**Architecture:** RaidLevelKit은 중립 앵커와 레벨 계약·환경 도구를 제공한다. TunaRaidMaps는 공식 맵과 필요한 환경 콘텐츠를 소유한다. 게임 전용 Actor·프로필·퀘스트·세이브는 TunaSweeper에 남기고 맵의 앵커로 런타임 생성한다. 제작자는 실제 사용자 맵 자체를 열어 테스트하며 공통 Persistent Level 스트리밍 구조를 새로 도입하지 않는다.

**Tech Stack:** Unreal Engine 5.7 (현재 설치 5.7.4), C++, Unreal Automation, Editor Python, PowerShell.

**Spec:** `Docs/raid_level_plugin_review.md`.

## Global Constraints

- 프로젝트는 `TunaSweeper/TunaSweeper.uproject`이며 UE 5.7 API와 `EngineIncludeOrderVersion.Unreal5_7`을 사용한다.
- 모든 사용자 표시 텍스트는 문자열 키로 해석한다. 공용 도구도 게임 모듈에 의존하지 않는 문자열 테이블을 제공한다.
- 기존 `PlacementId`, AnchorKind 0/1/2, 적 중복 ID 허용 규칙과 논리 레벨 별칭을 보존한다. 새 번호 대역은 `Docs/raid_placement_id_numbering.md`와 함께 정의한다.
- 앵커가 위치·회전·크기를 소유한다. 게임 프로필에는 같은 배치의 transform을 중복 저장하지 않는다.
- 제작 플러그인에서 `/Script/TunaSweeper`와 게임 `/Game` 패키지를 참조하지 않는다. 다른 플러그인으로 숨겨진 역의존성도 검사한다.
- 원본 작업 폴더의 미커밋 벙커 맵, 루팅 데이터, 질문·요청 기록을 보존한다.
- 작업 트리는 `C:/Users/blendue/.codex/worktrees/raid-level-plugin/extraction_shooter`, 시작 커밋은 `57a6a341158adb6d2ca4fcadb734f46d9e97e2fe`이다. 경로가 필요한 명령은 이 루트를 사용한다.
- 스토어 패키징은 이번 작업의 검증 대상으로 삼지 않는다. 기본 제작 프로젝트의 cook과 개발용 게임 실행으로 검증한다. 스토어 패키징을 수행해야 하면 그 시점에 지정 체크리스트와 권한 절차를 따른다.
- 접근 제한 데이터 저장소의 내용이나 원격 식별자를 읽거나 공개 파일에 복사하지 않는다. 기존 공개 소스의 데이터 경로 계약은 유지한다.
- 일회성 이전 코드·진입점·의존성은 생성 에셋 검증 후 제거하고 재검증한다. 구현·정리·문서·로그는 최종 단일 작업 커밋에 함께 포함한다. 단계별 커밋을 요구하는 스킬 기본값보다 이 프로젝트 규칙을 우선한다.

## 범위

이번 구현은 원래 요청 1의 레이드 독립화와 그 독립성을 입증하는 사용자 제작 프로젝트까지이다. 원래 요청 2의 ZIP/Steam 창작마당은 가능성 검토 결과를 유지한다. 실제 사용자 팩 설치 UI, 런타임 Pak/IoStore 로더, Workshop 게시/구독, 외부 맵 저장 정책은 독립화 후 별도 구현 단위다. 제작 프로젝트에서 cook된 샘플을 생성했다는 사실만으로 본 게임의 외부 팩 로딩이 구현됐다고 보고하지 않는다.

## Review Focus

1. 모듈·패키지 경로 변경 후 기존 BP가 부모 클래스·enum 값을 잃지 않아야 한다 — Task 2의 redirect 재로드 검사.
2. 앵커 전환 시 EditInstanceOnly 설정, 컴포넌트 값, attachment와 액터 간 참조가 유실되지 않아야 한다 — Task 1/3/4의 snapshot 왕복 대조.
3. 레이드 별칭과 실제 패키지 경로를 혼동해 스폰·귀환 저장·XP·미니맵이 누락되지 않아야 한다 — Task 5의 경로/저장 회귀.
4. 초기화나 로드 이벤트가 두 번 와도 같은 앵커에서 중복 생성되지 않아야 한다 — Task 3의 멱등성 및 전환 테스트.
5. 플러그인이 편집기에서는 열려도 cook이 게임 콘텐츠/에디터 코드를 요구하는 상태여서는 안 된다 — Task 6의 게임 소스 없는 별도 빌드·cook.

## 검증 명령과 증거

각 명령의 전체 출력은 작업 트리 `TunaSweeper/Saved/RaidLevelMigration/` 아래에 저장한다. 오류 발생 시 로그 원인을 확인하고 같은 검증을 다시 수행한다.

```powershell
$taskRoot = 'C:/Users/blendue/.codex/worktrees/raid-level-plugin/extraction_shooter'
$taskEngine = 'C:/Program Files/Epic Games/UE_5.7/Engine'
$taskProject = "$taskRoot/TunaSweeper/TunaSweeper.uproject"
& "$taskEngine/Build/BatchFiles/Build.bat" TunaSweeperEditor Win64 Development "-Project=$taskProject" -WaitMutex -NoHotReloadFromIDE
```

자동화는 `UnrealEditor-Cmd.exe <project> -unattended -nullrhi -nosound -ExecCmds="Automation RunTests <filter>" -TestExit="Automation Test Queue Empty" -ReportExportPath=<report>`로 실행한다. 종료 코드뿐 아니라 요청한 테스트의 실행 건수와 실패 0개를 검사한다. DDC는 프로세스 환경 변수 `UE-LocalDataCachePath`로 작업 트리의 캐시 폴더를 지정하고 `-DDC=InstalledNoZenLocalFallback`을 사용한다. 실행 뒤 환경 변수를 복원한다.

---

### Task 1: 이전 전 배치 계약과 역참조 기준 확보

**Files:**
- Create: `Tools/RaidLevels/audit_levels.py` — 재사용 가능한 읽기 전용 의존성·배치 검사.
- Create: `Tools/RaidLevels/verify_migration.py` — 이전 전/후 manifest와 저장된 에셋의 비교.
- Modify: `Docs/raid_placement_id_numbering.md` — 실제 기존 ID 조사 후 추가 영역을 기록.
- Evidence: `TunaSweeper/Saved/RaidLevelMigration/before.json` 및 `reverse_references.json`.

**Interfaces:**
- Input worlds: `/Game/Maps/DemoBoxRaidMap`, `/Game/Maps/DemoRaidMap`, `/Game/MainRaid/RaidMap`.
- Output manifest: 원본 패키지, actor object name/class/transform/tags, 전체 기존 앵커 kind/id/중복 허용값, editable actor/component properties, attachment, level actor references, Level Blueprint graph 참조, hard/soft dependency 및 역참조.
- 생성 프로필과 앵커 연결은 `PhysicalMapId`, `PlacementId`, `ProfileId`로 기록한다. 기존 JSON의 논리 `LevelId`는 별도 필드로 보존한다.

- [x] 세 맵과 앵커 BP·미리보기 DA의 역참조, 전체 플러그인 의존성, Level Blueprint를 읽는다. World Partition이 있으면 필요한 외부 액터를 모두 로드한다.
- [x] 기존 직접 배치 액터의 설정과 참조를 manifest로 기록한다. 생성되는 에디터 보조 객체를 authored actor로 취급하지 않는다.
- [x] `verify_migration.py`가 맵에서 `/Script/TunaSweeper` 또는 게임 Actor를 발견하면 실패하도록 작성하고 현재 맵에서 의도한 실패를 확인한다.
- [x] 실제 앵커와 공개 배치 JSON의 ID를 대조한다. 이 단계에서는 기존 에셋이나 데이터 ID를 바꾸지 않는다.
- [x] baseline 자동화 `TunaSweeper.RaidPlacement`와 관련 메모·맵·BuildFlavor 테스트의 정확한 이름/결과를 기록한다. 기존 실패는 이전과 연관된 실패와 구분한다.

### Task 2: RaidLevelKit 앵커 계약 분리

**Files:**
- Create: `TunaSweeper/Plugins/RaidLevelKit/RaidLevelKit.uplugin`.
- Create: `.../Source/RaidLevelRuntime/RaidLevelRuntime.Build.cs`, `Private/RaidLevelRuntimeModule.cpp`.
- Move: 기존 `Public/Raid/TunaSweeperRaidPlacementAnchor.h`, `TunaSweeperLootAnchorPreviewDataAsset.h`와 대응 cpp를 플러그인의 동일 `Public/Raid`, `Private/Raid` 경로로 이동.
- Create: `.../Source/RaidLevelRuntime/Public/Raid/RaidLevelIdentity.h`와 `Private/Raid/RaidLevelIdentity.cpp`.
- Create: `.../Source/RaidLevelEditor/RaidLevelEditor.Build.cs`와 모듈 진입점, `.../Content/Localization/`의 플러그인 문자열 테이블.
- Modify: `TunaSweeper.uproject`, `Source/TunaSweeper/TunaSweeper.Build.cs`, `Config/DefaultEngine.ini`, 기존 RaidPlacement 자동화.
- Move assets: `/Game/Raid/Placement/*` → `/RaidLevelKit/Placement/*`, 필요한 미리보기 재질/텍스처 포함.

**Interfaces:**
- 기존 `ATunaSweeperRaidPlacementAnchor`, `ETunaSweeperRaidPlacementAnchorKind`, `FTunaSweeperLootAnchorPreviewDefinition`, `UTunaSweeperLootAnchorPreviewDataAsset` 이름을 유지하고 export macro를 `RAIDLEVELRUNTIME_API`로 바꾼다.
- `FRaidLevelIdentity { FName PackId; FName MapId; FName LogicalLevelId; FSoftObjectPath World; }`는 물리 맵과 논리 데이터 ID를 분리한다. 기존 맵 별칭의 판정은 게임 어댑터에 유지한다.
- 중립 구조 검증 결과는 문자열 키와 치환 인수로 반환하고 게임 타입을 include하지 않는다.

- [x] 기존 BP 부모/enum 값과 중복 허용 규칙을 재로드하는 회귀 테스트를 먼저 보강한다. 독립 프로젝트에서 앵커 클래스가 로드되지 않는 현재 상태의 실패를 확인한다.
- [x] 타입 이동과 Core Redirects(class/enum/struct)를 적용한다. 런타임 모듈에 UnrealEd 의존성을 추가하지 않는다.
- [x] UE 에셋 이동 API로 앵커/미리보기 콘텐츠를 이전하고 BP를 재저장한다. 파일 시스템 복사만으로 package path를 바꾸지 않는다.
- [x] 새 플러그인의 native include/Build.cs와 에셋 전체 참조가 게임을 요구하지 않는지 검사한다. 에디터 표시 문자열은 플러그인 문자열 키로 처리한다.
- [x] Editor 빌드, RaidPlacement 테스트, BP 저장 후 새 프로세스 재로드를 통과한다.

### Task 3: 게임 소유 프로필에서 기존 배치 액터 생성

**Files:**
- Create: `Source/TunaSweeper/Public/Raid/TunaSweeperRaidActorCatalog.h`, `Private/Raid/TunaSweeperRaidActorCatalog.cpp`.
- Create: `Source/TunaSweeper/Public/Subsystem/TunaSweeperRaidActorSubsystem.h`, `Private/Subsystem/TunaSweeperRaidActorSubsystem.cpp`.
- Create: `Source/TunaSweeper/Private/Tests/TunaSweeperRaidActorTests.cpp`.
- Modify: 공용 앵커 enum/header, RaidPlacementSubsystem과 MemoSubsystem의 공통 앵커 검증, `Docs/raid_placement_id_numbering.md`.
- Create assets: `/Game/RaidRuntime/Catalogs/DA_<MapId>_Actors`, `/Game/RaidRuntime/Profiles/<MapId>/BP_<ProfileId>`.

**Interfaces:**
- AnchorKind의 기존 값 뒤에 `AuthoredActor = 3`을 추가한다. 해당 ID는 문서에 새로 예약한 `3000+` 영역에서 할당하며 기존 종류/ID를 바꾸지 않는다.
- `FTunaSweeperRaidActorPlacement { int32 PlacementId; FName ProfileId; }`에는 transform을 저장하지 않는다.
- `FTunaSweeperRaidActorProfile`은 `ProfileId`, `TSoftClassPtr<AActor> ActorClass`, 인스턴스 전용 설정과 참조 연결 정보를 게임 쪽에 소유한다. 기존 Actor의 컴포넌트 구성은 게임 소유 프로필 Blueprint에 보존한다.
- `UTunaSweeperRaidActorCatalog : UDataAsset`는 한 물리 MapId에 대한 placements/profiles를 보관한다. 레이드 별칭이 같아도 다른 물리 맵의 프로필을 잘못 적용하지 않는다.
- `bool UTunaSweeperRaidActorSubsystem::EnsureActorsSpawnedForWorld(UWorld* World)`는 GameWorld·해당 카탈로그·앵커를 검증하고 일회 생성한다. 새 World에서는 새로 생성하고 같은 World의 반복 호출은 중복 생성하지 않는다.
- 레벨 Actor 참조는 게임 프로필에 대상 앵커 ID 또는 독립 환경 Actor 식별자로 저장한다. deferred spawn으로 전체 객체를 만든 뒤 참조/attachment를 연결하고 초기화를 마친다. BeginPlay가 연결 전 상태를 관찰하지 않도록 테스트한다.

- [x] 테스트 `InvalidCatalogDoesNotSpawn`, `RepeatedInitializationSpawnsOnce`, `NewWorldSpawnsAgain`, `AnchorTransformWins`, `InstanceOnlyPropertiesSurvive`, `ActorReferencesResolveBeforeBeginPlay`를 작성하고 실패를 확인한다.
- [x] 데이터 검증과 deferred spawning을 구현한다. 중복/누락/종류 불일치/누락 프로필은 추측 위치로 생성하지 않으며 같은 오류의 반복 실행도 추적 가능하게 처리한다.
- [x] UE `CreateBlueprintFromActor`는 `SkipInstanceOnlyProperties`를 사용하므로 별도 인스턴스 설정 보존을 구현한다. 기본 Blueprint 변환 성공만으로 원본 보존을 판정하지 않는다.
- [x] 동일 유형의 적·루팅·메모 기존 경로와 AuthoredActor 경로가 서로 중복 생성하지 않도록 기존 소비자 검증을 조정한다. 허용된 적 중복 ID는 메모 소비자에서도 동일 규칙으로 처리한다.
- [x] 카탈로그와 프로필은 공식 게임 데이터만 사용한다. 사용자 팩의 임의 클래스 경로·반사 속성 수정을 받아들이는 API는 추가하지 않는다.
- [x] 단위/통합 테스트를 통과하고 실제 레벨 이동 및 BeginPlay 시점으로 초기화 순서를 검증한다.

### Task 4: 세 맵을 정적 환경과 앵커로 전환하고 에셋 이전

**Files:**
- Create: `TunaSweeper/Plugins/TunaRaidMaps/TunaRaidMaps.uplugin` (`CanContainContent=true`, 게임 모듈 의존성 없음).
- Destination maps: `/TunaRaidMaps/Maps/DemoBoxRaidMap`, `/TunaRaidMaps/Maps/DemoRaidMap`, `/TunaRaidMaps/Maps/RaidMap`.
- Destination environment: `/TunaRaidMaps/Environment/<기존 상대 경로>`.
- Move neutral environment code: `TunaSweeperSplineConcreteBarrierActor`, `TunaSweeperShallowPuddleActor`, `TunaSweeperLocationBlendCameraActor`의 h/cpp를 RaidLevelKit Runtime의 `Environment`/`Camera` 폴더로 이동. native 기본 에셋 경로는 독립 플러그인 경로 또는 에셋에서 주입한 값으로 변경한다.
- Map capture: 공용 캡처 도구를 RaidLevelEditor로 분리하고 게임 MapDefinition 갱신은 TunaSweeperEditor의 어댑터에 유지한다. 게임 타입을 참조하는 캡처 BP는 이동 후 레이드 맵에 남기지 않는다.
- Temporary: `Source/TunaSweeperEditor/Private/RaidLevelMigrationOnce.*`와 명시적으로 호출되는 임시 진입점. 검증 후 삭제한다.

**Interfaces:**
- `before.json`의 각 게임 Actor는 새 앵커/프로필로 1:1 추적된다. 이동된 카메라·물·스플라인은 독립 환경 Actor로 남길 수 있다.
- 정적 에셋 이전 manifest는 old package → new package를 정확히 기록한다. 파일 이름이나 짧은 맵 이름으로 참조를 추측하지 않는다.

- [ ] DemoBox에서 게임 Actor를 앵커로 교체하고 해당 프로필을 생성한다. 설정·컴포넌트·참조 왕복 검증이 통과하기 전 원본 Actor를 제거하지 않는다.
- [ ] 같은 검증을 DemoRaid/RaidMap에 적용한다. 이미 존재하는 앵커는 변환하거나 재번호 부여하지 않는다. 직접 배치된 적 7개도 원본 동작을 유지한다.
- [ ] Level Blueprint의 게임 참조를 런타임 어댑터로 이전한다. 발견한 그래프를 무조건 비우는 방식은 금지한다.
- [ ] 맵과 필요한 환경 에셋을 Unreal 에셋 도구로 옮긴다. 물 플러그인 등 허용 의존성도 게임 `/Game`을 역참조하지 않는지 재귀 검사한다.
- [ ] hard/soft 역참조, 컴포넌트 에셋, Landscape LayerInfo, foliage, external actors/build data를 갱신한다. 작업 전부터 수정된 원본 BunkerMap은 덮어쓰지 않는다.
- [ ] 기존 게임 참조의 전환용 redirector/Core Redirects를 정확한 경로 단위로 유지한다. Kit와 독립 맵이 이전 게임 경로의 redirector에 의존해서만 로드되는 상태는 실패로 판정한다.
- [ ] 새로운 프로세스로 저장된 맵을 열어 `verify_migration.py`를 통과한다. 삭제/추가 Actor 목록과 설정 차이는 모두 명시적으로 설명돼야 한다.

### Task 5: 게임의 맵 경로·데이터·저장 호환성 연결

**Files:**
- Modify: `Private/Settings/TunaSweeperBuildFlavor.cpp`, `Private/Game/TunaSweeperGameInstance.cpp`, `Private/Map/TunaSweeperMapDefinition.cpp`.
- Verify/modify as needed: `Private/Game/TunaSweeperGameInstanceSave.cpp`, `TunaSweeperGameInstanceExperience.cpp`, QuestSubsystem, MemoSubsystem, RaidPlacementSubsystem.
- Modify: `Config/DefaultGame.ini`, `Config/Custom/Demo/DefaultGame.ini`, `Config/Custom/StoveDemo/DefaultGame.ini`, `Config/Custom/NoStoreDemo/DefaultGame.ini`, `Source/TunaSweeperEditor/Private/TunaSweeperLevelOpenTool.cpp`.
- Update map definition assets and tests containing old paths.
- Modify: `Docs/raid_placement_anchors.md`, `Docs/raid_placement_id_numbering.md`; `Docs/save_persistence.md` only if persisted semantics change.

**Interfaces:**
- 기존 `GetRaidGameplayLevelName()`, `IsRaidGameplayLevelName(FName)`, `ResolveGameplayLevelName(FName)`의 호출 계약을 유지한다. 반환 경로는 새 플러그인 World 경로이며 기존 logical aliases를 정확히 정규화한다.
- 적/루팅/메모 JSON의 기존 `level_name`과 저장 ID를 유지한다. 공식 맵 경로 이동 때문에 새 슬롯이나 새로운 저장 네임스페이스를 만들지 않는다.

- [ ] 기존/새 전체 패키지 경로, 짧은 별칭, PIE 접두어, 비레이드 맵을 포함한 BuildFlavor/MapDefinition 회귀를 작성하고 실패를 확인한다.
- [ ] 게임·에디터 맵 선택, 미니맵 soft World 참조, cook include/exclude를 새 경로로 갱신한다. Demo/Main의 기존 포함 정책을 유지한다.
- [ ] 진입·탈출·사망 저장, 적/루팅/메모 생성, XP 적용을 실제 세션에서 검증한다. 기존 저장 파일 fixture를 읽어 이전 기록이 유지되는지 검사한다.
- [ ] 대상 맵이 없는 경우 진입 실패가 명확히 처리되는지 검사하고 임의 맵으로 fallback하지 않는다.

### Task 6: 게임 소스 없는 제작 프로젝트와 cook 검증

**Files:**
- Create: `Tools/RaidLevelAuthoring/Template/RaidLevelAuthoring.uproject`와 최소 Game/Editor Target 및 빈 프로젝트 모듈.
- Create: `Tools/RaidLevelAuthoring/create_project.ps1`, `validate.ps1`, `cook.ps1`, `README.md`.
- Create: `TunaSweeper/Plugins/RaidLevelKit/Source/RaidLevelEditor/Private/RaidLevelValidationCommandlet.cpp`와 대응 Public header.
- Evidence: `TunaSweeper/Saved/RaidLevelMigration/StandaloneAuthoring/`와 cook 결과.

**Interfaces:**
- `create_project.ps1 -Destination <directory> -EngineRoot <UE5.7 path>`는 빈 제작 프로젝트에 Kit와 명시한 재배포 가능 환경 플러그인만 복사한다. 게임 Source/Content를 링크하거나 숨겨진 추가 플러그인 디렉터리로 참조하지 않는다.
- `validate.ps1 -Project <uproject> -Map <full package>`는 `-run=RaidLevelValidate`로 전체 게임 의존성, 앵커, 맵 BP/지원 환경 참조를 검사하고 오류가 있으면 비정상 종료한다.
- `cook.ps1 -Project <uproject> -Map <full package> -Platform Windows`는 검사 성공 후 정확한 맵과 의존성을 cook한다. 출력은 제작 산출물이며 아직 게임의 설치 가능한 사용자 팩 포맷으로 부르지 않는다.

- [ ] 깨끗한 별도 폴더에 제작 프로젝트를 만들고 본 게임 모듈이 없는지 검사한다. 최소 샘플 맵과 공식 맵 검증은 별도 실행한다.
- [ ] 앵커 표시, 미리보기, 지원 환경 Actor가 정상 로드되는지 확인한다. 게임 전용 클래스·데이터를 일부러 참조하는 샘플은 검증에서 실패해야 한다.
- [ ] 공식 세 맵을 필요한 독립 플러그인과 함께 이 환경에 복사하여 로드/검사/cook한다. 게임 프로젝트나 전환 redirector를 찾지 않아도 통과해야 한다.
- [ ] 내보내는 제작 프로젝트의 README에 고정 UE 버전, 플러그인 의존성, 맵 경로 유지, 검증/쿠킹 명령을 기록한다. 엔진 바이너리와 배포 권한을 확인하지 않은 에셋은 SDK 배포물에 포함하지 않는다.

### Task 7: 정리, 독립 검토, 최종 단일 커밋

**Files:** 위 구현/에셋/검증 도구, `Docs/raid_level_plugin_review.md`, `Docs/requests.md`.

- [ ] 단계별 구현 에이전트와 다른 에이전트가 spec 준수와 코드/에셋 품질을 검토한다. 실제 출력·diff·manifest를 근거로 검사한다.
- [ ] 임시 이전 코드와 진입점·그 코드만 필요로 한 Build.cs 의존성을 제거한다. 자동 재생성이나 시작 시 migration 경로가 남아 있으면 완료 처리하지 않는다.
- [ ] 정리 후 Editor 재빌드, 관련 자동화, 세 맵 재로드/런타임 대조, 독립 제작 프로젝트 검사·cook을 다시 실행한다.
- [ ] 원본 작업 폴더의 기존 미커밋 변경이 그대로인지 확인한다. 작업 트리의 최종 diff에 무관한 에셋이나 로그가 섞이지 않게 한다.
- [ ] 검증 결과·남은 제한·실제 경과 시간을 요청 로그와 문서에 기록하고 구현·에셋·정리·검증·기록을 한 번에 커밋한다.
- [ ] 빌드한 작업 트리의 정확한 `TunaSweeper/TunaSweeper.uproject`로 에디터를 연다. 원본으로 병합/푸시하거나 사용자 맵을 외부에 게시하는 작업은 별도 승인 없이 수행하지 않는다.

## 계획 자체 검토

- 원래 요청 1의 맵 독립성, 정적 요소+앵커, 게임 → 플러그인 방향은 Task 2–6으로 검증한다.
- 제작 방식 확정은 Task 6에 반영했다. 원래 요청 2는 조사 범위였으므로 로컬 사용자 팩 로더/Workshop 기능을 이번 완료 조건으로 확장하지 않았다.
- Actor 개별 설정 보존과 BeginPlay 순서는 Task 3–4의 선행 검증이다. 지원되지 않는 설정을 조용히 버리는 이전은 허용하지 않는다.
- 게임 저장 키와 Demo/Main 구분은 Task 5에서 보존한다. 새 데이터 영속화가 생기면 문서와 회귀 검증을 함께 추가한다.
- 모든 단계의 생성물 검증을 최종 소스 정리 후 재실행하며, 최종 커밋은 하나로 묶는다.
