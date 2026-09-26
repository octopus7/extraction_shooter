# UE5 코드의 미구현·임시 구현·추후 제거 항목 조사

조사일: 2026-09-26. 현재 체크아웃의 코드를 검색하고 관련 구현·호출부를 정적으로 대조했다. 아래 항목의 구현이나 삭제는 수행하지 않았다.

## 요약

- **명확하게 미뤄 둔 작업 4건:** 작업대 수리, 지면 종류별 발소리, 영상 촬영용 탄막 모드 제거, 저금통 출금.
- **향후 확장 제안 1건:** SplineWorldBuilder의 정적 컴포넌트 Bake. 주석은 가능성을 언급하며 확정 계획은 아니다.
- 그 밖에 임시 명칭을 가진 연출·개발 도구, 실험 기능, 에셋 배정 주석, 과거 UI 호환 처리가 남아 있다. 현재 사용 여부와 제거 약속의 유무를 각각 표시했다.
- `TODO`, `FIXME`, `HACK`, `TBD`, `WIP`, `stub`의 독립된 단어 표시는 검색되지 않았다. 실제 대상은 문장형 주석과 `Temporary` 등의 식별자에 들어 있었다.

## 조사 범위와 한계

| 범위 | 코드 파일 수 |
|---|---:|
| TunaSweeper/Source: 런타임·에디터·AnimGraph·테스트·Target/Build 규칙 | 608 |
| TunaSweeper/Plugins: 프로젝트에 있는 플러그인 10개 | 80 |
| TunaSweeper/BatchScripts, BuildScripts | 9 |
| 합계 | **697** |

- C++ 377개, 헤더 283개, C# 25개, BAT 8개, PowerShell 1개, Python 1개, 셰이더 `.ush` 2개를 검색했다. 설정 파일과 플러그인 설명 문서는 필요한 문맥 확인에 사용했다.
- 검색어는 `TODO/FIXME`, `temporary/prototype/placeholder/legacy`, `later/future/for now`, `not implemented/NotImplemented`, `remove after`, `임시/추후/나중/향후/미구현/예정` 및 유사 표현이다. `\uXXXX`로 기록된 한글도 디코딩해 검색했다. 검색 결과는 구현과 호출 맥락으로 판별했다.
- 엔진 설치 디렉터리, 외부 의존성, 빌드·캐시·Saved 백업, 저장소 밖 도구는 대상에서 제외했다.
- 바이너리 `.uasset`·`.umap` 안의 블루프린트 그래프·주석·인스턴스 설정은 조사하지 않았다. C++ 기본값이 꺼져 있어도 에셋에서 켰을 가능성은 남아 있다.
- 빌드나 에디터 실행 검증은 수행하지 않았다. 이 목록은 코드에 드러난 유예 표현과 잔여 구현의 조사 결과이며, 문구가 없는 모든 기능 누락까지 증명하는 목록은 아니다.

## 1. 명확한 미구현·추후 제거

### 1-1. 작업대 수리 기능 — 미구현

- 근거: [TunaSweeperWorkbenchActor.h:36](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperWorkbenchActor.h:36)
- 주석: `Repair is intentionally reserved for a future workbench mode; no repair behavior is implemented yet.`
- 현재 액터에는 제작·분해·설계도 등록 상호작용이 구성되어 있고, 수리 동작은 구현되어 있지 않다.
- 남은 일: 수리 규칙, 비용·재료, 대상 장비 상태 변경, UI와 저장 동작을 정의하고 구현해야 한다. 이 세부 범위는 조사에 따른 후속 작업 제안이다.

### 1-2. 지면 종류별 발소리 — 일반화된 선택 로직 미구현

