# Miyakov 클래스 이식 후보 분석

작성일: 2026-07-08

대상 프로젝트: `D:\github\miyakov\Miyakov`

적용 검토 대상: `TunaSweeper/TunaSweeper.uproject`

## 결론

`../miyakov/Miyakov` 프로젝트는 런타임 본체보다 플러그인 모듈에 재사용 가능한 코드가 많다. 다만 TunaSweeper는 이미 인벤토리, 아이템 데이터, 상호작용, 시야, 무기/투사체, 레벨 전환 시스템을 프로젝트 고유 방식으로 갖고 있으므로, 대부분은 그대로 복사하면 중복과 충돌이 크다.

가져올 만한 항목은 다음 네 가지가 중심이다.

1. 범용 모달 팝업: `DCPopupUI`의 구조를 TunaSweeper 스타일로 재작성
2. 투척물 시스템: `UThrowableComponent`, `AThrowableGrenade`, `UThrowableConfig` 부분 포팅
3. 데미지 정책: `UDamageCalculator`, `UDamagePolicy_Tarkov`, `FDamageContext` 선택 포팅
4. 펫/동료 추적: `APetCompanionCharacter`, `APetCompanionAIController`를 TunaSweeper 캐릭터 기준으로 수정 포팅

이미 사족보행 쪽은 별도 작업으로 `QuadrupedCharacter`, `QuadrupedComponent`를 현재 프로젝트에 최소 런타임 플러그인 형태로 이식한 상태이므로, 이 문서에서는 후속 후보와 비추천 대상을 중심으로 정리한다.

## Miyakov 구조 요약

Miyakov 본체 모듈은 비교적 작고, 실제 시스템은 플러그인으로 나뉘어 있다.

- `DCPopupUI`: 모달 팝업 UI
- `MiyakovCharacterSystem`: 캐릭터, 체력, 데미지 정책, 경험치, 적 AI, 펫, 사족보행, 사운드 감지
- `MiyakovEnvironmentSystem`: 나무/환경 fade 컴포넌트
- `MiyakovInteractionSystem`: 상호작용, 문, 상자, 레벨 전환, 로딩 화면
- `MiyakovInventorySystem`: 인벤토리, 장비, 퀵슬롯, 창고, 상점, 루팅
- `MiyakovItemDataSystem`: JSON 기반 아이템 데이터, 소유 아이템 인스턴스, 부착물 데이터
- `MiyakovVisionSystem`: raycast/navmesh 기반 시야 및 fog of war
- `MiyakovWeaponSystem`: 무기 서브시스템, 확산, 사운드, 효과, 레이저, 투척물
- `OctoOpusMCP`: 에디터/외부 제어 성격으로 게임 런타임 이식 대상 아님

TunaSweeper에는 동일 영역의 시스템이 이미 많다. 그래서 포팅 판단은 “현재 시스템 공백을 메우는가”, “기존 데이터/저장/상호작용 경계와 맞는가”, “UE 5.7에서 유지보수 가능한가”를 기준으로 봤다.

## 우선순위별 후보

### 1. 범용 모달 팝업

원본 클래스:

- `UPopupSubsystem`
- `UPopupWidget`
- `UPopupThemeDataAsset`
- `UPopupUISettings`
- `FPopupPayload`

판정: 부분 재작성 추천

TunaSweeper에는 `UTunaSweeperToastSubsystem`, `UTunaSweeperItemStackSplitPopupWidget`, 인트로 메뉴 내부 확인 패널 등 개별 UI는 있지만, 게임 전역에서 쓰는 확인/취소 모달 서브시스템은 없다. 저장 삭제 확인, 설정 변경 확인, 추출 확인, 위험 행동 확인 같은 곳에 재사용할 수 있다.

