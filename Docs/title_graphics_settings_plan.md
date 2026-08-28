# 타이틀 그래픽 설정 구현 계획

## 1. 확정 정책

- 자동 감지는 실제 렌더링에 사용 중인 어댑터의 **전용 VRAM**만 기준으로 한다.
- 전용 VRAM이 `4GiB` 미만이면 `낮음`, `4GiB` 이상이면 `최고`를 적용한다.
- VRAM을 알 수 없거나 `0`으로 보고되는 통합 GPU는 안전하게 `낮음`으로 처리한다.
- 자동 감지는 `중간`이나 `높음`을 선택하지 않는다. 두 단계는 사용자 수동 선택 전용이다.
- 사용자가 `낮음`, `중간`, `높음`, `최고` 또는 개별 설정을 선택한 뒤에는 자동 감지가 그 선택을 덮어쓰지 않는다.
- 사용자가 다시 `자동`을 선택하면 현재 실행 중인 GPU를 즉시 다시 판정하고 `자동(낮음)` 또는 `자동(최고)`를 적용한다.
- 정확히 `4GiB`인 GPU는 `최고`로 처리한다.
- 자동 감지에 하드웨어 성능 벤치마크는 사용하지 않는다. VRAM이 4GiB 이상이지만 연산 성능이 낮은 GPU도 정책상 `최고`가 될 수 있으며, 사용자가 중간 단계로 내릴 수 있게 한다.
- 내부 품질 단계는 Unreal의 `0=Low`, `1=Medium`, `2=High`, `3=Epic`을 사용한다. `Cinematic(4)`은 메뉴에 노출하지 않고 사용자 표시명 `최고`는 `Epic(3)`을 뜻한다.

## 2. 현재 구조와 변경 이유

- `DefaultGameUserSettings.ini`는 모든 Scalability 그룹을 현재 `3`으로 시작한다. 저사양 첫 실행에서도 자동 판정 전에 최고 품질이 기본값이 될 수 있다.
- `UTunaSweeperGameInstance::Init()`은 `UGameUserSettings`를 읽지만 현재는 지원하지 않는 전체화면 해상도만 보정한다.
- `ATunaSweeperPlayerController::ApplyInitialTitleDisplaySettings()`가 `IntroMap`의 `BeginPlay()`에서 설정을 다시 읽고 즉시 적용한다.
- 화면 모드와 해상도는 `UGameUserSettings`에 저장하지만 DLSS 선호값은 `TunaSweeper.GraphicsSettings/DLSSMode`라는 별도 config 키로 관리한다.
- 그래픽 설정 위젯은 고정 높이 `780px`의 `VerticalBox`이며 화면 모드, 해상도, DLSS만 들어 있다. 개별 품질 항목을 추가하려면 스크롤 가능한 구조가 필요하다.
- 기존 `WBP_IntroMenu`의 큰 계층은 에디터 원샷으로 관리한다. 그래픽 본문은 독립 C++ 위젯으로 분리하고 기존 `GraphicsSettingsPanel`에 런타임으로 한 번만 추가해, 열린 에디터의 WBP를 강제로 재생성하지 않아도 패키징 빌드와 에디터에서 같은 UI를 사용하게 한다.

## 3. 설정 소유권

### 3.1 전용 GameUserSettings 클래스

`UGameUserSettings`를 상속한 `UTunaSweeperGameUserSettings`를 추가하고 다음 책임을 한곳에 모은다.

- 그래픽 프리셋 선택과 개별 Scalability 값
- VRAM 자동 판정
- 화면 모드, 해상도, VSync, 최대 FPS, 동적 해상도
- DLSS 선호값과 지원 여부 fallback
- 모션 블러 및 하드웨어 레이 트레이싱 같은 추가 옵션
- 적용, 취소용 스냅샷, 저장과 구버전 설정 마이그레이션

기본 `UGameUserSettings`가 이미 저장하는 화면·Scalability 값은 중복 필드로 만들지 않는다. 다음과 같이 TunaSweeper 고유 의미가 필요한 값만 `Config` 속성으로 추가한다.

- `GraphicsSettingsSchemaVersion`
- `SelectedGraphicsPreset`: `Auto`, `Low`, `Medium`, `High`, `Epic`, `Custom`
- `PreferredDLSSMode`
- `bMotionBlurEnabled`
- `bHardwareRayTracingEnabled`
- 선택적으로 진단용 `LastDetectedDedicatedVideoMemoryMB`와 `LastAutoResolvedPreset`

