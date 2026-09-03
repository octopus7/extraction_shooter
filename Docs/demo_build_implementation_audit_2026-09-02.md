# 데모 빌드 전 구현 점검

- 점검일: 2026-09-02
- 대상: UE 5.7 `TunaSweeper` 데모 타깃과 데모에 포함되는 `IntroMap`, `BunkerMap`, `DemoRaidMap`
- 중점: 실행 중단, 진행 막힘, 저장 손상, 데모 빌드·콘텐츠 누락처럼 실제 데모에 영향을 주는 문제
- 제외: 당장 데모에 영향이 없는 확장성·대규모 구조 개선

## 결론

현재 코드와 데이터에서 **데모 빌드를 즉시 중단해야 할 정도로 확인된 치명 결함은 없다.** `TunaSweeperDemo Win64 Shipping` 타깃이 빌드됐고, 데모 퀘스트 데이터 검증과 주요 데이터 참조 검증도 통과했다.

다만 데모 시작 맵과 데모 적에서 반복 재현되는 오류성 로그 2건은 출시 후보를 만들기 전에 정리하는 것이 일반적이다. 둘 다 현재 확인 범위에서는 하드 크래시나 진행 불능으로 이어지지는 않았지만, 실제 시각 기능 누락을 숨기거나 이후 검증 결과를 오염시킬 수 있다.

## 이건 무조건 고쳐야 한다

### 확인된 항목 없음

정적 검토, Development Editor 빌드, Demo Shipping 빌드, 자동화와 데이터 검증 범위에서는 크래시, 저장 손상, 새 게임·맵 분기 자체를 막는 확정 결함을 찾지 못했다.

단, 이번 점검에서는 기존 패키지 출력물을 지우고 다시 만드는 전체 `BuildCookRun`과 패키지 실행 플레이를 수행하지 않았다. 따라서 이 결론은 **클린 cook/package와 실기 플레이를 통과했다는 의미는 아니다.** 데모 후보 확정 전에는 반드시 별도 게이트로 수행해야 한다.

## 고치는 게 일반적이다

### 1. 사족보행 적 기본 객체 생성 중 Niagara 에셋 로드가 매 실행마다 `ensure`를 발생시킨다

근거:

- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperQuadrupedEnemyCharacter.cpp:50`
- `NS_Explosion_SmokeRobot`을 네이티브 생성자에서 `ConstructorHelpers::FObjectFinder`로 동기 로드한다.
- 이 과정에서 Niagara 내부 `NE_PostProcess`의 `CameraShakeSourceComponent`가 typed element registry 준비 전에 요청되어 handled ensure가 발생한다.
- 자동화 로그의 콜스택도 위 생성자 50행을 직접 가리킨다.
- 사족보행 적은 데모 레이드에 실제 포함되므로 사용되지 않는 코드의 경고로만 볼 수 없다.

영향:

- 에디터와 명령행 검증 시작 때마다 오류 콜스택과 handled ensure 크래시 리포트가 생성된다.
- 이번 실행은 계속 진행됐고 Shipping 타깃도 빌드됐으므로 현재 하드 크래시는 확인되지 않았다.
- 다만 사망 이펙트 자체의 일부 렌더러가 잘못 초기화되는 문제를 가릴 수 있고, 깨끗한 자동화 판정을 방해한다.

제안:

- 이미 `DeathNiagaraEffect`가 `TSoftObjectPtr`이고 실제 사망 시 `LoadSynchronous()`를 호출하므로, 생성자에서는 에셋을 로드하지 말고 `FSoftObjectPath`만 지정한다.
- 필요하지 않은 `NE_PostProcess`/카메라 셰이크 렌더러가 이펙트에 포함된 것이라면 제거하거나 다시 저장한다.
- 수정 후 에디터와 `UnrealEditor-Cmd`를 각각 한 번 새로 시작해 handled ensure가 사라졌는지 확인한다.

### 2. 데모 시작 맵의 `BP_Morph` Construction Script가 없는 액터를 세 번 참조한다

근거:

- `IntroMap` 로드 직후 `/Game/LunaFacialTemp/BP_Morph`의 `UserConstructionScript`에서 `GetActorOfClass` 반환값 `Accessed None` 3건과 `Attempted to assign to None` 3건이 반복된다.
- 데모의 기본 시작 맵이 `IntroMap`이므로 모든 사용자 경로에 걸린다.
- Map Check는 0 오류·0 경고지만 Construction Script 런타임 경고는 별도로 재현된다.

영향:

- 현재는 맵 로드가 중단되지 않는다.
- 해당 Morph 연결이 실제 타이틀 캐릭터 표정에 필요한 것이라면 일부 얼굴 표현이 조용히 빠질 수 있다.
- 사용하지 않는 임시 액터라면 매 실행마다 불필요한 오류만 남긴다.

제안:

- `BP_Morph`가 현재 Luna Mk2 타이틀 구성에 불필요하면 `IntroMap`에서 제거한다.
- 필요하면 세 `GetActorOfClass` 결과에 각각 유효성 검사를 넣고, 필수 대상 액터가 누락된 경우 한 번만 알아볼 수 있는 경고를 남긴다.
- 타이틀에서 표정·머리·안구 추적을 실제로 확인한다.

## 고칠지 고민해 봐야 한다

### 1. 벙커에서 이어하기 할 때 미완료 시나리오의 자동 재시작 여부

`ATunaSweeperPlayerController::BeginPlay()`는 벙커 진입 대기 상태를 이번 로드에서 완료했을 때만 `level_entered` 시나리오 트리거를 큐에 넣는다. 새 게임의 최초 벙커 진입은 정상이나, 벙커 저장에서 바로 이어하기 한 경우에는 자동 트리거가 생기지 않는다. 같은 시나리오가 `interaction.mole`로도 시작되므로 진행 불능은 아니지만, 최초 대화를 끝내기 전에 종료한 사용자가 이어하기 했을 때 자동 재개되어야 하는지는 의도 확인이 필요하다.

제안: 자동 재개가 의도라면 벙커에서도 항상 `level_entered`를 큐에 넣고 완료 플래그의 one-shot 판정으로 중복을 막는다. 두더지 상호작용으로만 재시작하게 할 의도라면 현재 동작을 유지한다.

### 2. `ui.common.confirm` 다국어 키가 두 번 정의되어 뒤 행이 앞 행을 덮어쓴다

`UITextStrings.csv`의 같은 키가 한국어 `결정`/`확인`, 일본어 `決定`/`確認` 두 버전으로 존재한다. 로더는 경고 후 뒤 행을 사용하므로 실행은 되지만, 난이도·그래픽 화면이 같은 키를 사용하면서 화면별 의도와 다른 문구가 나올 수 있다.

제안: 두 의미가 같다면 한 행으로 통일하고, 화면별 의미를 구분하려면 `confirm_selection`과 `confirm_dialog`처럼 키를 나눈다.

## 그 아래 우선순위

- 자동화 37개 중 36개 성공, 1개 실패: 현재 타이틀에서 이미 교체된 `BP_GazeTestRobot`이 `IntroMap`에 1개 있어야 한다는 오래된 배치 검사다. 런타임 결함보다는 테스트 계약을 현재 타이틀 구성에 맞게 정리할 항목이다.
- `RegionalGroundFog`의 생성 머터리얼이 SM6 컴파일에 실패해 기본 머터리얼로 대체된다. 데모 3개 맵에서 해당 에셋 참조는 찾지 못했으므로 현재 데모 우선순위는 낮다.
- 플레이어 총기 발사는 발사 성공 뒤 탄약을 차감한다. 단일 플레이와 사전 탄약 검사 때문에 현재 위험은 낮지만, 향후 발사 중 인벤토리 상태가 변할 수 있게 되면 발사·차감을 한 동작으로 묶는 편이 안전하다.

## 검증 결과

- `TunaSweeperEditor Win64 Development`: 성공, 이후 에디터 실행
- `TunaSweeperDemo Win64 Shipping`: 전체 컴파일·링크 성공
- `TunaSweeper.*` 자동화: 37개 실행, 36개 성공, 1개 실패
  - 저장 버전 정책, 손상 후보 fail-closed, 전투 데이터, 빌드 flavor, 연구, UI 주요 테스트 성공
  - 실패 1개는 위의 오래된 테스트 로봇 배치 조건
- 데모 퀘스트 검증: 성공, 현재 데모 퀘스트 0개는 현재 데이터 구성과 일치
- `Content/Data`: JSON 30개 파싱 성공, CSV 4개 필수 셀 확인
- 아이템·루팅·상점·작업대·적 스폰 프로필의 ID 교차 참조: 누락 없음
- JSON의 런타임 `/Game` 에셋 경로: 누락 없음
- 데모 맵 구성: `IntroMap`, `BunkerMap`, `DemoRaidMap` 존재 및 Demo cook 목록에 포함, `MainRaid/RaidMap`은 Demo cook에서 제외

## 데모 후보를 만들 때 권장 순서

1. 사족보행 적 Niagara 생성자 로드 `ensure` 제거
2. `IntroMap`의 `BP_Morph` 참조 오류 제거 또는 유효성 처리
3. 오래된 시선 테스트 로봇 배치 검사를 현재 타이틀 계약에 맞게 수정
4. 클린 Demo `BuildCookRun` 수행
5. 패키지 실행으로 `새 게임 → 벙커 → 데모 레이드 → 탈출 → 재실행/이어하기`와 `사망 → 인벤토리 정리 → 재실행`을 각각 한 번 스모크 테스트

