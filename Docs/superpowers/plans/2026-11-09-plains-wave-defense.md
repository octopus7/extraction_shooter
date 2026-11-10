# 3지역 평원 1인 웨이브 거점 방어 Implementation Plan

> **For agentic workers:** 구현 착수 시 `superpowers:executing-plans` 또는 사용자가 선택한 `superpowers:subagent-driven-development`를 적용해 체크박스 단위로 진행한다. 이 문서 작성은 구현 착수 승인이 아니다. 현재는 문서만 작성했으며 실행 방식은 아직 선택하지 않았다.

**Goal:** 평원에 독립 거점을 조성하고 혼자서 6웨이브를 방어·완주·재도전하는 선택 미니게임을 구현한다.

**Architecture:** 기존 평원 맵과 GameMode 안에 전용 Director를 배치한다. DataAsset은 규칙, 저장된 맵 액터는 위치, Director는 회차, 기존 GameInstance는 영속 결과와 코인 수령을 소유한다. 일반 적의 전투는 재사용하고 회차 적에만 목표·보상 정책을 적용한다.

**Tech Stack:** Unreal Engine 5.7, C++, DataAsset, 편집 가능한 Actor/Widget Blueprint, 기존 내비게이션·팩션·상호작용·문자열 키·세이브 시스템.

**Spec:** [2026-11-09-plains-wave-defense-design.md](../specs/2026-11-09-plains-wave-defense-design.md). 사용자 확정 사항과 작성자 추천값을 구분해 읽는다. 구현 중 수치를 바꾸면 설계 표·DataAsset·검증 기대값을 함께 갱신한다.

## Global Constraints

- 프로젝트는 `TunaSweeper/TunaSweeper.uproject`, 엔진은 **UE 5.7**이다.
- **평원에 독립된 새 거점을 조성한다.** SSTO 부속 시설로 대체하지 않는다.
- 북쪽은 +X, 오른쪽은 +Y, 1m=100cm다. 기존 플레이 동선·액터 식별자·사다리·탈출을 보존한다.
- 모든 플레이어 UI는 문자열 키로 제공하고 `ko,en,ja,zh-Hans,zh-Hant,ru` 6개 언어를 작성한다. 플레이어-facing 한국어는 `기지`를 사용한다.
- 새 위젯 클래스는 목적+실제 모델명, 디렉터리는 실제 모델명, WBP는 실제 모델명 접미사를 사용한다. 소스에 실제 모델명과 reasoning effort를 기록하며 추측하지 않는다.
- 위젯 트리는 저장된 Widget Blueprint에서 구성하고 C++ 위젯 클래스에서 생성하지 않는다.
- 에디터 감지·사용·종료·재실행 전 [에디터 조정 규칙](../../unreal_editor_coordination.md)을 읽고 다른 작업과 조정한다.
- 일반 레이드 앵커 또는 `PlacementId`를 변경하게 되면 먼저 [번호 규칙](../../raid_placement_id_numbering.md)을 읽는다. 전용 웨이브 생성 후보에는 기존 앵커 ID를 임의 사용하지 않는다.
- 기존 Main 서사·퀘스트 데이터 복사나 자동 동기화는 범위 밖이다. 이 기능은 공개 일반 전투 에셋과 별도 비서사 설정으로 구현한다.
- 저장 필드를 구현할 때 [저장 계약](../../save_persistence.md)을 함께 갱신한다. 현재 문서 작업에서는 기존 계약을 수정하지 않는다.
- 명시적인 요청 없이 커밋하지 않는다. 일회성 생성기는 검증 후 제거하고, 다른 대화의 미커밋 변경을 보존한다.

## Review Focus

1. 마지막 적 사망과 장치/플레이어 사망이 같은 프레임에 겹쳐도 실패 우선이며 이중 종결되지 않는다 — Task 1·3.
2. 코인 수령 연타·저장 실패·재시작·슬롯 전환에서 중복 지급이나 잘못된 성공 알림이 없다 — Task 5.
3. 적 삭제·스폰 막힘·비가시 경로 정체가 조용한 성공 또는 무한 대기를 만들지 않는다 — Task 3.
4. 회차 정리가 일반 적·퀘스트·전리품에 영향을 주지 않으며 포탑/펫 처치도 한 번 집계한다 — Task 2·3·4.
5. 준비 종료와 시설 구매가 겹치거나 일시정지·이탈·이동이 발생해도 비용과 상태가 일치한다 — Task 1·4·6.