그대로 복사하지 않는 편이 낫다. Miyakov 쪽은 문서와 일부 기본 문자열 인코딩이 깨져 있고, 현재 구현은 단일 active popup만 허용한다. 또한 TunaSweeper UI는 `TunaSweeperUIFont`, 현지화 string key, 코드 기반 WidgetTree 생성 패턴을 이미 많이 쓰므로, `DCPopupUI` 플러그인 전체보다 아래처럼 프로젝트 네이티브로 흡수하는 방식이 맞다.

권장 형태:

- `UTunaSweeperPopupSubsystem : ULocalPlayerSubsystem`
- `UTunaSweeperPopupWidget : UUserWidget`
- `FTunaSweeperPopupPayload`
- `ETunaSweeperPopupResult`
- 기본 버튼 텍스트는 `UTunaSweeperTextSubsystem` 또는 `UTunaSweeperGameInstance::ResolveLocalizedText` 사용
- 팝업 큐는 최소 1단계 후속 기능으로 둠

포팅 난이도: 낮음

리스크: 입력 모드 복원, 기존 인벤토리/대화 UI와 충돌 가능성

### 2. 투척물 시스템

원본 클래스:

- `UThrowableComponent`
- `AThrowableGrenade`
- `UThrowableConfig`
- `FThrowableEntry`
- `UThrowableDamageType`, `UGrenadeDamageType`, `UDynamiteDamageType`
- `AThrowableDestructible`

판정: 기능 단위 포팅 추천

TunaSweeper의 무기/투사체는 이미 `ATunaSweeperProjectile`, `UTunaSweeperWeaponCombatComponent`, `ATunaSweeperWeapon`, `UTunaSweeperProjectileHitEffectDataAsset` 중심으로 구현되어 있다. 그래서 `UWeaponSubsystem` 전체는 중복이지만, 투척물 조준과 포물선 프리뷰, 폭발 처리만 따로 가져오면 현재 시스템에 없는 기능을 빠르게 추가할 수 있다.

그대로 복사할 때 맞지 않는 부분:

- Miyakov는 `ItemId`와 `UThrowableConfig` DataAsset 매핑 중심이다.
- TunaSweeper는 `FTunaSweeperItemInstance`의 `FGuid` 기반 인스턴스, `UTunaSweeperItemDataSubsystem`, 장비/퀵슬롯/선택 탄약 구조를 쓴다.
- Miyakov 구현은 debug circle을 상시 그릴 수 있고, 폭발 소음/AI 경보 연동은 없다.

권장 포팅 방향:

- `UTunaSweeperThrowableComponent`를 플레이어 캐릭터에 붙인다.
- 아이템 소비는 `UTunaSweeperGameInstance` 인벤토리 API와 연결한다.
- 조준 위치는 기존 top-down cursor/aim 시스템에서 받는다.
- 폭발 피해는 `UGameplayStatics::ApplyRadialDamage` 또는 TunaSweeper 전투 처리 함수로 통합한다.
- 폭발 사운드는 `UTunaSweeperNoiseSubsystem::ReportNoiseAtLocation`으로 AI 청각에 연결한다.
- 효과는 Niagara 직접 참조보다 TunaSweeper의 효과 DataAsset 방식에 맞춘다.

포팅 난이도: 중간

리스크: 아이템 소비, UI 프리뷰, 피해/소음/저장 상태 연결을 함께 맞춰야 함

### 3. 데미지 정책

원본 클래스:

- `UDamageCalculator`
- `UDamagePolicy`
- `UDamagePolicy_Simple`
- `UDamagePolicy_Tarkov`
- `FDamageContext`
- `FDamageResult`

판정: 현재 전투 규칙을 확장할 때만 선택 포팅

Miyakov의 데미지 정책은 단독성이 높다. 특히 `UDamagePolicy_Tarkov`는 관통력과 방어구 등급으로 관통 확률을 계산하고, 실패 시 blunt damage만 적용하는 구조다. 탄약/방어구 세분화를 할 계획이 있으면 좋은 참고 코드다.

다만 현재 TunaSweeper의 `Docs/game_conventions.md` 전투 기준은 단순하다.

