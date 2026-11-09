# Environment Weather Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking. 현재 요청은 계획 작성이다. 아래 체크박스는 구현 완료를 의미하지 않는다. 명시적인 커밋 요청이 없으면 커밋하지 않는다.

**Goal:** UE 5.7에서 재사용 가능한 시각·음향 날씨 플러그인과 새로 제작한 바람·비·돌풍 음원을 완성하고, 이후 AI·플레이어 영향을 연결할 수 있는 조회/API 경계를 제공한다.

**Architecture:** 월드별 C++ 상태 코어와 DataAsset 프리셋을 하늘·강수·재질·음향 컴포넌트가 소비한다. 플러그인 콘텐츠는 프로젝트 콘텐츠를 역참조하지 않으며 기존 나무·연기·지역안개 연결은 TunaSweeper 모듈이 담당한다. 음원은 오프라인 합성 후 WAV/SoundWave로 저장하며 런타임 합성기를 필수로 두지 않는다.

**Tech Stack:** UE 5.7 C++, DataAsset, UTickableWorldSubsystem, Niagara, Material Parameter Collection, SoundWave/UAudioComponent, Unreal Automation, 오프라인 Python/FFmpeg 음원 제작·검사.

**Spec:** [재사용 환경·날씨 플러그인 설계](../specs/2026-11-09-environment-weather-design.md)

## Global Constraints

- UE 5.7만 대상으로 한다. `EngineAssociation=5.7`, `EngineIncludeOrderVersion.Unreal5_7`을 유지한다.
- 명시적인 별도 커밋 요청 전까지 Git 커밋·푸시를 하지 않는다. 이전 자동 커밋 규칙보다 최신 금지 지시를 우선한다.
- 다른 대화의 변경, 접근 제한 ProductionPayload, 퀘스트 데이터는 건드리지 않는다.
- UE 확인·실행·종료·빌드 전에 `Docs/unreal_editor_coordination.md`를 따른다. 공유 에디터를 임의 종료/재실행하지 않는다.
- 모든 사용자 노출 문구는 문자열 키로 조회한다. 플러그인 자체 CSV/StringTable을 제공하고 코드에 표시 문구/대체 문구를 하드코딩하지 않는다. 한국어 플레이어 문구는 `기지`를 사용한다.
- 1차는 기존 Details 시스템을 확장하며 신규 UMG 위젯을 만들지 않는다. 위젯이 추가되는 별도 범위에서는 실제 모델명·reasoning을 확인하고 최신 위젯 폴더/이름/WBP 트리 규칙을 적용한다.
- 월드 단위는 cm, 속도는 cm/s, +Z는 위다. 기본 북쪽은 +X이며 플러그인 북쪽 축은 설정으로 교체 가능하다. 풍향은 바람이 이동하는 방향으로 정의한다.
- 한 게임 월드에는 활성 Rig 하나만 허용한다. 여러 PIE 월드의 상태·MPC 인스턴스·오디오를 공유하지 않는다.
- 일회성 애셋 생성 코드는 최종 검증 전에 진입점·의존성과 함께 제거한다. 저장된 애셋을 시작할 때 재생성하지 않는다.

## Review Focus

- 전환 도중 재요청·override 종료·180도 풍향에서 표시값이 튀거나 자동 타이머가 두 번 진행하지 않아야 한다. Task 1/2의 interruption/override 테스트로 고정한다.
- 실내의 카메라가 창밖을 볼 때 외부 비는 남고, 높은 속도의 입자가 얇은 차폐를 통과하지 않아야 한다. Task 4의 GPU 시나리오와 segment 검사로 고정한다.
- 맵 이동·두 PIE 월드·pause/resume에서 음원 중복·재시작·상태 혼선이 없어야 한다. Task 2/6의 수명 테스트로 고정한다.
- 누락 자산·지원하지 않는 구름 재질·잘못된 snapshot이 기존 월드나 다른 표현 채널을 망가뜨리지 않아야 한다. Task 2/3/6/8의 fail-closed 테스트로 고정한다.
- 편집기에서 보이던 플러그인 자산이 cook/다른 프로젝트에서 누락되거나 게임 역참조를 요구하지 않아야 한다. Task 9/10의 dependency/cook/portable 검사로 고정한다.

## 작업 단위와 의존 순서

`1 → 2 → 3/4/6 → 7/8 → 9 → 10 → 11`. Task 5 음원 제작은 Task 1과 병렬로 진행할 수 있으며 Task 6의 실제 재생 검증 전에 완료한다. Task 3과 4는 Task 2의 타입/API를 확정한 다음 서로 다른 파일을 맡아 병렬화할 수 있다. 최종 통합은 한 실행자가 담당한다.

현재 존재하는 파일은 수정 대상으로, 새 경로와 명령은 앞으로 만들 대상으로 아래에 명시한다. 이번 계획 작성 단계에서는 생성기·컴파일·자동화·음원 생성을 실행하지 않는다.

## 파일 배치