## 파일 구성과 공통 계약

아래 C++ 경로는 `TunaSweeper/Source/TunaSweeper/` 기준이다. `Tools/`, `Docs/`, `TunaSweeper/Content/`로 시작하는 경로는 저장소 루트 기준이다. 기존 파일을 수정할 때 현재 작업본을 다시 확인하며 다른 대화의 변경을 덮어쓰지 않는다.

| 신규 경로 | 책임 |
| --- | --- |
| `Public/WaveDefense/WaveDefenseTypes.h` | 상태·결과·역할·시설 타입, UI 스냅샷·저장 레코드 |
| `Public/WaveDefense/WaveDefenseDefinition.h`, `Private/WaveDefense/WaveDefenseDefinition.cpp` | DataAsset 정의 및 검증 |
| `Public/WaveDefense/WaveDefenseRun.h`, `Private/WaveDefense/WaveDefenseRun.cpp` | 월드와 분리한 상태 전이·수량·자원 계산 |
| `Public/WaveDefense/WaveDefenseDirector.h`, `Private/WaveDefense/WaveDefenseDirector.cpp` | 실제 회차 진행·스폰·참조·정리 |
| 같은 두 디렉터리의 `WaveDefenseCore.h/.cpp` | 장치 내구도와 피해 필터 |
| 같은 두 디렉터리의 `WaveDefenseParticipantComponent.h/.cpp` | 회차 귀속·적 역할·기본 보상 억제 |
| 같은 두 디렉터리의 `WaveDefenseSpawnPoint.h/.cpp` | 편집 가능한 생성 위치·진입로 |
| 같은 두 디렉터리의 `WaveDefenseFacility.h/.cpp` | 방벽·포탑·고정 슬롯 런타임 |
| 같은 두 디렉터리의 `WaveDefenseTerminal.h/.cpp` | 단말과 기존 상호작용 연결 |
| `Private/Game/TunaSweeperGameInstanceWaveDefense.cpp` | 저장·보상 트랜잭션 |
| `Private/Tests/WaveDefense{Rules,Combat,Spawn,Facility,Persistence,Assets}Tests.cpp` | 각 단계의 동작 검증 |
| `Tools/WaveDefense/verify_assets.py` | 저장된 에셋·맵을 읽기 전용으로 검증 |

`{Rules,...}`는 파일 목록을 줄여 표기한 것으로 실제 파일은 각각 생성한다. UI 파일명만은 실제 구현 모델을 확인한 뒤 `Public/UI/<실제모델>/WaveDefenseHUD<실제모델>.h`와 `WaveDefensePanel<실제모델>.h`, 대응 Private `.cpp`로 확정한다. 실제 정보가 없으면 이름을 추정해 만들지 말고 구현 환경에서 확인한다.

### 공통 데이터

- `EWaveDefensePhase`: `Idle`, `Preparing`, `Active`, `Intermission`, `Resolving`, `Finished`.
- `EWaveDefenseOutcome`: `None`, `Succeeded`, `CoreDestroyed`, `PlayerDied`, `Abandoned`, `Interrupted`, `Faulted`.
- `EWaveDefenseEnemyRole`: `CoreAssault`, `PlayerPressure`, `EliteAssault`.
- `EWaveDefenseFacilityKind`: `Barrier`, `Turret`.
- `EWaveDefenseAction`: `InstallBarrier`, `InstallTurret`, `RepairCore`, `RepairFacility`, `ReloadTurret`.
- `FWaveDefenseProgress`: 최근 `FGuid RunId`, `DifficultyStage`, `Outcome`, `LastCompletedWave`, `RewardCoins`, `bRewardPending`, `BestCompletedWave`, `CompletedRuns`. `RunId` 유효 + `Outcome=None`은 저장된 진행 중 회차다.
- `FWaveDefenseSnapshot`: phase/outcome, 1부터 시작하는 wave/total, alive/queued, core current/max, supplies, 준비·이탈 남은 초, 예고 진입로. 표시 문자열을 포함하지 않는다.
- `UWaveDefenseDefinition`: `EncounterId`, 장치·준비·이탈·생성·정체 기준, 시설 설정, 6개 wave 행, 300코인. 적 행은 클래스 soft reference·전투 프로필 키·역할·기본 체력·무기/탄약 ID·예비 탄수·수량을 가진다.
- 설정 수치의 원본은 설계 §5~§6이다. 예상되지 않은 빈 프로필·클래스·0/음수 수량·소프트 참조 로드 실패는 시작 불가이며 추측한 적을 대신 생성하지 않는다.