`DefaultEngine.ini`의 `[/Script/Engine.Engine]`에 `GameUserSettingsClassName=/Script/TunaSweeper.TunaSweeperGameUserSettings`를 지정한다.

### 3.2 기존 설정 마이그레이션

새 스키마 키가 없는 실행 환경을 다음처럼 구분한다.

1. 기존 사용자 `GameUserSettings.ini`에 Scalability 값이 있으면 이를 보존한다.
2. 현재 값이 명명된 프리셋과 정확히 일치하면 해당 프리셋으로, 아니면 `Custom`으로 기록한다.
3. 기존 `TunaSweeper.GraphicsSettings/DLSSMode`가 있으면 새 클래스의 `PreferredDLSSMode`로 한 번만 옮긴다.
4. 사용자 설정 파일이 없는 새 설치만 기본 `Auto`로 시작한다.
5. 마이그레이션이 끝난 뒤 스키마 버전을 저장해 다음 실행부터 반복하지 않는다.

이렇게 해야 기존 사용자의 해상도·DLSS·품질 선택을 자동 감지가 갑자기 덮어쓰지 않는다.

## 4. 자동 VRAM 판정

UE 5.7 설치 소스 기준으로 `FRHIGlobals::FGpuInfo`에는 전용 VRAM 필드가 없으므로 5.8 방식에 의존하지 않는다. `RHIGetTextureMemoryStats(FTextureMemoryStats&)`의 `DedicatedVideoMemory`를 사용한다. 프로젝트는 이미 `RHI`를 private dependency로 가지고 있어 새 모듈 의존성은 필요하지 않다.

판정 함수는 순수 로직과 실제 RHI 조회를 분리한다.

```text
ResolveAutoPreset(DedicatedVideoMemoryBytes):
    값이 0 이하 또는 알 수 없음 → Low
    값이 4 * 1024^3 미만       → Low
    그 외                      → Epic
```

적용 순서는 다음과 같다.

1. 엔진 기본 사용자 설정은 보수적인 낮음 프로필로 시작한다.
2. `UTunaSweeperGameInstance::Init()`에서 사용자 설정을 로드한다.
3. 선택 모드가 `Auto`일 때만 RHI의 전용 VRAM을 조회한다.
4. 판정된 `Low` 또는 `Epic` 프로필을 적용하고 저장한다.
5. 이후 `IntroMenuWidget`을 생성한다.
6. 사용자가 게임을 시작해 `BunkerMap` 또는 `RaidMap`을 열기 전에 이미 해당 텍스처 정책이 활성화되어 있어야 한다.

개발·자동화 검증을 위해 Shipping이 아닌 빌드에만 `-TunaGraphicsTestVRAMMB=<값>` 실행 인자를 제공한다. 실제 GPU를 바꾸지 않고 `3072`, `4095`, `4096`, `8192` 경계를 검증하는 용도이며 Shipping에서는 무시한다.

## 5. 프리셋 정의

해상도, 화면 모드, DLSS 모드, VSync, 최대 FPS와 모션 블러는 개인 선호 성격이 강하므로 전체 품질 프리셋이 덮어쓰지 않는다. 프리셋은 장면 품질과 부하를 결정하는 Scalability 그룹만 변경한다.

| 항목 | 낮음 | 중간 | 높음 | 최고 |
|---|---:|---:|---:|---:|
| 텍스처 | 0 | 1 | 2 | 3 |
| 그림자 | 1 | 1 | 2 | 3 |
| 글로벌 일루미네이션 | 1 | 1 | 2 | 3 |
| 반사 | 0 | 1 | 2 | 3 |
| 시야 거리 | 1 | 1 | 2 | 3 |
| 이펙트 | 1 | 1 | 2 | 3 |
| 포스트 프로세싱 | 0 | 1 | 2 | 3 |
| 식생 | 0 | 1 | 2 | 3 |
| 셰이딩·재질 | 1 | 1 | 2 | 3 |
| 지형 | 1 | 1 | 2 | 3 |
| 안티앨리어싱 | 1 | 2 | 3 | 3 |