- 근거: [TunaSweeperFootstepPresentationDataAsset.h:9](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Character/TunaSweeperFootstepPresentationDataAsset.h:9)
- 주석: `Surface-specific resolution will extend this asset later.`
- 현재 데이터 에셋에는 `BasicFootstepSound` 하나만 있다. 재생 코드는 웅덩이 전용 소리 → 기본 소리 → 절차 생성 대체음을 사용한다. [재생 구현:186](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterMovement.cpp:186)
- 남은 일: 일반 지면 재질별 발소리 매핑과 선택 경로 확장. 웅덩이 분기는 이미 구현되어 있으므로 모든 지면 구분이 미구현인 것은 아니다.

### 1-3. 영상 촬영용 탄막 모드 — 촬영 후 제거 명시

- 대표 근거: [TunaSweeperEnemyCharacter.cpp:969](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp:969), [TunaSweeperWeapon.h:52](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Weapon/TunaSweeperWeapon.h:52)
- 주석에 `Remove after video capture`, `remove this argument after capture`라고 명시한다. `TEMP_VIDEO_BULLET_STORM`으로 관련 분기를 표시해 놓았다.
- [스위치 선언:243](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/AI/TunaSweeperEnemyCharacter.h:243)의 C++ 기본값은 `false`다. 켜면 피해 0, 가상 무한 탄약, 재장전 무시, 빠른 연사, 넓은 탄착, 발사음 생략과 별도 AI 공격 타이밍이 적용된다.
- 제거 범위는 체크박스 하나보다 넓다. 적 액터, AI, 무기 API, 투사체, 화상 피해 숫자 표시까지 함께 확인해야 한다.

| 관련 코드 | 역할 |
|---|---|
| [EnemyCharacter.cpp:1058](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp:1058) | 탄약·재장전 우회와 피해·탄착·연사 값 변경 |
| [EnemyAIController.cpp:1232](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyAIController.cpp:1232) | 촬영 전용 공격 패턴과 타이밍 |
| [Weapon.cpp:678](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Weapon/TunaSweeperWeapon.cpp:678) | 연사 간격 재정의. 같은 파일 877행에 발사음 생략 분기 |
| [Projectile.cpp:77](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Weapon/TunaSweeperProjectile.cpp:77) | 촬영 모드에 따른 분기 |
| [BurnComponent.cpp:205](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Component/TunaSweeperBurnComponent.cpp:205) | 피해 숫자 표시 생략. 이 참조에는 TEMP 주석이 없어 식별자 검색도 필요 |

촬영이 끝났는지는 코드로 알 수 없다. 완료되었다면 가장 명확한 정리 대상이다.

### 1-4. 저금통 출금 — 미구현 안내만 수행

- 근거: [TunaSweeperPiggyBankActor.cpp:337](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperPiggyBankActor.cpp:337)의 `ShowWithdrawNotImplemented`.
- ‘빼기’ 상호작용은 [InteractionSubsystem.cpp:1016](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperInteractionSubsystem.cpp:1016)을 통해 이 함수로 연결된다.
- 현재는 ‘미구현’ 말풍선을 표시하고 플레이어의 ‘먹었냐?’ 대화를 시작한 뒤 `true`를 반환한다. 이 경로에 저금액 차감이나 아이템 반환은 없다. [대화 구현:473](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperPiggyBankActor.cpp:473)
- 입금은 구현되어 있으므로 출금만 별도 미완료 항목이다. 남은 일은 반환 방식과 저장 반영 구현이다. 화면 문구도 프로젝트 문자열 키로 처리해야 한다.

## 2. 향후 기능으로 제안만 남긴 항목

### 2-1. SplineWorldBuilder 정적 컴포넌트 Bake

- 근거: [SplineWorldBuilderActor.cpp:44](D:/github/extraction_shooter/TunaSweeper/Plugins/SplineWorldBuilder/Source/SplineWorldBuilder/Private/SplineWorldBuilderActor.cpp:44)
- 주석: `A later explicit bake may convert the generated output to static components`.
- 현재 생성 결과는 이동 가능한 HISM 컴포넌트이며 편집 가능한 스플라인과 함께 움직인다. 플러그인 소스의 `Bake` 검색은 이 주석만 반환했다.
- 정적 컴포넌트로 확정하는 Bake 기능은 코드에서 확인되지 않는다. 다만 `may`라는 표현이므로 필수 구현 약속보다는 확장 선택지다.