## Task 1: 데이터와 회차 상태 모델

**Files:** 새 `WaveDefenseTypes`, `WaveDefenseDefinition`, `WaveDefenseRun`, `WaveDefenseRulesTests`.

**Interfaces:** `UWaveDefenseDefinition::ValidateDefinition(TArray<FName>& OutErrors) const -> bool`; `FWaveDefenseRun::Start(const UWaveDefenseDefinition&, FGuid RunId) -> bool`; `AdvanceTime(float GameSeconds) -> void`; `NotifySpawned(FGuid EnemyToken) -> bool`; `NotifyEnemyDied(FGuid EnemyToken) -> bool`; `ResolveFrame(bool bPlayerDead, bool bCoreDestroyed) -> void`; `GetSnapshot() const -> FWaveDefenseSnapshot`. 검증은 오류 키를 반환하고 호출 측에서 기존 TextSubsystem으로 현지화한다.

- [x] `TunaSweeper.WaveDefense.Rules`에 실패 테스트를 추가한다. 6행 수량 `6,8,10,12,14,16`, 합계66, 상한 `4,5,6,7,8,8`을 검증한다.
- [x] 45초 준비→Active, `queued=0 && alive=0`에서만 30초 Intermission, 마지막 wave에서 Resolving으로 가는 전이를 테스트한다. 빈/잘못된 정의는 시작을 거절한다.
- [ ] 동일 EnemyToken 사망 중복, 상한 도달 중 spawn, 마지막 사망+core/player 사망, 시작 연타를 테스트한다. 실패를 먼저 확인한 뒤 상태 모델을 구현한다.
- [ ] 초기200·wave1~5 완료마다100·wave6 추가없음, pause 중 `AdvanceTime` 호출 없음, 음수 delta 거절을 구현한다. 종결 입력을 같은 프레임에서 집계하여 사망을 우선한다.
- [x] 정의를 `/Game/MainRaid/WaveDefense/DA_PlainsWaveDefense`로 후속 에셋 작성 단계에서 저장할 수 있게 구성하고 같은 테스트 필터의 PASS를 확인한다.

**완료 조건:** UE 월드를 생성하지 않고 규정된 상태 전이와 수량을 테스트할 수 있다.

## Task 2: 장치 공격과 회차 적의 격리

**Files:** 새 `WaveDefenseCore`, `WaveDefenseParticipantComponent`, `WaveDefenseCombatTests`; 수정 `Public/AI/TunaSweeperEnemyAIController.h`, `Private/AI/TunaSweeperEnemyAIController.cpp`, `Public/AI/TunaSweeperEnemyCharacter.h`, `Private/AI/TunaSweeperEnemyCharacter.cpp`, `Private/Subsystem/TunaSweeperFactionSubsystem.cpp` 및 필요 선언 헤더.

**Interfaces:** `UWaveDefenseParticipantComponent::Initialize(AWaveDefenseDirector* Director, FGuid RunId, FGuid Token, EWaveDefenseEnemyRole Role) -> void`; `GetPreferredTarget() const -> AActor*`; `ShouldGrantDefaultKillRewards() const -> bool`; `AWaveDefenseCore::ResetForRun(FGuid RunId, int32 MaxHealth) -> void`; `TakeDamage(...) -> float`는 기존 AActor 시그니처를 사용한다. 참가자 사망 알림은 `(RunId, EnemyToken)`을 Director로 전달한다.