- 탄약 최종 피해는 기본 투사체 피해와 탄약 보너스로 계산
- 플레이어 피해는 장비 방어 수치 합계를 뺀 `max(0, damage - defense)`
- 레벨 기반 공격력/방어력 증가는 없음

따라서 지금 당장 이식하면 기존 게임 기준을 바꾸는 작업이 된다. 실제로 도입하려면 문서 기준, 아이템 데이터 필드, HUD 피해 표시, 저장/장비 계산을 함께 수정해야 한다.

권장 포팅 방향:

- 먼저 `FTunaSweeperDamageContext`, `FTunaSweeperDamageResult`로 네이밍을 바꿔 프로젝트 네이티브화한다.
- 기본 정책은 기존 단순 방어 차감으로 둔다.
- 관통 정책은 옵션 또는 특정 난이도/탄약 타입으로만 사용한다.
- 도입 시 `Docs/game_conventions.md`의 전투 수치 기준을 업데이트한다.

포팅 난이도: 낮음에서 중간

리스크: 게임 밸런스와 기존 전투 기준 변경

### 4. 펫/동료 추적

원본 클래스:

- `APetCompanionCharacter`
- `APetCompanionAIController`

판정: 동료 기능을 만들 계획이면 시작점으로 적합

Miyakov의 펫 동료는 전투 없는 ghost-like companion이다. `AAIController::MoveToActor`를 일정 간격으로 호출하고, 거리별로 걷기/달리기 속도를 바꾸는 단순한 구조다. TunaSweeper에 동료 NPC, 드론, 보조 로봇 같은 기능을 추가한다면 빠르게 가져올 수 있다.

수정 필요점:

- `APlayerCharacter` 의존을 `ATunaSweeperTopDownCharacter` 또는 `APawn` 기반으로 바꿔야 한다.
- 충돌 정책과 NavMesh 사용 여부를 TunaSweeper 이동/전투 공간에 맞춰야 한다.
- 저장이 필요한 동료라면 `Docs/save_persistence.md`와 save data를 업데이트해야 한다.
- 동료가 인벤토리, 펫 가방, 퀘스트 상태를 갖는다면 Miyakov 인벤토리와 섞지 말고 TunaSweeper 인벤토리 구조를 사용해야 한다.

포팅 난이도: 중간

리스크: pathfinding, collision, save/load, 동료 상태 UI

## 이미 처리된 후보

### 사족보행

원본 클래스:

- `AQuadrupedCharacter`
- `UQuadrupedComponent`
- `FQuadrupedLegData`

판정: 이미 최소 런타임 플러그인 형태로 이식됨

`UQuadrupedComponent`는 프로젝트 의존이 낮고, 다리 IK 타겟 계산과 diagonal gait 처리가 한 컴포넌트에 모여 있다. 이미 `TunaSweeper/Plugins/MiyakovCharacterSystem`에 이식한 상태다. 후속 작업은 실제 애니메이션 블루프린트, IK 연결, TunaSweeper 캐릭터/AI 체계와의 통합 여부를 보는 단계다.

## 참고만 할 후보

### Tree fade

원본 클래스:

- `UTreeFadeComponent`

판정: 필요 시 부분 참고

플레이어가 나무 아래에 들어가면 머티리얼 파라미터로 dissolve를 거는 구조다. TunaSweeper에는 이미 `UTunaSweeperRevealOccluderComponent`, `UTunaSweeperOcclusionRevealSourceComponent`, transparent obstacle spawn 데이터가 있어 같은 문제를 다른 방식으로 풀고 있다.

나무/식생 전용 fade가 필요하면 로직은 참고할 수 있지만, 기존 occlusion reveal 머티리얼 파라미터 컬렉션 구조와 합치는 편이 낫다.

### Camera view interaction

원본 클래스:

- `ACameraViewInteractionActor`
- `UCameraViewerWidget`

판정: 특정 감시카메라/관찰 상호작용이 필요할 때만 참고