`P = TunaSweeper/Plugins/EnvironmentWeather`, `R = P/Source/EnvironmentWeather`, `E = P/Source/EnvironmentWeatherEditor`, `G = TunaSweeper/Source/TunaSweeper`, `T = Tools/EnvironmentWeather`. 아래 별칭은 이 표의 정확한 프로젝트 상대 경로를 의미한다.

| 경로 | 책임 |
|---|---|
| `P/EnvironmentWeather.uplugin`, `R/EnvironmentWeather.Build.cs`, `E/EnvironmentWeatherEditor.Build.cs` | 콘텐츠 포함 Runtime/Editor 모듈과 엔진 의존성 |
| `R/Public/EnvironmentWeatherTypes.h`, `EnvironmentWeatherProfile.h`, `EnvironmentWeatherConfiguration.h` | 상태·전환·snapshot·설정의 공개 타입 |
| `R/Private/EnvironmentWeatherEvaluator.h/.cpp` | 보간·풍향·젖음의 UObject 독립 계산 |
| `R/Public/EnvironmentWeatherSubsystem.h`, `R/Private/EnvironmentWeatherSubsystem.cpp` | 등록, 스케줄, override, 조회·snapshot |
| `R/Public/EnvironmentWeatherRig.h`, `R/Private/EnvironmentWeatherRig.cpp` | 월드 소유권·명시적 참조·채널 구성 |
| `R/Public/Components/WeatherSkyComponent.h`, `WeatherPrecipitationComponent.h`, `WeatherAudioComponent.h` 및 `R/Private/Components/*.cpp` | 각각 하늘/재질, 강수, 오디오 출력 |
| `R/Public/EnvironmentWeatherShelterVolume.h`, `R/Private/EnvironmentWeatherShelterVolume.cpp` | 작성된 box 차폐와 위치 노출도 |
| `R/Public/EnvironmentWeatherPresentation.h`, `EnvironmentWeatherAudioProfile.h` | 출력별 곡선·자산·품질·믹스 데이터 |
| `R/Private/Tests/EnvironmentWeather{Core,Lifecycle,Shelter,Audio,Asset}Tests.cpp` | 기능·수명·경계·자산 검사 |
| `E/Private/EnvironmentWeatherEditorModule.cpp`, `EnvironmentWeatherDetails.cpp`, `EnvironmentWeatherPreview.cpp` | 문자열 테이블, Details·미리보기·검증 |
| `E/Private/Tests/EnvironmentWeatherPreviewTests.cpp` | 미리보기 복원·Undo·PIE 분리 |
| `P/Content/{Profiles,Materials,FX,Audio,Maps,Localization}` | 저장된 런타임·샘플 콘텐츠 |
| `P/Resources/SourceAudio/*.wav`, `manifest.json`, `recipe.json`, `README.md` | 신규 원본과 제작 출처·재제작 조건 |
| `G/Public/Environment/TunaSweeperWeatherBindingComponent.h`, `G/Private/Environment/TunaSweeperWeatherBindingComponent.cpp` | 게임/기존 플러그인 선택적 연동 |
| `G/Private/Tests/TunaSweeperWeatherBindingTests.cpp` | 경량 갱신·복원 회귀 검증 |
| `T/{run_tests.ps1,validate_audio.py,validate_portability.ps1,README.md}` | 남겨둘 검증 도구. 애셋 생성기는 포함하지 않는다. |

## 실행·검증 명령 계약

실행 시 작업 트리와 에디터 조율 상태를 먼저 다시 확인한다. 엔진 경로는 `UE_5_7_ROOT`가 있으면 그것을 쓰고 없으면 `C:/Program Files/Epic Games/UE_5.7`을 사용한다. 다른 버전이면 중단한다. WindowsApps의 `python.exe` 발견만으로 Python 작동을 가정하지 않고 `py -3 --version`과 필요한 라이브러리 import를 확인한다. 추가 설치가 필요하면 실제 실행 환경에 맞춰 별도로 처리한다.

Task 1에서 만드는 `T/run_tests.ps1 -Suite <Core|Lifecycle|Sky|Shelter|Audio|Assets|Preview|Integration|All> -Mode <Logic|Render|Audio>`는 아래 Unreal 자동화를 감싸고 해당 suite의 필수 테스트 이름이 모두 보고서에 있는지 검증한다. 실패 0뿐 아니라 누락 0, timeout 없음, 프로세스 종료 코드 0을 요구한다. 매 실행 고유 로그/보고서 디렉터리를 사용한다.

```powershell
$weatherRepo = 'D:/github/extraction_shooter'
$weatherProject = Join-Path $weatherRepo 'TunaSweeper/TunaSweeper.uproject'
$weatherEngine = if ($env:UE_5_7_ROOT) { $env:UE_5_7_ROOT } else { 'C:/Program Files/Epic Games/UE_5.7' }
$weatherBuildLog = Join-Path $weatherRepo ('TunaSweeper/Saved/Logs/Build/Weather-' + [guid]::NewGuid().ToString('N') + '.log')
& "$weatherEngine/Engine/Build/BatchFiles/Build.bat" TunaSweeperEditor Win64 Development "-Project=$weatherProject" -WaitMutex "-Log=$weatherBuildLog"
# 종료 코드0을 확인한 뒤 작업 대상 suite만 실행한다.
& "$weatherRepo/Tools/EnvironmentWeather/run_tests.ps1" -Suite Core -Mode Logic
```