- [ ] `TunaSweeper.WaveDefense.Combat`에 장치의 실제 피해, 동일 회차 여부·우호 피해·다른 회차 피해·비활성 피해 필터 테스트를 먼저 작성하고 실패를 확인한다.
- [ ] 회차 적만 기본 장치 이동 의도를 받게 한다. 일반 AI의 기존 target 유지 분기보다 앞에서 역할에 따른 재평가를 수행하여 한 번 잡은 플레이어 표적에 영구 고정되지 않도록 한다. 실제 발사에는 기존 시야·사선·회전·거리 조건을 유지한다.
- [ ] 설계의 근접병 8m/최근 피격3초 우선순위, 사수의 거점 접근, 장치에 대한 일반 탐험 적의 비대상 처리를 구현한다. 팩션 관계는 기존 subsystem 판정기를 사용한다.
- [ ] 장치·방벽·포탑의 적 피해에도 시작 난이도 배율을 정확히 한 번 적용한다. 원시 피해10에 배율0.5/1/2를 주었을 때 실제 구조물 피해5/10/20, 우호 피해0을 테스트하고 플레이어의 기존 배율 경로는 이중 적용하지 않는다.
- [ ] 일반 적 사망 경로에서 참가자 컴포넌트가 있는 경우 기본 경험치·퀘스트·업적·전리품/무기·회복 드롭만 억제한다. 사망 연출은 유지하고 살아 있는 수는 체력0 시점에 한 번 감소시킨다. `OnDestroyed`만으로 처치를 판정하지 않는다.
- [ ] 플레이어·포탑·펫 처치와 삭제를 구별하는 테스트, 같은 클래스의 일반 적에는 기존 지급이 유지되는 회귀 테스트를 통과시킨다.

**완료 조건:** 실제 피해로 장치가 파괴되고, 일반 전투의 기존 보상 동작은 유지된다.

## Task 3: Director·스폰·중단·정리

**Files:** 새 `WaveDefenseDirector`, `WaveDefenseSpawnPoint`, `WaveDefenseSpawnTests`; 수정 Task 1~2 인터페이스 연결부, 필요시 `Private/Weapon/TunaSweeperProjectile.cpp`와 발사체 생성 지점의 회차 귀속 연결.

**Interfaces:** Director의 `TryStart(APawn* Player) -> bool`, `SetReady() -> bool`, `NotifyEnemyDied(FGuid RunId, FGuid EnemyToken) -> void`, `Abort(EWaveDefenseOutcome Reason) -> void`, `GetSnapshot() const -> FWaveDefenseSnapshot`. `AWaveDefenseSpawnPoint`는 `LaneId`와 저장된 transform을 제공한다. 스폰 성공은 참가자 초기화·소유 추적 완료 후에만 상태 모델에 반영한다.

- [ ] `TunaSweeper.WaveDefense.Spawn`에 전부 막힌 후보, caps 도달, stale RunId 이벤트, 예기치 않은 destroy, 중복 Abort 테스트를 작성하고 실패를 확인한다.
- [ ] 기존 `ConfigureCombatProfile`·`ConfigureSpawnData`를 사용하되 DataAsset의 유효한 class/loadout만 사용한다. 생성 전 NavMesh·캡슐·경로·15m 이격·화면 노출을 검사하고 2마리/3초 및 방향 예고를 적용한다.
- [ ] 소유 토큰을 적·컨트롤러·무기·발사체·시설에 전달한다. 종료 시 해당 토큰 소유의 개체만 제거하고 일반 월드 액터는 보존한다.
- [ ] 스폰 재시도 2초/실패15초, 이동 정체10초+재경로10초 후 화면 밖 재배치 또는 Faulted를 구현한다. 교전·공격 준비 중인 적을 정체로 오판하지 않는다. 재배치는 체력·EnemyToken을 유지한다.
- [ ] 실패 우선 종결, 10초 경계 이탈, 차량 탑승·사망·맵 이동·EndPlay 중단, pause 시 타이머 정지를 연결한다. 저장 트랜잭션은 Task 5에서 연결하며 그 전 임시 테스트에서는 영속 성공으로 표시하지 않는다.
- [ ] 반복 20회 시작/중단 뒤 관련 타이머·델리게이트·컨트롤러·발사체·회차 참조가 0인지 확인한다. 기존 적·상자·메모 수는 변하지 않아야 한다.