상호작용하면 레벨 카메라로 전환하고 UI 버튼으로 돌아오는 기능이다. TunaSweeper에는 대화 카메라, 레벨 전환, 상호작용 흐름이 이미 있으므로 전체 포팅보다 특정 기능 구현 시 참고하는 정도가 적절하다.

### VFX DataAsset

원본 클래스:

- `UVFXDataAsset`

판정: 비추천

단순 Niagara 참조 묶음이다. TunaSweeper는 이미 projectile hit effect, local explosion, melee impact 등 도메인별 효과 클래스를 갖고 있다. 별도 범용 VFX DataAsset을 추가하면 효과 관리 기준이 분산될 가능성이 크다.

## 포팅 비추천

### 인벤토리와 아이템 데이터

원본 모듈:

- `MiyakovInventorySystem`
- `MiyakovItemDataSystem`

대표 클래스:

- `UInventoryComponent`
- `UEquipmentComponent`
- `UQuickSlotComponent`
- `ULootContainerSubsystem`
- `UTradeSubsystem`
- `UWarehouseSubsystem`
- `UItemDataSubsystem`
- `UOwnedItemSubsystem`

비추천 이유:

- TunaSweeper는 이미 `FTunaSweeperItemInstance`와 `FGuid` 기반 슬롯 구조를 사용한다.
- 상점, 창고, 루팅, 워크벤치, 분해, 장비, 탄약, 부착물, 저장까지 기존 GameInstance와 연결되어 있다.
- Miyakov는 `int64 InstanceId`, `items.json`, `loot_tables.json`, `shops.json`, `npcs.json` 기준이라 데이터 포맷과 런타임 소유권이 다르다.

가져올 수 있는 것은 구조 아이디어 정도다.

- 슬롯은 인스턴스 참조만 들고 실제 아이템 데이터는 중앙 저장소가 가진다는 구조
- 루트 테이블에서 guaranteed/random item을 나누는 방식
- 장비 슬롯과 선택 탄약을 별도 map으로 관리하는 방식

### 상호작용과 레벨 전환

원본 모듈:

- `MiyakovInteractionSystem`

대표 클래스:

- `UInteractionComponent`
- `AInteractableActor`
- `ALevelTransitionActor`
- `AExtractionPointActor`
- `ADoorActor`
- `ACrateActor`

비추천 이유:

- TunaSweeper는 `UTunaSweeperInteractableComponent`, `UTunaSweeperInteractionSubsystem`, `ATunaSweeperLevelTravelInteractableActor`, `ATunaSweeperWarpPointActor`, `ATunaSweeperExtractionPointActor`를 이미 갖고 있다.
- 기존 구현은 퀘스트, HUD marker, persistent world state, save/load, level transition subsystem과 연결되어 있다.
- Miyakov의 `ALevelTransitionActor`는 단순해서 현재 TunaSweeper 기능보다 낮은 단계다.

참고할 만한 부분은 같은 레벨 안에서 `PlayerStartTag`를 찾아 fade 후 teleport하는 정도다.

### 시야와 fog of war

원본 모듈:

- `MiyakovVisionSystem`

대표 클래스:

- `UVisionComponent`
- `UVisionSubsystem`
- `UNavMeshVisibilityComponent`
- `AFogOfWarPostProcess`

비추천 이유:

- TunaSweeper는 이미 `UTunaSweeperPlayerVisionComponent`, `UTunaSweeperVisionMaskWidget`, `UTunaSweeperVisionVisibilitySubsystem`, `UTunaSweeperVisionSubjectComponent`를 갖고 있다.
- Miyakov `UVisionSubsystem`은 렌더 타깃 해상도가 고정값 중심이고, post process/fog material 흐름이 TunaSweeper UI mask 흐름과 다르다.
- `UNavMeshVisibilityComponent`는 Recast/Detour 내부 API를 직접 읽기 때문에 UE 5.7 유지보수 리스크가 있다.