주석을 포함한 문서 언어는 한국어를 기본으로 유지한다. 위 빌드는 에디터를 자동으로 열지 않는 직접 Build.bat 경로다. 조율상 닫힌 상태를 유지해야 할 때 자동 실행 wrapper를 사용하지 않는다. 전체 작업 완료 후 재실행은 조율 문서대로 한다.

Logic 모드만 `-nullrhi -nosound`를 허용한다. Render/Audio는 각각 GPU/실제 오디오 믹서를 활성화한 실행이어야 하며 성공 증거를 파일로 남긴다. 최종 cook smoke는 스토어 배포가 아닌 독립 테스트 호스트를 대상으로 한다. Windows `RunUAT BuildCookRun` 호출은 AppData 쓰기를 위해 처음부터 escalation을 요청한다. 향후 실제 스토어 패키징으로 범위가 바뀔 때만 스토어 체크리스트를 읽는다.

---

### Task 1: 데이터 계약과 프리셋 평가기

**Files:** Create plugin descriptor, 두 Build.cs와 모듈 cpp, Types/Profile/Configuration/Presentation/AudioProfile 헤더 및 필요한 cpp, evaluator, `CoreTests.cpp`, `T/run_tests.ps1`. Modify `TunaSweeper/TunaSweeper.uproject`에 플러그인 enable만 추가한다.

**Interfaces:**
- Produces `FEnvironmentWeatherState`, `FEnvironmentWeatherTransition`, `FEnvironmentWeatherSample`, `FWeatherExposure`, `FEnvironmentWeatherSnapshot`과 DataAsset 계약. Snapshot config ID는 `FName ConfigurationId`이며 configuration 내 profile ID는 유일하다. FWeatherExposure는 이진 PrecipitationExposure01과 연속 AudioExposure01이며 Sample은 State와 이 두 값을 포함한다.
- Produces `bool ValidateState(const FEnvironmentWeatherState&, FName& OutErrorKey)`, `FEnvironmentWeatherState EvaluateTransition(const FEnvironmentWeatherTransition&, float ElapsedSeconds)`, `float StepWetness(float Wetness01,float RainIntensity01,float DeltaSeconds)` in `EnvironmentWeather` namespace.
- `UEnvironmentWeatherPresentation`은 cloud/fog/lighting mapping curve와 Niagara/MPC refs, `UEnvironmentWeatherAudioProfile`은 3 wind/3 rain/4 gust soft refs와 gain/filter/volume 기본값을 가진다.

- [ ] `EnvironmentWeather.Core.RangeValidation`에 NaN/Inf/음수 풍속 거절, intensity 범위와 유효 방향 검사를 작성하고 실패를 확인한다.
- [ ] `TransitionEndpoints`, `InterruptedTransitionContinuity`, `WindShortestArc`, `WetnessAccumulation` 테스트를 작성한다. 0초=Target, 15초=Target, 350→10도 midpoint=0도, 0→180도는 +yaw, 강수1의 90초=Wetness1, 무강수180초=0, 30/60fps 결과 차이 <=1e-4를 검증한다.
- [ ] spec 5절의 값·지연·계산식을 구현한다. 평가기는 생성/전환 시 검증된 입력만 받으며 매 프레임 자산 조회를 하지 않는다.
- [ ] 모듈 의존성을 최소화하고 `run_tests.ps1`의 Core 필수 테스트 목록과 보고서 검사를 구현한다.
- [ ] Editor 빌드 후 `-Suite Core -Mode Logic`에서 모든 명명된 테스트 통과를 확인한다. 공개 헤더가 게임 타입을 포함하지 않는지 점검한다.

**산출물:** 엔진 렌더러 없이 검증 가능한 상태 계산과 다른 작업이 사용할 확정 타입.

### Task 2: 월드 수명, 스케줄, override, snapshot

**Files:** Create Subsystem/Rig 헤더·cpp, `LifecycleTests.cpp`; modify Types/Configuration의 스케줄·토큰 필드.