**완료 조건:** 회차가 오류나 이동으로 끝나도 무한 대기와 다른 콘텐츠 삭제가 없다.

## Task 4: 준비 시간의 방벽·포탑·자원

**Files:** 새 `WaveDefenseFacility`, `WaveDefenseFacilityTests`; 수정 Director/Run/Definition의 시설 명령 처리.

**Interfaces:** `AWaveDefenseDirector::TryFacilityAction(FName SlotId, EWaveDefenseAction Action) -> bool`; `AWaveDefenseFacility::InitializeForRun(AWaveDefenseDirector*, FGuid RunId, FName SlotId, EWaveDefenseFacilityKind Kind) -> void`. 장치 수리는 예약 슬롯 키 `core`를 사용하며 일반 시설 슬롯과 중복되지 않게 검증한다.

- [ ] `TunaSweeper.WaveDefense.Facility`에 비용50/150/50/25/50, 자원 부족, 이미 찬 체력/탄약, 포탑 2기 상한, 중복 슬롯, Active 중 구매 거절을 작성하고 실패를 확인한다.
- [ ] 단일 게임 스레드 명령에서 phase·슬롯·참조·비용을 재검사한 뒤 차감과 설치/수리/보충을 함께 적용한다. 설치 실패는 자원을 복원한다. 준비 종료와 같은 프레임의 명령도 실제 처리 시점 phase로 결정한다.
- [ ] 방벽HP300, 포탑HP250/60발/1초/18m/피해10, 장치+200, 시설+최대치50%를 설정에서 적용한다. 잘못된 faction/회차에는 피해를 주거나 받지 않는다.
- [ ] 포탑은 실제 발사체로 사선 있는 회차 적만 공격한다. 가림막 너머·일반 적·죽은 적에 발사하지 않는다. 발사 성공시에만 탄을 차감하고 발사 실패는 무소모 처리한다.
- [ ] 방벽 설치/파괴 전후 우회 경로가 유지되는지 검증하고, 정리 시 시설과 포탑 탄환도 제거되는 테스트를 통과시킨다.

**완료 조건:** 한 플레이어가 준비 시간에 시설을 선택하고 전투 중 사격에 집중할 수 있다.

## Task 5: 저장·기록·코인 지급

**Files:** 새 `Private/Game/TunaSweeperGameInstanceWaveDefense.cpp`, `WaveDefensePersistenceTests`; 수정 `Public/Game/TunaSweeperGameInstance.h`, `Private/Game/TunaSweeperGameInstanceSave.cpp`, `Public/Inventory/TunaSweeperSaveGame.h`, `Public/Subsystem/TunaSweeperQuestSubsystem.h`, `Private/Subsystem/TunaSweeperQuestSubsystem.cpp`; `Docs/save_persistence.md`.

**Interfaces:** GameInstance의 `TryBeginWaveDefenseRun(FName EncounterId, int32 RewardCoins, int32 DifficultyStage, FGuid& OutRunId) -> bool`, `TryFinalizeWaveDefenseRun(FName EncounterId, FGuid RunId, EWaveDefenseOutcome Outcome, int32 CompletedWave) -> bool`, `TryClaimWaveDefenseReward(FName EncounterId, FGuid RunId) -> bool`. 저장 필드는 `TMap<FName, FWaveDefenseProgress> WaveDefenseProgress`다.