`낮음`이 모든 값을 0으로 만들지 않는 이유는 현재 프로젝트가 정적 조명을 끄고 Lumen·Virtual Shadow Maps 기반으로 구성되어 있으며, 그림자와 전투 이펙트를 완전히 제거하면 전투 가독성과 장면 인상이 훼손될 수 있기 때문이다. 각 프리셋은 위 표의 전체 조합이 정확히 일치할 때만 해당 이름으로 표시하고, 한 항목이라도 다르면 `사용자 지정`으로 표시한다.

## 6. 텍스처 VRAM 정책

프로젝트에 `DefaultScalability.ini`를 추가하고 UE 5.7 엔진 기본값을 명시적인 프로젝트 기준점으로 고정한다.

| 텍스처 단계 | 풀 크기 | Mip Bias | VRAM 제한 |
|---|---:|---:|---|
| 낮음 | 400MB | 16 | 사용 |
| 중간 | 600MB | 1 | 사용 |
| 높음 | 800MB | 0 | 사용 |
| 최고 | 1000MB | 0 | 사용 |

세부 원칙은 다음과 같다.

- `DefaultScalability.ini`에는 `ECVF_Scalability`로 허용되는 CVar만 둔다. UE 5.7에서 Scalability 파일 사용이 허용되지 않는 `r.Streaming.UsePerTextureBias`는 전용 `GameUserSettings`의 적용 단계에서 `SetByGameSetting`으로 켠다.
- 모든 단계에서 `r.Streaming.LimitPoolSizeToVRAM=1`을 사용한다. UE 5.7 기본값은 Epic에서 이 값이 꺼져 있으므로 프로젝트에서 명시적으로 켠다.
- `r.TextureStreaming`은 항상 켜 두며 사용자 토글로 노출하지 않는다.
- 첫 구현은 UE 5.7의 400/600/800/1000MB 기준으로 시작하고, TunaSweeper 패키지 실측 뒤에만 숫자를 조정한다.
- `UI`, `Never Stream`, mipmap이 없는 텍스처는 풀 제한만으로 충분히 줄지 않을 수 있으므로 별도 자산 감사를 한다.
- 현재 타이틀은 전체화면 2D 배경 대신 3D 타이틀 룸을 사용하므로 우선 일반 스트리밍 경로를 검증한다. 큰 UI·난이도 삽화가 비스트리밍 메모리를 과도하게 사용한다고 측정될 때만 고·저해상도 소프트 참조 변형을 추가한다.

## 7. 그래픽 설정 UI

### 7.1 위젯 분리

거대해진 `UTunaSweeperIntroMenuWidget`에 그래픽 버튼과 핸들러를 계속 추가하지 않는다.

- 새 C++ 위젯 `UTunaSweeperGraphicsSettingsWidget`을 추가한다.
- `UTunaSweeperGraphicsSettingsWidget`과 재사용 가능한 `UTunaSweeperGraphicsQualityRowWidget`이 자체 UMG 트리를 한 번 구성한다.
- `WBP_IntroMenu`의 기존 `GraphicsSettingsPanel`에는 런타임에 새 위젯 하나만 추가하고 기존 화면·해상도·DLSS 섹션은 숨긴다.
- 화면 모드·해상도·DLSS 처리도 새 위젯으로 이동한다.
- 부모 인트로 위젯은 탭 전환, 뒤로가기, 전체 설정 창 표시만 담당한다.

그래픽 본문은 생성 후 다시 만들지 않으며, 부모 인트로 WBP를 재생성하지 않고도 독립적으로 확장할 수 있다.

### 7.2 화면 배치

설정 창의 제목·현재 상태·탭은 고정하고 그래픽 본문만 `ScrollBox`로 만든다. 적용·취소 버튼도 스크롤 밖 하단에 고정한다.

그래픽 본문 순서는 다음과 같다.

1. **품질 프리셋**: 자동, 낮음, 중간, 높음, 최고
2. **감지 상태**: `전용 VRAM 3.0GB · 자동(낮음)` 같은 읽기 전용 문구
3. **화면**: 화면 모드, 해상도, VSync, 최대 FPS
4. **업스케일링**: DLSS 끄기·품질·균형·성능, DLSS 비지원 시 TSR 안내
5. **개별 품질**: 텍스처, 그림자, 글로벌 일루미네이션, 반사, 시야 거리, 이펙트, 포스트 프로세싱, 식생, 셰이딩·재질, 지형, 안티앨리어싱
6. **고급**: 모션 블러, 동적 해상도, 하드웨어 레이 트레이싱