## 3. 임시·실험 명칭을 가진 구현

아래는 현재 코드가 존재하는 기능이다. 이름만으로 삭제가 필요하거나 기능이 미완성이라고 확정하지 않았다.

| 항목 | 현재 상태와 판단 | 근거 |
|---|---|---|
| 타이틀의 임시 팔 자세 보정 | 본 변환을 직접 보정하는 구현이 남아 있다. 기본값과 확인된 활성 설정 호출이 모두 `false`이며 틱에서 보정 입력값은 계속 갱신한다. 비활성 잔여 구현 정리 후보. | [TitlePresentationActor.cpp:84](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Title/TunaSweeperTitlePresentationActor.cpp:84), [비활성 설정:217](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Title/TunaSweeperTitlePresentationActor.cpp:217) |
| 임시 구르기 시각 회전 | 구르기 시간에 따라 메시를 회전시키고 종료 후 복원한다. C++ 기본값은 `false`. 블루프린트 사용 여부를 확인한 뒤 유지·제거 판단 필요. | [TopDownCharacter.h:450](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Character/TunaSweeperTopDownCharacter.h:450), [Movement.cpp:486](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Character/TunaSweeperTopDownCharacterMovement.cpp:486) |
| 전투 실험실 자동 조종 | `Temporary lab-only pilot`이라고 설명한다. 실제 플레이어 입력을 공급하는 실험 도구이며 제거 시점은 명시하지 않았다. | [CombatLabAutopilotComponent.h:8](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Component/TunaSweeperCombatLabAutopilotComponent.h:8) |
| 물 표면 하늘 시차 반사 | `WATER_SKY_PARALLAX_EXPERIMENT` 표기가 있고 기본 비활성이다. 별도 머티리얼과 셰이더 경로가 구현되어 있으며 README에 제거 방법이 있다. 실험 채택 여부를 결정할 대상. | [StylizedWaterBodyActor.h:76](D:/github/extraction_shooter/TunaSweeper/Plugins/StylizedWater/Source/StylizedWater/Public/StylizedWaterBodyActor.h:76), [README:32](D:/github/extraction_shooter/TunaSweeper/Plugins/StylizedWater/README.md:32) |
| 차고문 임시 벽·지붕 | `TemporaryWallLeft/Right`, `TemporaryRoof`라는 이름으로 메시·충돌·배치에 실제 사용한다. 교체 일정이나 향후 삭제 선언은 없다. | [FoldingCanopyGarageDoorActor.h:198](D:/github/extraction_shooter/TunaSweeper/Plugins/FoldingCanopyGarageDoor/Source/FoldingCanopyGarageDoor/Public/FoldingCanopyGarageDoorActor.h:198), [구현:444](D:/github/extraction_shooter/TunaSweeper/Plugins/FoldingCanopyGarageDoor/Source/FoldingCanopyGarageDoor/Private/FoldingCanopyGarageDoorActor.cpp:444) |
| FM 임시 효과음 제작 도구 | 툴팁에 `temporary FM-style sound effects`라고 설명한다. WAV/SoundWave 내보내기 도구는 구현되어 있다. 개별 임시 음원의 교체 여부는 코드만으로 판단할 수 없다. | [TunaSweeperFMSoundTool.cpp:1371](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperFMSoundTool.cpp:1371) |

## 4. 에셋을 확인해야 남은 작업인지 알 수 있는 주석