코인 처리에는 기존 `AddCoins`의 즉시 알림·저장 동작을 먼저 확인한다. 필요한 최소 확장으로 `UTunaSweeperQuestSubsystem::TryCommitCoinGrant(int32 Amount, TFunctionRef<bool()> Persist) -> bool`를 추가한다. 음수·int32 오버플로·재진입을 거절하고 임시 코인 변경→Persist→실패 롤백→성공 알림 순서를 보장한다. GameInstance는 보상 플래그 변경과 코인 변경을 동일 `SaveGameStateInternal`에 포함한다.

- [ ] `TunaSweeper.WaveDefense.Persistence`에 이전 세이브의 빈 기록 로드, 슬롯 분리, begin 쓰기 실패, success 쓰기 실패, claim 연타/쓰기 실패/재로드 테스트를 작성하고 실패를 확인한다. 테스트 전용 저장 루트를 사용한다.
- [ ] 시작 시 Active 레코드를 먼저 저장한다. 이미 Active이거나 PendingReward가 있으면 시작을 거절한다. 저장 성공 전에는 스폰하지 않는다.
- [ ] 성공/실패의 한 번뿐인 결과 적용과 누적 기록을 구현한다. 같은 RunId의 반복 finalize는 이미 확정한 결과를 변경하거나 횟수를 늘리지 않는다. 성공 저장 실패 시 메모리의 기존 영속 레코드는 복원하고 Director가 종결 정보를 유지해 재시도한다.
- [ ] 성공300코인과 `bRewardPending=false`의 원자 수령을 구현한다. 시작 당시 보상량을 기록하여 DataAsset 수정 뒤에도 과거 보상이 바뀌지 않게 한다. 실패 시 코인·플래그·알림 모두 이전 상태를 유지한다.
- [ ] load 시 Active 레코드는 Interrupted로 정리하고 진행을 재생성하지 않는다. 정리 저장이 실패하면 재시도 전 시작을 막는다. 성공 미수령 기록은 사망·재로드 뒤 유지한다. 세이브 전환·새 게임·전투 실험실 격리를 연결한다.
- [ ] 성공 결과의 저장 실패 상태에서 사망·탈출·지역 이동·메뉴 종료하면 미저장 결과를 폐기하고 Interrupted로 처리한다. 현재 월드에서의 재시도 성공, 이동으로 인한 폐기, 이미 저장된 미수령 보상의 보존을 각각 테스트한다.
- [ ] 선택 난이도는 회차 동안 변경 불가이며 변경 시도는 무소모로 거절한다. 소모품·일반 인벤토리 저장은 기존 규칙을 유지하고 회차 시작 장비로 되돌리지 않는다.
- [ ] 새 필드·호환·사망·중단·쓰기 실패 정책을 저장 계약에 기록하고 테스트를 통과시킨다.

**완료 조건:** 지급 알림은 저장된 코인과 일치하고 진행 중 강제 종료는 중복 보상을 만들지 않는다.

## Task 6: 단말·WBP·독립 거점 에셋

**Files:** 새 Terminal, 실제 모델명 경로의 HUD/Panel 클래스, `Private/Player/TunaSweeperPlayerControllerWaveDefense.cpp`, `WaveDefenseAssetsTests`, `Tools/WaveDefense/verify_assets.py`; 수정 `Public/Player/TunaSweeperPlayerController.h`, `Public/Interaction/TunaSweeperInteractableComponent.h`, `Private/Subsystem/TunaSweeperInteractionSubsystem.cpp`, `TunaSweeper/Content/Data/UITextStrings.csv`, `TunaSweeper/Content/MainRaid/RaidPlains.umap`.

**Assets:** `/Game/MainRaid/WaveDefense/`의 `BP_WaveDefenseDirector`, `BP_WaveDefenseCore`, `BP_WaveDefenseTerminal`, `BP_WaveDefenseSpawnPoint`, `BP_WaveDefenseBarrier`, `BP_WaveDefenseTurret`, `DA_PlainsWaveDefense`; 하위 `UI/`의 `WBP_WaveDefenseHUD_<실제모델>`, `WBP_WaveDefensePanel_<실제모델>`.

