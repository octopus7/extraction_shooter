# 스토어 빌드 패키징 점검

스토어 배포용 패키징·재패키징 작업을 시작할 때만 읽는 점검 문서다. 일반 코드 수정이나 에디터 빌드에는 적용하지 않는다. 각 항목은 실제 확인 결과를 기록하고, 실행하지 못한 검증은 미확인으로 보고한다.

## 1. 대상과 입력 확인

- 요청한 스토어(Steam/STOVE), 제품(Demo/Full), 구성(배포 기본 Shipping), 버전을 확인한다.
- `TunaSweeper/BatchScripts/PackageTunaSweeper{Steam|Stove}{Demo|Full}Win64.bat` 중 해당 스크립트의 타겟, CustomConfig, 출력 경로를 확인한다. 에디터에서 마지막으로 선택한 미리보기 환경만으로 배포 대상을 판단하지 않는다.
- `TunaSweeper/Config/DefaultGame.ini`의 `ProjectVersion`과 해당 CustomConfig의 배포 채널·빌드 종류·온라인 서비스·업적 네임스페이스가 일치하는지 확인한다.
- STOVE는 `Docs/Stove/SDKIntegration.md`를 추가로 읽고 로컬 SDK와 선택 제품에 맞는 인증 정보가 준비되어 있는지 확인한다. Demo/Full이 인증 파일을 자동으로 구분하지 않으므로 제품 일치 여부를 확인한다. 인증값은 출력하거나 문서에 기록하지 않는다.

## 2. 스토어 설정 분리

- 프로젝트 패키징 설정의 `bMakeBinaryConfig=True`를 유지한다. 설치형 엔진에서는 게임 타겟의 `CUSTOM_CONFIG` 정의만으로 런타임 설정 선택이 보장되지 않는다.
- UAT가 선택한 CustomConfig로 `MakeBinaryConfig`를 실행하고 최신 `Config/BinaryConfig.ini`를 PAK에 포함하는지 확인한다. 설정 변경 시 `-skippak`으로 이전 PAK을 재사용하지 않는다. 런타임 검증에서 `-textconfig`로 바이너리 설정을 우회하지 않는다.
- STOVE Demo는 `TunaSweeperStoveDemo` / `StoveDemo` 조합, `DistributionChannel=Stove`, `BuildType=Demo`, `DefaultPlatformService=NULL`, `OnlineSubsystemSteam.bEnabled=false`, 업적 `DistributionNamespace=Stove`인지 확인한다.
- STOVE 빌드에서는 Steam 전용 위시리스트가 숨겨지고 Steam 온라인 서비스가 활성화되지 않아야 한다. Steam 빌드에서는 STOVE SDK가 링크·초기화되지 않아야 한다. 기능 비활성화와 모든 관련 파일의 물리적 제거는 별개다.
- 타이틀 버전의 채널 접미사와 위시리스트 표시 조건은 같은 `GetDistributionChannel()` 배포 채널 값을 참조한다. 패키지에서는 `GGameIni`의 `[TunaSweeper.Distribution] DistributionChannel`을 읽는다. 위시리스트는 이 값이 `Steam`이고 Demo 빌드인 경우에만 표시하며, 완성된 버전 문자열을 파싱해서 판단하지 않는다. 에디터 미리보기는 `UTunaSweeperBuildTargetSettings`의 선택값을 사용한다.

## 3. 빌드와 산출물 검증

- UE 5.7로 패키징한다. `RunUAT BuildCookRun`은 AppData 로그·캐시 쓰기를 위해 처음부터 escalated 권한으로 실행한다.
- 기본은 대상 스크립트의 전체 빌드·쿡·패키징이다. 코드/설정만 변경했고 동일 타겟의 유효한 쿡 결과가 있을 때만 `-skipcook`을 사용한다. 콘텐츠·맵·데이터셋 또는 쿡에 영향을 주는 설정을 변경했다면 다시 쿡한다.
- 최종 UAT `BUILD SUCCESSFUL`과 `ExitCode=0`을 확인한다. 패키징된 실제 실행 파일이 최신 컴파일 산출물과 일치하는지 확인한다.
- Demo는 `TunaSweeper/BuildScripts/BuildFlavorData.ps1 -Mode VerifyDemo -ArchiveDirectory <해당 아카이브 경로>`를 실행하고 통과 여부를 확인한다. 배포 대상 맵과 데이터셋이 맞는지 확인한다.
- STOVE는 실제 실행 파일 옆에 `BaseSDK.dll`, `OwnershipSDK.dll`, `LogSDK.dll`이 있고 SDK 원본과 일치하는지 확인한다. 인증 `.env`, Application Secret, SDK ZIP·헤더·라이브러리가 패키지에 유출되지 않도록 확인한다.
- STOVE 인증 코드를 변경했다면 `Tools/StoveTests/RunTests.bat`를 실행한다. 정확한 게임 ID와 보유 상태를 요구하는 판정을 유지한다. Demo 여부만으로 BASIC(3)을 거부하지 않는다.

## 4. 실행 확인과 전달

- 최신 출력본으로 타이틀 버전·채널·Demo 접미사, 긴 버전 문구의 화면 잘림, 스토어 전용 버튼 표시를 확인한다. STOVE Demo의 기대 표기는 `v<버전>.stove.demo`다.
- STOVE 런처에서 대상 제품의 인증·소유권 확인 후 게임 진입과 정상 종료를 확인한다. 패키징 성공이나 로컬 개발 정책만으로 실제 계정의 인증 성공을 단정하지 않는다. 직접 실행 검증을 못 했으면 사용자 확인이 필요하다고 명시한다.
- STOVE 업로드 기준은 `TunaSweeper/Builds/Stove/<Demo|Full>/Windows/` 전체다. 루트 실행 파일, `Engine/`, `TunaSweeper/`를 포함하며 내부 `TunaSweeper/` 폴더만 업로드하지 않는다. 런처 실행 파일 경로는 이 기준 루트에 대한 상대 경로로 맞춘다.
- 로컬 패키징은 기존 STOVE 설치 빌드를 갱신하지 않는다. 최신 출력 경로와 설치본을 구분해 전달하고, 업로드는 사용자가 요청한 경우에만 진행한다.
- 프로젝트 지침에 따라 빌드 후 해당 프로젝트 에디터를 열거나 기존 인스턴스를 재사용한다. 완료 로그에 타겟·버전·출력 경로·검증 결과·미확인 항목을 남긴다.