**Interfaces:**
- `bool RegisterRig(AEnvironmentWeatherRig* Rig, UEnvironmentWeatherConfiguration* Config, FName& OutErrorKey)` / `void UnregisterRig(AEnvironmentWeatherRig* Rig)`.
- `FEnvironmentWeatherState GetCurrentState() const`; `FEnvironmentWeatherSample GetStateAtLocation(FVector Location) const` (Task 4 전에는 두 Exposure 모두1). State의 `FVector2D GetWindVelocityCmPerSecond() const`는 spec 돌풍 보정·상한을 적용한다.
- `bool SetWeatherProfile(FName ProfileId, float TransitionSeconds)`; `int64 PushWeatherOverride(UObject* Owner,FName ProfileId,int32 Priority,float TransitionSeconds)` (실패=0); `bool PopWeatherOverride(int64 Token,float ReturnSeconds)`.
- `bool CaptureSnapshot(FEnvironmentWeatherSnapshot& Out,FName& ErrorKey) const`; `bool RestoreSnapshot(const FEnvironmentWeatherSnapshot& In,FName& ErrorKey)`.
- `OnWeatherStateChanged(const FEnvironmentWeatherState&)` multicast, 최대 10Hz. `OnGustStarted(int32 Serial,float Strength,float DurationSeconds)` 사건은 시작 시1회이며10Hz 제한과 별개다. `SetAutoScheduleEnabled(bool)`과 configuration의 명시적인 seed를 사용한다. schedule/gust stream은 각각 seed와 seed XOR 0x5EED로 초기화한다.

- [ ] `SingleRigPerWorld`, `PIEWorldIsolation`, `SeededSchedule`, `PauseFreezesTime`, `OverridePriorityAndOwnerLifetime`, `SnapshotRoundTripAndReject` 실패 테스트를 작성한다.
- [ ] 같은 seed의20회 자동 선택이 동일하고 transition graph를 지키는지 검증한다. override 중 타이머 정지, 동률 최신 승리, owner 제거, 중간 수동 요청의 base만 갱신, 마지막 해제 후 갱신된 base 목표 복귀와 타이머 재개를 검증한다.
- [ ] Game/PIE만 생성하는 subsystem과 opt-in Rig 등록을 구현한다. 기본값은 auto off, map enter 기본 preset, 월드별 clock/MPC/등록목록이다. 잘못된 config와 중복 Rig는 상태를 덮어쓰지 않는다.
- [ ] Snapshot round trip, version/config/profile 누락 시 원자적 실패, transient override 중 capture/restore 모두 실패를 구현한다. `GustSnapshotContinuity`에서 돌풍 중간 복원 후 동일한 상태/다음 사건을 얻되 복원 즉시 사건은 재발행하지 않는지 검증한다. 저장 슬롯이나 GameInstanceSave 필드는 추가하지 않는다.
- [ ] `-Suite Lifecycle -Mode Logic` 통과 후 게임 시간 배율·pause·Rig 파괴·두 월드에서 component/delegate 누수를 검사한다.

**산출물:** 수동/자동 전환 및 향후 게임 연동 API. 시각·음향 없이도 동작한다.

### Task 3: 하늘·안개·재질 출력

**Files:** Create WeatherSkyComponent, `R/Private/Tests/EnvironmentWeatherSkyTests.cpp`; create `P/Content/Materials/{MPC_EnvironmentWeather,MF_WeatherWetness,M_WeatherWetnessSample,M_WeatherCloud,MI_WeatherCloud}.uasset`; modify Rig/Presentation.

**Interfaces:** `bool BindTargets(const FWeatherSkyBindings& Targets,FName& ErrorKey)`; `void ApplyState(const FEnvironmentWeatherState&)`; `void RestoreTargets()`. `FWeatherSkyBindings`는 DirectionalLight/SkyLight/ExponentialHeightFog/VolumetricCloud의 명시적 weak refs. 출력 mapping curves는 Presentation에서 소비한다.

- [ ] `BaselineDoesNotCompound`, `RestoreOwnedProperties`, `MissingBindingIsChannelLocal`, `CloudMaterialContract` 테스트를 작성한다. 두 번 적용해 조명 강도가 제곱되지 않는 것과 태양 회전 불변을 검증한다.
- [ ] baseline 캡처·속성별 소유권·변경량이 있을 때만 갱신하는 로직을 구현한다. 동일 target을 가진 두 Rig는 등록 검증에서 거절한다. 동기 SkyLight recapture를 매 프레임 하지 않는다.
- [ ] MPC 스칼라 `RainIntensity`, `Wetness`, `WindSpeed`, `Gust`와 vector `WindDirection`을 만든다. 이름 계약을 자산 검사에서 검증한다. `MF_WeatherWetness(Wetness,SurfaceExposure)`로 지원하는 샘플 표면만 젖게 한다.
- [ ] plugin cloud material의 `WeatherCloudCover`, `WeatherCloudTint` 파라미터를 저장한다. custom material에서 필수 파라미터가 없으면 그 구름 채널을 건너뛰고 문자열 키 진단을 반환한다.
- [ ] `-Suite Sky -Mode Logic` 후 `-Suite Sky -Mode Render`로 clear/overcast/fog 및 wet/dry 샘플을 비교한다. 색/밀도/조명 범위는 튜닝 데이터에 기록한다.

**산출물:** 기존 환경 액터를 안전하게 제어하고 해제하면 복원되는 출력부.

### Task 4: 강수와 실내 차폐

**Files:** Create ShelterVolume, WeatherPrecipitationComponent, `ShelterTests.cpp`; create `P/Content/FX/{NS_WeatherRain,NEType_Weather}.uasset`, `P/Content/Materials/M_WeatherRain.uasset`; modify Subsystem의 shelter registry/위치 샘플.