**Interfaces:** `AWaveDefenseTerminal::OpenFor(APawn* Player) -> bool`; Controller의 `OpenWaveDefensePanel(AWaveDefenseDirector*) -> bool`, `CloseWaveDefensePanel() -> void`. HUD/Panel은 `FWaveDefenseSnapshot` 및 시설 목록을 받아 표시하고 Task 3~5 API에 명령을 전달한다. 새로운 interaction enum 값은 구현 시 현재 마지막 값 뒤에 추가하며 기존 숫자를 재사용하지 않는다.

- [x] 에디터 조정 문서를 읽고 맵의 기존 액터·동선·해당 영역의 지형을 조사한다. 설계 §4 조건에 맞는 독립 부지를 확정하고 새 거점 transform과 기존 보존 대상 목록을 검증기에 기록한다.
- [ ] UE에서 편집 가능한 거점·슬롯·후보 위치를 저장한다. 방벽/포탑 슬롯은 진입로별 1개, 생성 후보는 진입로별 최소2개, 출입은 남쪽이라는 개념을 실제 지형에 맞춰 구현한다. 기존 액터를 임의 이동하지 않는다.
- [ ] 기존 일반 적 클래스·무기·프로필을 검증해 DataAsset에 채운다. soft reference의 cook 포함 경로와 맵에서의 참조를 확인한다. 검토되지 않은 Main 전용 데이터에 의존하지 않는다.
- [ ] 저장된 WBP 트리에서 단말/준비/결과 패널 및 HUD를 작성하고 실제 모델 정보·명명 규칙을 적용한다. 모든 표시 문자열·숫자 포맷·오류를 6개 언어 CSV 키로 연결한다.
- [ ] 시작·미수령 수령·시설 명령·포기·준비 완료를 연결한다. 현재 난이도·기존 사망 손실·보급 필요성을 시작 전에 표시한다. UI 재열기·닫기·월드 종료 때 입력 모드와 포커스를 복원한다.
- [ ] 읽기 전용 검증기로 자산 참조·WBP 바인딩/트리·회차 ID 유일성·설정수·NavMesh 경로·기존 보존 액터를 검사한다. 저장 맵을 새 프로세스에서 재로드해 결과를 확인한다.
- [ ] 실제 카메라와 6개 언어에서 HUD를 확인한다. 1080p 기준 장치·적·생성 대기·자원·방향·이탈 카운트가 가려지지 않으며 빈 문자열/키 원문 표시가 없어야 한다.

**완료 조건:** 기존 평원 탐험 흐름 안에서 독립 거점으로 들어가 미니게임 전체를 조작할 수 있다.

## Task 7: 통합 검증·밸런스·정리

- [ ] 아래 빌드·자동화·맵 검증을 실행하고 새 보고서를 확인한다. 테스트 수·실패·로그 경로를 구현 결과에 기록한다.
- [ ] 각 기존 난이도에서 실제 수동 플레이를 수행한다. 완주·장치파괴·플레이어사망·포기·이탈·재진입·세이브 재로드·미수령 수령·차량탑승을 포함한다. 자동 플레이만으로 재미나 읽기 쉬움을 판정하지 않는다.
- [ ] 기록값은 회차 시간, 웨이브별 시간, 피격, 장치 잔여HP, 플레이어 탄약/치료 소모, 포탑 처치, 수리 횟수, 정체 횟수다. 10~15분 목표와 300코인 가치가 실제 소비에 맞는지 확인하고 조정 내역을 설계와 DataAsset에 함께 기록한다.
- [ ] 임시 생성 코드·startup 재생성·생성 전용 의존성을 제거하고 재빌드·관련 재검증한다. 읽기 전용 검증기는 유지한다.
- [ ] 구현한 파일만 검토하고 요청 로그를 작성한다. 다른 작업의 기존 변경은 포함하지 않는다. 사용자 명시 요청이 있을 때만 해당 작업의 커밋을 수행한다.

## 검증 명령과 판정

다음은 **구현 시 실행할 명령**이며 이번 계획 작성에서 실행한 결과가 아니다. 저장 테스트는 실제 사용자 세이브가 아닌 전용 임시 루트를 강제해야 한다. 에디터 사용 전 조정 절차를 완료한다.