| 항목 | 근거 | 판단 |
|---|---|---|
| 쇠지렛대 거치대·쇠지렛대 메시 | [CrowbarWallRackActor.h:54](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperCrowbarWallRackActor.h:54), 같은 파일 58행: `Assign its mesh later in the Blueprint` | 대응 BP 에셋은 존재한다. 메시가 이미 지정되었는지는 이번 텍스트 조사로 확정하지 못했다. |
| 막힌 취수구의 잔해 메시 | [BlockedIntakeScreenActor.h:93](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/Interaction/TunaSweeperBlockedIntakeScreenActor.h:93): `Optional debris visual assigned later` | 선택적 잔해 시각 요소를 나중에 배정한다는 주석. BP·맵 인스턴스 확인 필요. |
| ATV 향후 리모델링 | [TunaSweeperATVRiderTests.cpp:101](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Tests/TunaSweeperATVRiderTests.cpp:101): `future remodel` | 라이더만 렌더링해 다리 검수를 가리지 않도록 한 테스트 설명이다. 모델 교체 완료 여부나 일정의 증거는 아니다. |

## 5. 미구현으로 잘못 집계하기 쉬운 잔여·호환 처리

- **HUD의 ‘미구현’ 패널:** [GameHudWidgetLayout.cpp:195](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetLayout.cpp:195)와 [GameHudWidgetRefresh.cpp:337](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGameHudWidgetRefresh.cpp:337)에 `ui.common.unimplemented`가 남아 있다. 하지만 [HudTypes.h:17](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperHudTypes.h:17)의 정상 enum 값은 모두 구현된 모드이거나 `None`이며, 해당 표시 조건에서 모두 제외된다. 현재 특정 탭의 미구현 증거가 아니라 방어·잔여 분기 정리 후보다.
- **구형 UI 숨김·호환:** [IntroMenuWidgetDelete.cpp:82](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperIntroMenuWidgetDelete.cpp:82)는 과거 삭제 홀드 게이지를 숨기고, [HudBottomStatusWidget.cpp:87](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperHudBottomStatusWidget.cpp:87)는 구형 무게 표시를 숨긴다. [ItemContainerWidget.h:114](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Public/UI/ItemContainerWidget.h:114)에는 구형 HUD BP의 외부 컨테이너 호환 경로가 있다. 관련 BP 이관 여부 확인 전 삭제 근거로 삼을 수 없다.
- **화상 피해의 `future kill credit`:** [BurnComponent.cpp:86](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Component/TunaSweeperBurnComponent.cpp:86)은 투사체가 사라진 뒤 발생할 처치의 귀속 정보를 보존한다. [피해 전달:210](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/Component/TunaSweeperBurnComponent.cpp:210)과 [EnemyCharacter.cpp:1327](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp:1327)에 이미 처치 업적·퀘스트·경험치 반영이 연결되어 있다. 미래 시점의 처치를 뜻할 수 있으므로 미구현 목록에서 제외했다.
- **지도 placeholder:** [MapWidget.cpp:491](D:/github/extraction_shooter/TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperMapWidget.cpp:491)은 지도 정의가 없을 때 대체 텍스처를 로드한다. 정식 지도 자체의 미구현 선언은 아니다.
- **기타 정상 처리:** 저장·장비·그래픽 옵션의 `Legacy` 이관, 비동기 텍스처의 임시 자원, MP4 미리보기용 임시 WAV, 실험실 종료 때 복원하는 임시 장비는 수명·호환성을 설명한다. `/Game/Prototype` 경로와 가림 효과 프로토타입 클래스도 이름만으로 폐기 예정이라고 판정하지 않았다.

## 정리 순서 제안

1. 촬영 완료 여부를 기준으로 `TEMP_VIDEO_BULLET_STORM`과 `IsTemporaryVideoBulletStormEnabled` 관련 분기 전체 제거 여부 결정.
2. 타이틀 임시 팔 자세, 임시 구르기, 물 반사 실험, 차고문 임시 외형의 현재 에셋 사용 여부 확인.
3. 실제 새 기능 작업으로 저금통 출금, 작업대 수리, 지면별 발소리를 관리. Bake는 채택 여부부터 결정.
4. BP·맵을 확인한 뒤 오래된 에셋 배정 주석과 UI 호환 코드를 정리.