**Interfaces:** `FWeatherExposure EvaluateExposure(FVector WorldLocation) const` on ShelterVolume; `RegisterShelter/UnregisterShelter(AEnvironmentWeatherShelterVolume*)` on subsystem; `SetViewTransform(const FTransform&)` / `ApplyState(const FEnvironmentWeatherState&)` on precipitation component. `FWeatherShelterDescriptor`는 center, 회전축3개, half-extents, bBlockRain, feather, interior audio exposure다.

- [ ] `RotatedBoxExposure`, `OverlappingSheltersUseMinimum`, `ShelterRemovedDuringPlay`, `SegmentCrossingThinShelter`, `ShelterBudgetExceeded`를 작성한다. 기본 box 중심=0/실외=1, audio feather 연속성, bBlockRain=false에서 강수1 유지, audio .5 설정이어도 bBlockRain=true이면 강수0, volume 해제 후 복원을 검증한다.
- [ ] box geometry를 CPU 조회와 Niagara에서 같은 정의로 사용한다. GPU에는 최대16개 center/axes/extents 배열을 전달하고 실제 Niagara array 타입/API는 UE 5.7 헤더로 확인한다. 검증은 위치 샘플뿐 아니라 이전→현재 입자 선분도 포함한다.
- [ ] 월드 공간 강수와 카메라 중심 이동, 6000cm teleport reset, 1500/4000/8000 live cap을 구현한다. 프레임마다 컴포넌트를 생성하지 않는다. 17개 이상 겹치는 강수 영역은 명시적 validation failure로 비활성화한다.
- [ ] `GetStateAtLocation()`을 실제 shelter 노출도에 연결한다. 카메라 실내 여부로 전역 State/MPC나 전체 비를 끄지 않는다.
- [ ] `-Suite Shelter -Mode Logic` 통과 후 GPU로 열린 공간·창문·처마·회전된 실내·문턱·얇은 차폐·teleport 시나리오를 검증한다. phase1에 없는 자동 건물 판별을 통과 조건으로 주장하지 않는다.

**산출물:** 지정된 실내에는 비가 들이치지 않고 창밖 비는 유지되는 강수 표현.

### Task 5: 오리지널 음원 제작과 파일 QA

**Files:** Create `P/Resources/SourceAudio/{EW_Wind_Low,EW_Wind_Mid,EW_Wind_High,EW_Rain_Light,EW_Rain_Medium,EW_Rain_Heavy,EW_Gust_01,EW_Gust_02,EW_Gust_03,EW_Gust_04}.wav`, manifest/recipe/README; create `T/validate_audio.py`. 임시 합성 코드·시청 샘플은 `TunaSweeper/Saved/WeatherAuthoring/`에 둔다.

**Interfaces:** manifest version=1, 각 item에 name/path/seed/algorithm_version/sample_rate=48000/bits=16/channels/frame_count/loop/sha256/peak_dbfs/dc/loudness/source_method. 최종 loop frames=2,880,000, gust frames=288,000/336,000/384,000/480,000. Python checker는 manifest와 WAV를 읽어 JSON 결과·경고 timecode를 출력하고 mandatory failure에서 nonzero 종료한다.

- [ ] 먼저 20초 약풍·약한 비 샘플을 float 합성하고 로컬 오디오 플레이어로 제시한다. 유색잡음·필터·비 충격층을 조정하고 recipe에 선택값을 기록한다. 청취 의견을 기다리는 동안 다른 구현을 계속할 수 있다.
- [ ] seed를 고정한 여섯 루프/네 돌풍을 합성한다. 긴 루프는 주기적 합성 또는 0.5..2초 crossfade 구간을 사용하고 fade-to-zero 반복을 피한다. stereo 채널의 공통층/독립층 비율을 명시한다.
- [ ] 최종 float→16bit dither 변환 후 metadata·checksum을 기록한다. 신규 원본 출처는 자체 절차적 합성으로 표기하고 기존 음원/외부 다운로드로 바꾸지 않는다.
- [ ] `py -3 Tools/EnvironmentWeather/validate_audio.py --manifest TunaSweeper/Plugins/EnvironmentWeather/Resources/SourceAudio/manifest.json --report TunaSweeper/Saved/WeatherAudioQA/source.json`을 실행한다. 형식·길이·clipping/DC/peak는 mandatory, seam/mono/loudness 변화는 timecode와 함께 검토한다.
- [ ] 아래와 같은 FFmpeg 측정으로 true peak·loudness를 교차 확인한다. `ffmpeg -i <실제 WAV 경로> -af ebur128=peak=true -f null -`. 이 표기는 실행 시 manifest의 각 실제 경로로 치환한다. 자동 수치만으로 음색 품질을 통과시키지 않는다.
- [ ] 3..5분 반복 시청 샘플을 제공해 클릭·주기감·기계적 공명·고역 피로를 점검하고 보정한다. 선택된 10개 원본과 recipe/manifest를 최종 산출물로 남긴다.