```powershell
$waveProject = Join-Path (Get-Location) 'TunaSweeper/TunaSweeper.uproject'
$waveBuild = 'C:/Program Files/Epic Games/UE_5.7/Engine/Build/BatchFiles/Build.bat'
$waveEditor = 'C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
& $waveBuild TunaSweeperEditor Win64 Development "-Project=$waveProject" -WaitMutex
& $waveEditor $waveProject -unattended -nop4 -NullRHI -nosound `
    '-ExecCmds=Automation RunTests TunaSweeper.WaveDefense' `
    '-TestExit=Automation Test Queue Empty' `
    '-ReportExportPath=Saved/Automation/WaveDefense'
& $waveEditor $waveProject -unattended -nop4 -NullRHI `
    '-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities' `
    "-ExecutePythonScript=$((Join-Path (Get-Location) 'Tools/WaveDefense/verify_assets.py'))"
```

단계별 자동화는 `TunaSweeper.WaveDefense` 대신 각 Task의 세부 필터를 사용한다. exit code만으로 판정하지 않고 최신 보고서에 의도한 테스트가 1개 이상 실행됐으며 실패 0개인지 확인한다. 에셋 검증기는 `TunaSweeper/Saved/WaveDefense/verification.json`에 현재 실행의 `passed=true`와 검사 목록을 기록하고 에러 시 실패 종료한다. 렌더링·UI·수동 조작은 NullRHI 검증에 포함되지 않는다.

현재 단계의 산출물은 설계안과 이 구현 계획이다. 체크박스를 미리 완료 처리하거나 구현·플레이 검증이 끝났다고 기록하지 않는다.

## 중간 커밋 — 2026-11-09

사용자 요청으로 중단 상태를 보존한다. 런타임·테스트·문자열·BP/WBP/DataAsset 초안이며 완성본이 아니다. 마지막 보고 빌드는 WaveDefense-13 성공, 자동화는 WaveDefense09 5개 성공이지만 이후 전체 변경은 미검증이다. 실제 평원 배치·에셋 재로드·6웨이브 통합·실플레이·회귀검증·최종리뷰가 남았다. WaveDefenseGenerateWidgets.cpp는 재개용 임시 생성기로 남겨두며 최종 완료 전에 제거·재검증한다. 재개 시 Saved/WaveDefense와 관련 빌드/자동화 로그를 먼저 확인한다.


## 재개 후 두 번째 중간 커밋 — 2026-11-10

사용자 요청으로 다시 일시 중단했다. 평원 배치와 NavMesh 6경로, 실제 66마리/6웨이브 자동 통합, 슬롯·저장실패·AI·화면 회귀를 진행했다. 기존 리뷰9건/UI후속3건은 제한 정적 재검토에서 해결됐다. 러시아어 시설 버튼 줄바꿈 수정의 최종 에셋 반영·언어 재검증, 실제 수동 플레이/밸런스, 임시 생성기 제거와 최종 재빌드·회귀는 남아 있다. `Saved/WaveDefense/implementation_report.md`와 검증 보고서를 읽고 재개한다. 일회성 `WaveDefense*Once.cpp` 및 `WaveDefenseGenerateWidgets.cpp`는 재개용으로 보존한 미완성 작업물이며 최종 제출 전 제거한다. 자동 PIE의 시간 가속·피해 주입은 수동 밸런스 검증을 대신하지 않는다.

## 사용자 검증으로 인계 — 2026-11-10

사용자가 추가 검증 중단과 현재 상태 커밋을 지시했다. 러시아어 시설 버튼을 두 줄로 나눈 WBP를 저장했고 새 프로세스 재컴파일·6언어 렌더를 확인했다. 난이도1 수동 시험은112.9초에 플레이어 사망으로 끝났다. 수동 완주·밸런스·최종 수용 판정은 사용자 확인 대상이다. 이 지시 이후 자동/수동 검증을 추가 실행하지 않는다. 기존 일회성 생성기 및 검증 보조 코드 정리는 아직 수행하지 않았으며, 완성 검증을 모두 통과한 것으로 표시하지 않는다.