개별 품질 행은 `◀  현재 단계  ▶` 형태로 만들어 11개 항목마다 네 개의 버튼을 늘어놓지 않는다. 키보드·게임패드 포커스 이동과 1280x720 화면에서도 사용할 수 있는 높이를 기준으로 한다.

### 7.3 적용과 취소

- 그래픽 탭을 열 때 현재 설정의 스냅샷을 `PendingSettings`에 복사한다.
- 프리셋과 개별 행 조작은 우선 pending 값만 변경한다.
- 개별 항목을 수정하면 preset은 즉시 `Custom`이 된다.
- `적용`을 누르면 Scalability 그룹을 한 번에 적용한 뒤 모션 블러·RT 같은 개별 override, DLSS, 화면 설정 순으로 적용하고 저장한다.
- `취소` 또는 변경 상태에서 뒤로가기는 스냅샷으로 복구한다.
- 해상도·전체화면 변경은 적용 뒤 15초 확인 창을 표시하고 확인하지 않으면 이전 값으로 되돌린다.
- DLSS가 활성화된 동안 안티앨리어싱 행은 `DLSS에서 자동 관리`로 비활성 표시한다.
- 지원하지 않는 DLSS·하드웨어 RT 버튼은 비활성화하고 이유를 상태 문구로 표시한다.

## 8. 런타임 적용 순서

한 번의 적용에서 서로 덮어쓰는 CVar가 생기지 않도록 순서를 고정한다.

1. pending Scalability 품질을 `UGameUserSettings`에 기록한다.
2. `ApplyNonResolutionSettings()`로 표준 그룹을 적용한다.
3. 모션 블러와 하드웨어 RT 같은 사용자 override를 다시 적용한다.
4. DLSS 지원 여부를 확인하고 선호 모드를 적용한다. 비지원이면 DLSS를 끄고 TSR 경로를 사용한다.
5. 화면 모드와 해상도를 적용한다.
6. 확인된 결과만 `SaveSettings()`로 저장한다.
7. UI 상태와 선택 스타일을 새 값으로 갱신한다.

`ATunaSweeperPlayerController::ApplyInitialTitleDisplaySettings()`의 중복 `LoadSettings()` 호출은 제거하거나 `UTunaSweeperGameUserSettings`의 초기화 완료 상태만 확인하게 바꿔, `GameInstance`에서 적용한 자동 결과가 뒤에서 다시 덮이지 않게 한다.

## 9. 파일별 작업 계획

### 새 파일