**산출물:** 실제 신규 WAV 10개. 음원 제작 없이 단순 재생 코드만 완료하는 것으로 대체할 수 없다.

### Task 6: 음원 임포트와 날씨 믹서

**Files:** Create WeatherAudioComponent, `AudioTests.cpp`, `P/Content/Audio/{Loops,OneShots,Mix}`의 10 SoundWave 및 `SC_Weather`, `SM_Weather`, `SCN_Weather`; modify AudioProfile/Rig. `SM_Weather`는 SoundSubmix다.

**Interfaces:** `bool BeginPreloadAudio(UEnvironmentWeatherAudioProfile*)`는 요청 수락 여부, `OnAudioReady(bool bSuccess,FName ErrorKey)`는 비동기 완료; `ApplyState(const FEnvironmentWeatherState&,float AudioExposure01)`; `SetHostVolume(float)`; `SetWeatherVolume(float)`; `SetListenerLocationOverride(bool bEnabled,FVector Location)`; `StopWeatherAudio(float FadeSeconds)`.

- [ ] `GainCurveContinuity`, `FiveVoiceBudget`, `NoRestartOnStateUpdate`, `PauseResumePreservesPosition`, `WorldTravelStopsVoices`, `LateAudioLoadDoesNotResurrectWorld`, `MissingWaveOnlyDisablesItsLayer` 테스트를 작성한다. 10Hz로1분 상태를 바꿔도 loop Play 호출이 재시작되지 않는지 계측한다. stop/travel 뒤 도착한 비동기 load 완료는 generation token으로 무시한다.
- [ ] WAV를 SoundWave로 임포트하고 loop 6개에 엔진 looping 설정을 적용한다. 재생 전에 비동기로 로드한다. 과한 리샘플링이 음색을 바꾸는지 프로젝트 quality setting과 함께 확인한다.
- [ ] 각 강도의 인접 2단계만 equal-power 혼합한다. 비=0/바람=0에서 무음, 실내 rain=-24dB/wind=-16dB/LPF1800Hz, Exposure smoothing=.75초를 적용한다. profile·호스트 volume은 gain에 각각 한 번만 곱한다.
- [ ] Task2의 OnGustStarted와 길이에 맞는 variant를 연결한다. 최대1개 accent를 유지하고 루프 위에 one-shot을 겹친다. 복원 즉시/늦은 async 완료에 이미 지난 사건을 재생하지 않는다. 볼륨0에서도 코어 seed가 오디오 조건 때문에 달라지지 않게 한다.
- [ ] BGM 채널과 독립임을 확인한다. pause/resume, 10회 travel, 두 PIE 월드, Rig 해제에서 voice 수가 원래대로 돌아와야 한다.
- [ ] `-Suite Audio -Mode Logic`와 실제 Audio 모드 캡처를 모두 수행한다. compressed SoundWave를 5회 이상 반복하고 seam·최대교차에서 weather submix true peak <=-6dBTP를 확인한다. `-nosound` 결과는 청취/루프 품질 근거로 인정하지 않는다.

**산출물:** 날씨·실내외에 반응하는 오디오와 검증된 임포트 애셋.

### Task 7: 기존 나무·연기·지역 안개의 경량 연동

**Files:** Create game WeatherBindingComponent and `TunaSweeperWeatherBindingTests.cpp`; modify `G/{Public,Private}/Environment/TunaSweeperAnimeTreeActor.{h,cpp}`, `TunaSweeper/Plugins/RegionalGroundFog/Source/RegionalGroundFog/{Public,Private}/RegionalGroundFogActor.{h,cpp}`, `G/TunaSweeper.Build.cs`. ExtractionPointActor는 기존 API를 소비하며 변경은 필요성이 확인될 때만 한다.

**Interfaces:** Tree `void SetWeatherWindStrength(float Strength)` / `void ClearWeatherWindOverride()`; Fog `void SetEnvironmentModifiers(float DensityMultiplier,FVector2D WindVelocityCmPerSecond)` / `void ClearEnvironmentModifiers()`; game bridge `void ApplyWeather(const FEnvironmentWeatherState&)` / `void RestoreBindings()`.

- [ ] Tree shadow proxy와 Fog card pointer/개수를 저장하고 100회 적용 후 동일함을 확인하는 `NoTreeProxyRebuild`, `NoFogCardRespawn` 회귀 테스트를 작성한다. 해제 후 원래 authored 설정 복원을 검증한다.
- [ ] Tree의 런타임 override만 MID에 적용한다. 기본 값은 수정하지 않고 새 MID 생성 시 active override를 다시 적용한다. 강도 mapping은 `clamp(WindSpeed/1000 + .25*Gust,0,1.5)`로 시작한다.
- [ ] Fog는 기존 radial/height density에 multiplier를 적용하며 카드 opacity도 같은 비율로 조절한다. drift에 local actor 공간으로 변환한 world wind×.02를 더하고 기존 카드 방향/phase는 유지한다. `RefreshFogRegion`/카드 생성은 호출하지 않는다.
- [ ] bridge는 명시적으로 연결한 actor 목록만 관리한다. smoke/fog에는 `GetWindVelocityCmPerSecond()`의 방향·속도, fog density에는 `lerp(.25,1.5,FogAmount)`를 전달한다. Target을 매 Tick 전체 월드 검색하지 않는다.
- [ ] 게임 모듈에 EnvironmentWeather/RegionalGroundFog 의존성을 필요한 Private 영역에 추가한다. 이미 RegionalGroundFog가 enable된 현재 호스트에만 연결하며 EnvironmentWeather 플러그인에는 게임/안개 의존성을 추가하지 않는다.
- [ ] `-Suite Integration -Mode Logic/Render`에서 opted-in 대상만 바뀌는지, unbound 나무/안개/물/BGM이 영향을 받지 않는지 검증한다.

