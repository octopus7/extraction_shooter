# TunaSweeper UE 5.7.4 → 5.8.4 마이그레이션 계획

**목표**는 기존 5.7.4 플레이어의 세이브, 화면, 조작, 스토어 기능을 유지한 채 Win64 배포판을 5.8.4로 빌드하는 것이다. 첫 빌드는 Development Editor로, 첫 패키징은 Steam 데모 Shipping으로 진행한다. 전체 전환은 나머지 타깃과 기존 세이브 검증까지 마친 뒤 결정한다.

2026-11-20에 공개 저장소 `cdff6cdc7dec`와 작업 트리, 로컬 엔진 설치 파일을 조사했다. 아래 체크박스는 후속 실행 때 채운다. 비교 자료는 엔진 버전과 타깃별로 작업 사본의 `Saved` 아래에 보관한다.

## 조사 결과

| 확인 대상 | 현재 상태와 전환에 미치는 영향 |
| --- | --- |
| 로컬 엔진 | 기존 설치는 `C:\Program Files\Epic Games\UE_5.7`의 **5.7.4**다. 후보 설치는 `E:\WorkTemp\Epic Games\UE_5.8`이며, `Engine/Build/Build.version`과 `Engine/Binaries/Win64/UnrealEditor.version` 모두 **5.8.4, CL 58902052**다. Epic Launcher 설치 목록에는 같은 경로가 5.8.3으로 남아 있다. 실행할 파일의 버전과 빌드 로그로 판정한다. |
| 타깃과 도구 | `.uproject`의 엔진 연결은 `5.7`이다. Editor 1개와 Game 6개가 전부 `BuildSettingsVersion.V6` 및 `Unreal5_7`을 쓴다. 에디터 실행 배치와 스토어 패키징 배치 4개도 `UE_5_7_ROOT`를 읽는다. [프로젝트](../../../TunaSweeper/TunaSweeper.uproject) · [실행 배치](../../../TunaSweeper/BatchScripts/BuildAndRunTunaSweeper.bat) |
| 플러그인 | `.uproject`에 활성화된 플러그인 27개를 5.8 엔진 및 프로젝트의 `.uplugin`과 대조했다. **`DLSS`와 `DLSSMoviePipelineSupport` 두 개가 누락**됐다. 5.7 엔진에는 둘 다 8.6.0 계열이 설치돼 있고 게임은 `DLSSBlueprint`에 직접 의존한다. 저장소 자체 플러그인은 18개다. `LoopRail`, `CoastalFish`, `CoastalWildlife`처럼 기본 활성인 플러그인도 빌드 대상에 포함한다. [게임 모듈 의존성](../../../TunaSweeper/Source/TunaSweeper/TunaSweeper.Build.cs) · [NVIDIA의 UE 5.8용 배포본](https://developer.nvidia.com/rtx/dlss) |
| C++와 셰이더 | 5.8.4의 `JsonObject.h`는 기본적으로 `UE::FSharedString`을 키로 쓴다. 프로젝트에는 `FJsonObject::Values`의 키를 `FString`으로 명시한 순회문이 세 곳 있다. 숲 수목의 `CanopyRim`은 `BeforeDOF`에서 장면 깊이와 스텐실을 읽어 전용 셰이더로 윤곽을 그린다. 전자는 컴파일과 데이터 로딩, 후자는 실제 GPU 화면으로 확인할 수 있다. [Epic JSON API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/Json/FJsonObjectSharedStringStorage?application_version=5.8) · [수목 렌더 패스](../../../TunaSweeper/Source/CanopyRim/Private/CanopyRimViewExtension.cpp) |

## 1. 5.7 기준선과 복구본 확보

- [ ] 같은 소스·콘텐츠 시점으로 5.7 기준본과 5.8 전용 체크아웃을 준비한다. 조사 당시 `RaidForest.umap`에는 미커밋 변경이 있었다. 작업 중인 맵을 비교에 포함한다면 소유 작업에서 저장한 동일 파일을 양쪽에 반영하고 해시를 기록한다. `Binaries`, `Intermediate`, 기존 Cook·Stage 결과는 사본별로 새로 만든다. 5.8에서 저장한 패키지를 다시 5.7에서 여는 데 의존하면 원본 복구가 어렵다. [Epic 프로젝트 전환 안내](https://dev.epicgames.com/documentation/unreal-engine/updating-projects-to-newer-versions-of-unreal-engine?application_version=5.8)
- [ ] 5.7에서 아래 자동화의 실제 실행 이름·결과, 숲 고정 시점 캡처, 패키징 성공 로그와 실행 가능한 기존 배포본을 확보한다. 이미 실패하는 검사가 있으면 재현 조건을 기록해 5.8에서 새로 발생한 실패를 구별한다.
- [ ] 지원 중인 세이브 버전 20·21·22의 샘플과 현재 슬롯, 업적, 이전 세대 파일을 보관한다. 각 샘플에 기대하는 아이템 수량·퀘스트 진행·철망문 상태를 적는다. 읽기 전용 원본에서 매번 새 복사본을 만들고 스토어 검증에는 테스트 계정을 사용한다. [버전 상수](../../../TunaSweeper/Source/TunaSweeper/Private/Game/TunaSweeperGameInstanceShared.h) · [저장 경로와 영속 상태](../../save_persistence.md)

**통과 기준**은 5.7 실행 파일, 원본 맵, 원본 세이브를 회수할 수 있고 같은 장면과 슬롯을 두 버전에서 비교할 수 있는 상태다.

## 2. 후보 엔진과 빌드 진입점 정렬

- [ ] 기존 `AGENTS.md`와 에디터 조정 규칙은 원본 프로젝트를 UE 5.7로 한정한다. 마이그레이션 실행 시 전용 체크아웃에 적용할 **5.8 실행 파일과 `.uproject` 절대 경로 쌍**을 먼저 명시한다. 에디터 종료·재개는 해당 쌍의 소유 작업과 조정한다. [에디터 조정 절차](../../unreal_editor_coordination.md)
- [ ] 후보 환경에 5.8용 NVIDIA 패키지를 준비해 두 디스크립터와 `DLSSBlueprint` 모듈이 발견되는지 확인한다. Windows 빌드 환경도 기록한다. Epic의 VS 2022 지원 기준은 17.14 이상이며 VS 2026은 18.0 이상이다. [Visual Studio 호환표](https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine?application_version=5.8)
- [ ] 사본의 `.uproject`는 `EngineAssociation = "5.8"`로, `TunaSweeper/Source`의 7개 `*.Target.cs`는 설치된 5.8.4 엔진의 기본 템플릿에 맞춰 `BuildSettingsVersion.V7` 및 `EngineIncludeOrderVersion.Unreal5_8`로 바꾼다. 게임별 `CustomConfig`, 데모 정의, 스토어 식별 설정과 `bOverrideBuildEnvironment`의 기존 용도를 보존한다.
- [ ] 실행 배치와 4개 패키징 배치가 `UE_5_8_ROOT`를 우선 사용하고, 기본 탐색 시 등록된 5.8 설치 경로를 쓰도록 맞춘다. 각 배치가 실제 `Build.bat` 또는 `RunUAT.bat` 경로를 출력하도록 한다. 패키징 테스트의 가짜 엔진 환경 변수도 같이 갱신한다. [패키징 배치](../../../TunaSweeper/BatchScripts/) · [검증용 가짜 엔진을 사용하는 테스트](../../../TunaSweeper/BuildScripts/TestQuestPackagingValidation.ps1)
- [ ] `rg -l 'UE_5_7_ROOT|UE_5\.7|Unreal5_7' TunaSweeper/Source TunaSweeper/BatchScripts TunaSweeper/BuildScripts Tools .codex/skills/tunasweeper-ui-texture-import`로 도구 진입점을 점검한다. `Tools/MainRaidLevels`, `Tools/PixelWorldBuilder`, UI 텍스처 임포터와 `Tools/SaveTool/CommandletClient.cs`가 대표적이다. SaveTool의 명시 인자와 `TUNASWEEPER_EDITOR` 우선순위를 보존하고, 이번 검증에 쓸 도구부터 엔진과 프로젝트 경로를 확인한다.

**통과 기준**은 테스트용 `TestQuestPackagingValidation.ps1`와 `TestBuildFlavorData.ps1`이 성공하고, 실제 빌드 명령이 후보 엔진과 전용 프로젝트를 가리키는 것이다.

## 3. C++ 빌드와 콘텐츠 로드

- [ ] 후보 저장소 루트에서 아래와 같이 첫 에디터 빌드만 수행한다. `BuildAndRunTunaSweeper.bat`는 성공 직후 에디터도 실행한다. 엔진 설치별 빌드 뮤텍스와 별도로 UBT 로그 위치가 공유되므로 직접 빌드할 때는 실행마다 로그 이름을 다르게 준다.

```powershell
$env:UE_5_8_ROOT = 'E:\WorkTemp\Epic Games\UE_5.8'
$project = (Resolve-Path 'TunaSweeper/TunaSweeper.uproject').Path
if ((Get-Content -LiteralPath $project -Raw | ConvertFrom-Json).EngineAssociation -ne '5.8') { throw '5.8 전용 사본에서 실행하세요.' }
$logDir = (New-Item -ItemType Directory -Force 'TunaSweeper/Saved/Logs/Build').FullName
$log = Join-Path $logDir ('UE58-Editor-' + [guid]::NewGuid().ToString('N') + '.log')
& (Join-Path $env:UE_5_8_ROOT 'Engine/Build/BatchFiles/Build.bat') TunaSweeperEditor Win64 Development "-Project=$project" -WaitMutex "-Log=$log"
```

- [ ] 첫 컴파일 오류부터 수정한다. JSON은 [아이템 중첩 설정](../../../TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp), [업적 플랫폼 ID](../../../TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperAchievementSubsystem.cpp), [소음원 머티리얼](../../../TunaSweeper/Source/TunaSweeper/Private/Interaction/TunaSweeperPeriodicNoiseEmitterActor.cpp)의 세 순회문을 확인한다. 실제 키 타입으로 순회하고 공백 제거 또는 소유 문자열을 요구하는 호출 지점에서 변환한다. 같은 입력으로 아이템 최대 중첩 수량, 업적 ID, 머티리얼 색상이 5.7과 일치해야 한다.
- [ ] 에디터 빌드 후 `TunaSweeperNoStoreDemo`와 `TunaSweeperNoStore`를 Win64 Development로 각각 빌드해 게임 전용 경로의 링크 오류를 찾는다. `CanopyRim`의 `PostConfigInit`, `TunaSweeperAnimGraph`의 `UncookedOnly`, `TunaSweeperEditor`의 `Editor` 모듈 경계를 유지한다.
- [ ] 후보 에디터에서 `WBP_TitleSettings`, `ABP_LunaMk2_Title`, 기지와 숲 맵을 열어 컴파일·로드한다. 타이틀 머리 추적 노드와 위젯 바인딩을 확인하고 로그에서 누락 클래스·셰이더 오류를 찾는다. 변환 저장이 실제로 필요한 에셋만 저장하고 에디터 재시작 후 다시 연다.

**통과 기준**은 세 타깃의 빌드 종료 코드가 0이고, 대표 맵·블루프린트·셰이더가 새 프로세스에서 정상 로드되는 것이다.

## 4. 자동화와 실제 플레이 비교

각 테스트를 후보 엔진의 `UnrealEditor-Cmd.exe`와 사본의 `.uproject`로 실행한다. 필터마다 `-ExecCmds="Automation RunTests <필터>"`, `-TestExit="Automation Test Queue Empty"`, 고유한 `-ReportExportPath`를 사용한다. `index.json`에서 예상 테스트 이름이 실제로 실행됐고 실패가 0인지 확인한다.

| 검증 대상 | 기존 자동화 필터와 추가 확인 |
| --- | --- |
| 아이템·업적·화면 | `TunaSweeper.Achievement.DefinitionValidation`, `TunaSweeper.NoiseEmitter.AuthoredMeshAndPulse`, `TunaSweeper.UI.Localization.RefreshesTitleAndGraphicsLabels`. 타이틀에서 언어와 그래픽 옵션을 바꾸고 라벨, 선택값, 재실행 후 표시를 확인한다. |
| 수목·물·렌더링 | `TunaSweeper.ForestScatter.DensityAndSeed`, `TunaSweeper.AnimeTree.Rendering`, `StylizedWater.MaskWater.RenderMatrix`. 렌더 검사는 DX12/SM6 GPU로 실행한다. 수목 테스트는 `NullRHI`에서 캡처를 건너뛰고 성공을 반환하므로 캡처와 셰이더 로그도 확인한다. [수목 테스트](../../../TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperAnimeTreeTests.cpp) |
| 차량·저장 | `TunaSweeper.Vehicle.Driving`, `TunaSweeper.Vehicle.PedestrianContact`, `TunaSweeper.Interaction.ChainLinkGate.Persistence`, `TunaSweeper.Save.VersionPolicy`, `TunaSweeper.BuildFlavor.Paths`. ATV의 평지 출발·경사면 접지·충돌과 저장된 문 상태를 플레이로 확인한다. |
| 온라인·계정 경로 | `TunaSweeper.OnlineCoop.Code`, `TunaSweeper.OnlineCoop.Lifecycle`, `TunaSweeper.Save.StoveDirectoryRouting`. 실제 서비스 접속은 다음 단계의 패키지에서 확인한다. |

- [ ] `RaidForest`의 숲 절벽·나무 윤곽·수면·안개가 함께 보이는 같은 카메라를 사용한다. 동일 GPU·드라이버·출력 해상도·그래픽 프리셋으로 DLSS 끔과 켬을 각각 비교한다. 모드별 내부 해상도도 기록한다. 셰이더 준비가 끝난 뒤 같은 경로를 30초씩 3회 재생해 GPU 프레임 시간 중앙값과 95백분위를 비교한다. 푸른 테두리, 수목 후광, 물 마스크 어긋남 또는 5.7 측정 범위를 반복적으로 벗어나는 성능 저하가 있으면 원인을 확인한다. [현재 렌더 설정](../../../TunaSweeper/Config/DefaultEngine.ini)
- [ ] 5.7 세이브 복사본으로 **기지에서 불러오기 → 레이드 진입 → 귀환 → 저장 → 게임 종료·재실행**을 진행한다. 아이템 수량, 퀘스트, 철망문, 업적과 선택 슬롯을 기대값과 대조한다. Demo와 FullGame에는 서로 구분되는 샘플을 넣어 다른 판의 슬롯이 나타나는지도 확인한다.

**통과 기준**은 지정한 자동화가 실제로 실행되고, 숲 화면과 ATV 조작에서 새 회귀가 없으며, 재시작한 세이브의 진행 값이 모두 맞는 것이다. GPU 캡처와 프레임 측정 결과를 자동화 보고서와 함께 남긴다.

## 5. Win64 Shipping과 스토어별 검증

스토어 패키징에 착수할 때 [패키징 체크리스트](../../store_packaging_checklist.md)를 적용한다. Windows `RunUAT BuildCookRun`은 AppData 쓰기가 필요하므로 시작할 때 실행 권한을 요청한다. 네 배치가 스테이징 디렉터리를 공유하고 기존 출력 폴더를 지우므로 **같은 체크아웃에서는 하나씩**, 보존할 5.7 아카이브와 다른 위치에서 실행한다. 첫 시험은 `PackageTunaSweeperSteamDemoWin64.bat Shipping`이다.

| 구성 | 정식판 타깃 / `CustomConfig` | 데모 타깃 / `CustomConfig` | 확인할 결과 |
| --- | --- | --- | --- |
| Steam | `TunaSweeper` / `Full` | `TunaSweeperDemo` / `Demo` | 두 Shipping 배치의 빌드·Cook·실행. 동일 판의 두 PC·Steam 계정으로 EOS 8자리 코드 참가, `CoopStaging` 도착, 호스트 퇴장 후 복귀. 계정별 저장 경로와 재시작 후 슬롯 확인. [협동 테스트 절차](../../eos_steam_coop_manual_test.md) |
| STOVE | `TunaSweeperStove` / `Stove` | `TunaSweeperStoveDemo` / `StoveDemo` | 두 Shipping 배치의 빌드·Cook·실행. PC SDK 3.4.2의 `BaseSDK.dll`, `OwnershipSDK.dll`, `LogSDK.dll` 스테이징, 테스트 계정의 SDK 초기화·클라우드 경로 및 경로 조회 실패 시 `LocalOnly` 저장 확인. [STOVE 저장 검증](../../stove_cloud_saves.md) |
| NoStore | `TunaSweeperNoStore` / `NoStore` | `TunaSweeperNoStoreDemo` / `NoStoreDemo` | 3단계의 Development 확인 후 두 타깃을 Win64 Shipping으로도 컴파일해 배포 설정의 링크를 확인한다. 이 두 타깃에는 전용 스토어 패키징 배치가 없다. |

- [ ] 네 배치의 번역 검사와 `ValidateMovementCheatPackage.ps1`가 성공하는지 확인한다. Demo는 `PrepareDemo`·`VerifyDemo`, 정식판은 `PrepareMain`·`Clean`을 거친다. 예외 종료 시 임시 정식판 스테이징을 확인해 정리한 뒤 재시도한다. 기존 승인된 정식판 입력은 해당 환경에서만 사용한다. [빌드별 데이터 처리](../../../TunaSweeper/BuildScripts/BuildFlavorData.ps1)
- [ ] Demo 아카이브의 느슨한 `Content/Data` 파일과 Pak/IoStore 컨테이너 목록을 모두 점검한다. 데모에 정식판 데이터나 `/Game/MainRaid` 맵이 섞이면 중단한다. Steam·STOVE 데모의 명시적 `-map` 목록이 다르므로 실제 접근 가능한 데모 레이드 경로를 각 빌드에서 끝까지 연다. 정식판에서는 `RaidForest`, `RaidVillage`, `RaidPlains`를 로드한다. [데모 Cook 설정](../../../TunaSweeper/Config/Custom/Demo/DefaultGame.ini) · [STOVE 데모 Cook 설정](../../../TunaSweeper/Config/Custom/StoveDemo/DefaultGame.ini)
- [ ] 각 스토어의 테스트 계정에서 해당 판의 5.7 슬롯 복사본으로 4단계의 저장·재시작을 반복한다. 기기 간 동기화는 다른 테스트 PC에서 동일 계정으로 진행 값을 확인한다.

**통과 기준**은 스토어별 Shipping 패키지가 실행되고, 선택한 판의 맵·데이터·세이브 경로가 일치하며, Steam 협동과 STOVE 인증·저장이 실제 테스트 계정에서 확인되는 것이다. 계정이나 정식판 입력을 사용할 수 없으면 해당 행에 미검증 사유를 남긴다.

## 6. 기본 엔진 전환과 복구

- [ ] 빌드 로그, 자동화 결과, 5.7/5.8 비교 캡처와 프레임 시간, 구버전 세이브 재시작 결과, 네 Shipping 패키지의 실기 결과를 한 표에서 대조한다. 남은 회귀와 서비스 미검증 항목이 해소된 시점에 기본 엔진을 전환한다. `AGENTS.md`, 에디터 조정 문서, 평소 쓰는 실행·패키징 진입점의 버전을 함께 갱신한다. [5.8.4 핫픽스 내역](https://forums.unrealengine.com/t/5-8-4-hotfix-released/2842775)
- [ ] 전환 전 시험이 실패하면 후보 사본과 로그를 보존하고 5.7.4 작업본·패키지로 개발을 이어간다. 배포 후 되돌리기에 대비해 **5.8이 재저장한 테스트 슬롯의 별도 복사본**을 5.7.4에서 읽고 진행도가 유지되는지도 미리 시험한다. 역호환이 실패하면 플레이어의 최신 진행을 보존할 복구 경로가 마련될 때까지 구버전 재배포를 보류한다.