- `TunaSweeper/Source/TunaSweeper/Public/Settings/TunaSweeperGameUserSettings.h`
- `TunaSweeper/Source/TunaSweeper/Private/Settings/TunaSweeperGameUserSettings.cpp`
- `TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperGraphicsSettingsWidget.h`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGraphicsSettingsWidget.cpp`
- `TunaSweeper/Source/TunaSweeper/Public/UI/TunaSweeperGraphicsQualityRowWidget.h`
- `TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperGraphicsQualityRowWidget.cpp`
- `TunaSweeper/Source/TunaSweeper/Public/Settings/TunaSweeperGraphicsSettingsTypes.h`
- `TunaSweeper/Source/TunaSweeper/Private/Tests/TunaSweeperGraphicsSettingsTests.cpp`
- `TunaSweeper/Config/DefaultScalability.ini`

### 수정 파일

- `TunaSweeper/Config/DefaultEngine.ini`: 전용 GameUserSettings 클래스 등록
- `TunaSweeper/Config/DefaultGameUserSettings.ini`: 첫 실행용 보수적 기본값과 새 설정 기본값
- `TunaSweeperGameInstance.cpp`: 타이틀 위젯보다 이른 초기화와 자동 판정
- `TunaSweeperPlayerController.cpp`: 중복 설정 로드·적용 경로 정리
- `TunaSweeperIntroMenuWidget.h/.cpp/.Settings.cpp/.Refresh.cpp/.Actions.cpp`: 그래픽 세부 로직을 새 자식 위젯으로 이동하고 부모 역할 축소
- `TunaSweeper/Content/Data/UITextStrings.csv`: 한국어·영어·일본어 설정명, 단계명, 감지 상태, 적용·취소·확인 문구 추가

## 10. 테스트 계획

### 10.1 자동화 테스트

- 전용 VRAM `0`, 알 수 없음, `3072MB`, `4095MB`는 낮음
- `4096MB`, `4097MB`, `8192MB`는 최고
- Auto는 Low/Epic만 반환하고 Medium/High를 반환하지 않음
- 수동 Low/Medium/High/Epic/Custom은 다음 실행에서 자동 판정으로 덮이지 않음
- 프리셋 전체 조합이 일치하면 올바른 이름, 한 값이라도 다르면 Custom
- 기존 DLSS config와 기존 Scalability 값 마이그레이션 보존
- DLSS·RT 미지원 환경 fallback
- Apply 후 저장, Cancel 후 원래 값 복구

### 10.2 에디터·UI 검증

- 그래픽 탭을 반복해서 열어도 런타임 위젯이 중복 생성되지 않음
- 1280x720, 1920x1080, 3840x2160에서 스크롤·글자 잘림·포커스 순서 확인
- 한국어·영어·일본어에서 모든 상태 문구가 정상 표시
- 자동은 `자동(낮음)` 또는 `자동(최고)`로 표시되고 수동 단계와 구분
- 해상도 확인 타이머와 자동 복구 확인

### 10.3 런타임·메모리 검증

- Development 패키지에서 테스트 VRAM 실행 인자로 4GiB 경계 확인
- 로그에서 GameInstance 판정·적용이 IntroMenu 생성 및 Bunker/Raid `OpenLevel`보다 먼저 기록되는지 확인
- `stat streaming`, `stat RHI`, `memreport -full`로 낮음과 최고의 Texture Pool, Streaming Memory, NonStreaming Memory 비교
- 낮음에서 `Texture Pool Over Budget`이 지속되지 않는지 확인
- 게임 시작 직후와 Bunker/Raid 진입 직후의 VRAM 피크 비교
- UI·Never Stream 텍스처가 저품질에서 비정상적으로 큰 경우에만 자산별 해결 추가
- 전투 경고, 투사체, 폭발, 추출 연기 등 게임플레이 정보를 전달하는 이펙트가 낮음에서도 사라지지 않는지 확인

### 10.4 빌드 완료 절차

1. 관련 자동화 테스트 실행
2. `TunaSweeperEditor Win64 Development` 빌드
3. UE 5.7 에디터로 `TunaSweeper.uproject` 실행
4. 원샷 WBP 생성·컴파일·저장 확인
5. PIE에서 설정 UI와 초기 자동 판정 확인
6. Development 패키지에서 메모리 계측
7. 문서와 요청 로그 갱신

## 11. 단계별 구현 순서

1. 전용 `GameUserSettings`와 프리셋 매핑, VRAM 판정 순수 함수 및 자동화 테스트를 먼저 만든다.
2. `DefaultGameUserSettings.ini`를 안전한 첫 실행값으로 바꾸고 GameInstance 초기 적용을 연결한다.
3. 기존 DLSS 저장값을 새 설정 클래스로 마이그레이션하고 PlayerController의 중복 적용을 정리한다.
4. 새 그래픽 자식 위젯과 스크롤 UI를 만들고 기존 화면·해상도·DLSS 기능을 이동한다.
5. 프리셋, 개별 Scalability, VSync·FPS·모션 블러·동적 해상도·RT 항목을 연결한다.
6. 적용·취소·해상도 확인 흐름과 다국어 문구를 완성한다.
7. 게임·에디터 타깃 빌드와 헤드리스 자동화 테스트를 통과시키고 에디터를 실행한다.
8. 패키지 메모리 계측 결과에 따라 Texture Pool 수치만 조정한다.

## 12. 완료 기준

- 신규 설치에서 4GiB 미만 또는 VRAM 미확인 환경은 자동 낮음, 4GiB 이상은 자동 최고로 시작한다.
- 중간·높음은 자동으로 선택되지 않으며 사용자가 선택한 뒤 유지된다.
- 게임 맵 로드 전에 텍스처 품질과 풀 제한이 적용된다.
- 모든 개별 품질 항목이 저장·복원되고 프리셋/사용자 지정 표시가 정확하다.
- 기존 해상도와 DLSS 사용자 설정이 마이그레이션에서 유실되지 않는다.
- 낮음에서도 필수 조명과 전투 정보 이펙트가 유지된다.
- 에디터 빌드와 자동화 테스트를 통과하고, 패키지 메모리 검증 결과를 기준으로 풀 수치를 후속 조정할 수 있다.