**산출물:** 현재 프로젝트 표현을 재사용하면서 재생 중 재생성 비용을 피하는 연결부.

### Task 8: 에디터 미리보기와 데이터 검증

**Files:** Create EditorModule/Details/Preview와 PreviewTests; create `P/Content/Localization/EnvironmentWeather.csv`; modify Presentation/Profile validation.

**Interfaces:** Editor 전용 `BeginPreview(AEnvironmentWeatherRig*)`, `SetPreview(FName ProfileId,float BlendAlpha)`, `EndPreview()`와 `ValidateConfiguration(const UEnvironmentWeatherConfiguration*,TArray<FName>& ErrorKeys)`.

- [ ] `PreviewRestoresOriginals`, `PreviewDoesNotDirtyAssets`, `PreviewSeparateFromPIE`, `AllDisplayTextUsesKeys`를 작성한다. preview 종료·맵 변경·module 종료·PIE 시작에서 복원 여부를 검사한다.
- [ ] 기존 Details customization으로 preset/전환 scrub/seed/품질/audio preview opt-in/Validate를 노출한다. 새로운 UMG/Slate 위젯 클래스를 만들어 이름 규칙을 우회하지 않는다.
- [ ] PixelWorldBuilder의 CSV 등록·`FText::FromStringTable` 패턴을 따르되 날씨 고유 namespace/key를 사용한다. display name·tooltip·오류 문구를 키로 조회한다.
- [ ] duplicate profile ID, missing assets, unsupported cloud contract, shelter budget, invalid curve/NaN, snapshot config mismatch를 편집 시 진단한다. 미리보기의 일시적 객체/동적 머티리얼을 저장된 콘텐츠에 기록하지 않는다.
- [ ] `-Suite Preview -Mode Logic/Render`를 수행하고 preset 수정 Undo/Redo와 preview 원상복원을 확인한다.

**산출물:** 디자이너가 코드를 바꾸지 않고 연출을 조절하고 잘못된 설정을 발견하는 도구.

### Task 9: 독립 시연 맵과 콘텐츠 의존성 완성

**Files:** Create `P/Content/Profiles/DA_Weather{Clear,Overcast,LightRain,HeavyRain,Fog,Configuration,Presentation,Audio}.uasset`, `P/Content/Profiles/PAL_WeatherRuntime.uasset`, `P/Content/Maps/L_EnvironmentWeather_Showcase.umap`, `TunaSweeper/Content/Maps/Tests/L_EnvironmentWeatherIntegration.umap`, `AssetTests.cpp`; create `P/README.md`.

**Interfaces:** plugin config ID `EnvironmentWeather.Default`, preset IDs `Clear`, `Overcast`, `LightRain`, `HeavyRain`, `Fog`. Rig/config의 reflected reference를 통해 runtime assets를 추적할 수 있어야 한다.

- [ ] spec 값으로 다섯 프리셋과 config를 저장한다. 자동 스케줄은 기본 off이며 시연 시 명시적으로 켠다.
- [ ] 시연 맵의 실외·창문 있는 방·처마·겹치는/회전된 차폐·wetness 샘플·풍향 표식을 만든다. 평상시 플레이용 게임 UI는 추가하지 않는다.
- [ ] 프로젝트 통합 맵에 opt-in 나무·연기·지역안개와 unbound 비교 대상을 배치한다. 이 맵과 자산은 플러그인이 역참조하지 않게 한다.
- [ ] `PAL_WeatherRuntime`에서 runtime profiles/audio/fx/materials를 명시적으로 AlwaysCook하며 sample map은 자동 포함하지 않는다. 실사용 게임에서 rig/config가 참조되는 경로와 config의 모든 soft references를 Asset Registry로 검사한다.
- [ ] `NoProjectBackReferences`, `AllProfileAssetsResolvable`, `SavedAssetsWithoutGenerator`, `LocalizationKeysResolvable` 검사와 두 맵 시나리오를 실행한다. 기존 `/Game/Audio` cook 규칙에 기대지 않는다.

**산출물:** 다른 프로젝트에 복사할 수 있는 완성 콘텐츠와 현재 게임에서의 비교 시연.

### Task 10: 성능·이식·cook 통합 검증