참고할 만한 부분은 NavMesh boundary 기반 visibility polygon 아이디어뿐이다. 실제 도입은 별도 실험 브랜치에서 검증하는 편이 안전하다.

### 무기 서브시스템과 레이저

원본 클래스:

- `UWeaponSubsystem`
- `UWeaponSpreadConfig`
- `UWeaponSoundConfig`
- `UProjectileEffectConfig`
- `UWeaponBlueprintConfig`
- `ULaserSightComponent`
- `UCrosshairWidget`

비추천 이유:

- TunaSweeper에는 이미 `UTunaSweeperWeaponCombatComponent`, `UTunaSweeperWeaponSpreadRecoilDataAsset`, `UTunaSweeperLaserSightComponent`, `ATunaSweeperProjectile`, `UTunaSweeperProjectileHitEffectDataAsset`가 있다.
- Miyakov `UWeaponSubsystem`은 GameInstanceSubsystem 하나에 ammo, reload, spread, effect, sound, blueprint map을 몰아둔 구조다.
- TunaSweeper는 무기 actor, 캐릭터, 게임 인스턴스, 아이템 데이터가 역할별로 나뉘어 있어 구조가 맞지 않는다.

가져올 수 있는 것은 `UWeaponSoundConfig`의 surface type별 impact sound map 같은 작은 아이디어 정도다.

### 적/스포너/데미지 존

원본 클래스:

- `AEnemySpawner`
- `AEnemyCharacter`
- `AEnemyAIController`
- `UEnemyDataSubsystem`
- `ADamageZoneActor`

비추천 이유:

- TunaSweeper에는 `UTunaSweeperEnemySpawnSubsystem`, `ATunaSweeperEnemyCharacter`, `ATunaSweeperEnemyAIController`, `ATunaSweeperRollingBomberSpawner` 등이 있다.
- Miyakov `AEnemySpawner`는 간단한 editor icon과 BeginPlay spawn 중심이다.
- `ADamageZoneActor`는 `ABaseCharacter`에 직접 의존하고 debug draw가 강해 TunaSweeper의 vitals/interaction 기준에 맞게 다시 쓰는 편이 빠르다.

## 권장 작업 순서

1. `UTunaSweeperPopupSubsystem` 추가
   - 범용 확인/취소 UI로 가장 독립적이고 재사용성이 높다.
   - 기존 개별 confirm UI를 점진적으로 대체할 수 있다.

2. 투척물 MVP 추가
   - 수류탄 아이템 1종, 조준 프리뷰, 폭발 피해, 폭발 소음까지 작게 묶는다.
   - 인벤토리 소비와 퀵슬롯 사용 흐름을 먼저 고정한다.

3. 데미지 정책 실험
   - 기존 단순 방어 차감을 유지한 채 관통 정책을 별도 함수로 실험한다.
   - 밸런스가 확정되면 `Docs/game_conventions.md`를 업데이트한다.

4. 펫/동료 follow prototype
   - 전투 없는 follow-only 캐릭터로 시작한다.
   - 저장/가방/전투 기능은 후속 단계로 분리한다.

## 포팅 공통 주의사항

- Miyakov 클래스명을 그대로 유지하지 말고 TunaSweeper 네이밍으로 바꾼다.
- Miyakov의 `ItemId`, `int64 InstanceId` 구조를 그대로 섞지 않는다. TunaSweeper는 `FGuid` 기반 `FTunaSweeperItemInstance`를 기준으로 유지한다.
- 플레이어 타입 직접 의존은 `ATunaSweeperTopDownCharacter` 또는 `APawn` 기반으로 정리한다.
- 새 저장 데이터가 생기면 `Docs/save_persistence.md`를 같이 업데이트한다.
- 전투 규칙이 바뀌면 `Docs/game_conventions.md`를 같이 업데이트한다.
- UE 5.7 기준에서 내부 엔진 API, Recast/Detour 직접 접근, 고정 render target 크기 같은 코드는 별도 검증 없이는 런타임 본선에 넣지 않는다.