**Files:** Create `T/validate_portability.ps1`, `T/README.md`; extend test wrapper와 보고서 수집. 결과는 `TunaSweeper/Saved/WeatherValidation/`에 저장한다.

**Interfaces:** `validate_portability.ps1 -EngineRoot <UE5.7경로> -PluginPath <EnvironmentWeather경로>`는 새 독립 호스트를 Saved/WeatherValidation 아래 고유 디렉터리에 만들고 기본 플러그인만 복사한다. 원본 프로젝트 설정과 다른 저장소를 변경하지 않는다.

- [ ] `-Suite All -Mode Logic`를 한 번 실행해 테스트 누락0/실패0을 확인한다. 실패 관련 수정이 생긴 suite만 다시 실행하고 최종 한 번 통합한다.
- [ ] GPU/Audio 활성 상태에서 다섯 프리셋·전환 중 재요청·pause·실내외·teleport·10회 travel을 검증한다. 시각 캡처와 소리 캡처를 각각 저장한다.
- [ ] 같은 장치/RHI/1080p/카메라에서 60초 warmup+120초 clear/heavy-rain을 측정한다. High CPU p95 추가 .5ms/GPU p95 추가2ms를 제안 목표로 비교하고 초과 원인/튜닝을 기록한다. NullRHI 성능으로 대체하지 않는다.
- [ ] 깨끗한 UE5.7 테스트 호스트에서 플러그인 샘플을 실행한다. public plugin include/build/assets에 TunaSweeper/다른 플러그인 종속성이 없는지 검증한다.
- [ ] Windows BuildCookRun은 upfront escalation과 고유 로그로 실행한다. 테스트 호스트의 샘플 맵만 cook/package하고 실행해 missing package0, SoundWave/Niagara/머티리얼 로드 성공을 확인한다. 실제 스토어 배포와 기존 릴리스 패키지는 만들지 않는다.
- [ ] 압축 후 루프와 믹스의 수치 결과를 정리하고 사용자에게 최종 음원/연속 녹음을 제공한다. 사용자 청취 결론을 자동화 성공과 구분해 기록한다.

**산출물:** 기능·성능·이식·cook 증거. 플랫폼/장치/청취 미검증 항목은 명시한다.

### Task 11: 생성기 정리, 문서와 최종 재검증

**Files:** Modify `P/README.md`, SourceAudio README/manifest, `Docs/save_persistence.md`, `Docs/requests.md`; remove 이번 작업에서 만든 임시 합성/애셋 생성 코드와 진입점/생성기 전용 의존성만.

- [ ] 생성한 파일 목록을 근거로 일회성 editor generator와 startup 경로를 제거한다. Windows 삭제는 절대 경로가 작업용 디렉터리 아래인지 확인하고 PowerShell `-LiteralPath`로 수행한다. 타 작업의 임시 파일은 삭제하지 않는다.
- [ ] 원본 WAV·recipe·manifest·저장된 uasset은 보존하고 generator 없는 상태로 Editor 빌드, Assets/Integration 테스트, 실제 시연을 다시 확인한다. binary 자산을 생성기 호출로 복구해야만 실행된다면 완료로 처리하지 않는다.
- [ ] README에 설치·Rig 바인딩·차폐 작성·오디오 교체·volume API·문자열 테이블·품질·제약을 기록한다. `Docs/save_persistence.md`에는 날씨가 세이브에 기록되지 않고 map entry default이며 snapshot API는 명시적 호스트 호출용이라는 정책을 기록한다. 기존 save 버전은 바꾸지 않는다.
- [ ] 요청 로그에 구현·음원 목록·검증 결과·청취 미확정 항목·정리 결과를 실제 소요시간으로 기록한다. 이번 계획 작성 기록과 나중 구현 기록을 혼동하지 않는다.
- [ ] diff/자산 의존성/파일 목록을 검토하고 사용자의 명시적 요청 전까지 stage/commit/push하지 않는다. 완료 보고에는 문서·음원 청취 링크·검증 결과와 남은 제한만 담는다.

**산출물:** 실행에 필요 없는 생성 코드가 남지 않고, 재사용 방법과 검증 근거가 포함된 작업 단위.

## 계획 자체 검토와 실행 방식

- 데이터/API는 Task 1/2를 공통 경계로 사용한다. 동일한 이름과 단위가 시각·음향·프로젝트 어댑터에 전달된다.
- 상태·차폐·오디오 수명·기존 컴포넌트 재생성·미리보기·cook의 주요 실패 조건을 각 Task에 배치했다.
- 실제 구현에서는 공통 계약을 한 실행자가 먼저 완성하고, 음원 제작과 시각 표현처럼 파일 소유권이 분리된 작업만 병렬화하는 방식을 추천한다. 최종 통합·생성기 정리·검증은 순서대로 수행한다.
- 최초 실행 전 최신 작업 트리와 조율 상태를 다시 읽는다. 본 계획은 2026-11-09의 공개 코드 확인을 바탕으로 하며, 이후 바뀐 코드/자산을 덮어쓰는 권한이 아니다.

