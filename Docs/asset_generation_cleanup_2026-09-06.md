# 애셋 생성 코드 조사 및 정리 — 2026-09-06

## 판단

기존 애셋의 일회성 생성 코드를 제거해도 된다. 조사에서 식별한 BP/WBP/ABP 이름은 모두 Git에 저장된 애셋과 대응했고, 제거한 C++ 파일에는 애셋 부모가 되는 UCLASS/USTRUCT/UENUM 정의가 없다. 런타임 모듈·BP 부모 클래스·콘텐츠 파일·기본 GameInstance/GameMode 설정은 유지했다.

생성 코드는 저장된 애셋을 만들거나 다시 편집·저장하는 에디터 도구였다. 앞으로 BP 구성·기본값·UMG 트리·머티리얼 그래프는 저장된 애셋에서 편집한다. 필요해진 생성기는 해당 작업에 맞춰 다시 작성하거나 Git 이력을 참고한다.

## 불필요한 변경의 원인

1. 주요 경로는 UCommandlet 파생 클래스가 아니라 `TunaSweeperEditor::StartupModule`과 `OnEditorInitialized`/ticker였다. 실제 commandlet에서는 주 에디터 모듈이 조기 반환했지만 일반 에디터 시작에서는 RunOnce 작업과 맵 보정 예약이 실행됐다.
2. `FTunaSweeperEditorRunOnce`는 `GEditorPerProjectIni`의 `TunaSweeperEditor.RunOnce/CompletedTasks`에만 완료 목록을 기록했다. 새 PC, Saved/Config 초기화, TaskId 변경 시 이미 Git에 있는 애셋도 작업 대상이 될 수 있었다.
3. 여러 Ensure 함수는 기존 애셋을 로드한 뒤에도 Modify, Blueprint compile, MarkPackageDirty, SavePackage를 호출했다. Ensure라는 이름이 무변경 실행을 보장하지 않았다. 머티리얼 그래프·UMG 트리 재작성, GUID/컴파일 데이터·임포트 메타데이터 변경이 바이너리 diff를 만들 수 있는 구조였다.
4. StylizedWater, SplineWorldBuilder, RegionalGroundFog, TunaWarpTransition에도 시작 시 생성 경로가 있었다. 물·스플라인·워프는 버전/누락 판정에 따른 갱신, 안개는 누락 파일 생성이었다. 안개가 매번 기존 파일을 덮어썼다는 뜻은 아니다.

재저장 가능 경로를 확인했다. 개별 과거 커밋의 모든 바이너리 변경이 이 경로 때문이었다고 단정하지는 않는다.

## 제거·유지 판단

| 구분 | 대표 저장 애셋·내용 | 처리 |
|---|---|---|
| 기본 게임·입력 | GameInstance/GameMode, 플레이어·적 BP, IA/IMC | 생성·입력 매핑 보정·RunOnce 제거 |
| UI | 인벤토리/HUD/퀘스트/상호작용 WBP, 타이틀·설정 화면 WBP | UMG 트리 생성·스타일 재작성 제거 |
| 월드·상호작용 | 전리품·스포너·연구대·빠루 거치대·이동·워프·맵 캡처 BP | 생성·기존 맵 자동 배치 제거 |
| 무기·이펙트 | 무기 연출/반동/발소리 DataAsset, 탄피·충돌·연막·Niagara, 파괴 토마토/사과상자 | 초기 데이터·메시·재질·이펙트 생성 제거 |
| 환경 | 차고문, 끓는 냄비, 벽 코핑, 실험 식생, 웅덩이 반사, 절차 지형 테스트 | 애셋과 맵 유지, 전용 생성기 제거 |
| 캐릭터·물리 | Luna 치마/사이드테일 Physics/ABP, 사족 보행 프리셋, 사고 표시, 로봇 디졸브 | 일회성 설정 제거, 런타임 유지 |
| StylizedWater | 내부 BP 1개, T 1개, M 2개, MI 2개 | 자동 생성 제거, 물 추가 메뉴는 저장된 6개 애셋 로드 |
| SplineWorldBuilder | 테스트 메시 5개, T/M 각 1개, Profile 1개 | 생성·Rebuild 메뉴 제거, 테스트 배치는 저장 Profile 사용 |
| RegionalGroundFog | Density T, FogCard M | 자동 생성 제거, 볼륨 시각화 유지 |
| TunaWarpTransition | 기본 Profile, M 2개, MI 2개 | 생성 코드 제거, 모듈 이름·플러그인 설정 유지 |
| Python 일회성 생성 | GazeTestRobot BP/메시/재질·맵 배치, 사족 로봇 4색 BP/MI, 퀘스트 표식 M 2개 | 완성 애셋의 생성 스크립트 3개 제거 |

대형 SetupShared 헤더, TaskId, 생성 전용 `.inl`, 전용 명령행·콘솔 연결도 함께 제거했다. 기존 타이틀 테스트는 구체적인 런타임/UMG 헤더를 직접 포함하도록 정리했다. 생성 전용 빌드 의존성을 제거하고 도구·테스트용 의존성은 유지했다.

## 유지한 기능과 경계

- UI 텍스처·오디오 명시적 임포트는 `TunaSweeperEditorAssetImport.cpp/.h`로 분리했다. 기존 옵션명·기본 임포트 동작은 유지한다.
- 메모 장치·롤링 폭탄 스포너·모래주머니 전용 텍스처 생성/보정 옵션은 전용 생성기와 함께 제거했다. 기존 텍스처는 에디터에서 재임포트한다.
- 맵 캡처와 MapRegistry 갱신, FM 사운드 제작/임포트, SOG·FBX 등의 반복 임포트, BushMeshBuilder, ChainPhysicsEditor는 명시적 편집 도구로 유지한다.
- 물·스플라인·벽 코핑·안개가 현재 레벨에서 만드는 절차 메시/컴포넌트와 깊이 베이크는 레벨 편집·게임 기능이므로 유지한다.
- 저장된 애셋이 누락되면 물/스플라인 배치 도구가 재생성하지 않는다. Git/LFS 원본을 복구해야 한다.
- SourceArt·원본 모델·이미지·제작 파라미터는 삭제하지 않았다. 파라미터 파일은 제거된 생성기와 더 이상 자동 동기화되지 않는다.
- 게임 세이브 구조·퀘스트 저작 데이터는 변경하지 않았다.

## 조사 목록과 재현 기준

[소스별 애셋 참조 목록](asset_generation_inventory_2026-09-06.csv)은 정리 직전 소스의 문자열·공용 상수를 Git 추적 콘텐츠 파일명에 대조한 목록이다. 생성 출력뿐 아니라 입력·템플릿·의존 애셋도 포함한다. 여러 소스가 같은 애셋을 참조하며 문자열 조합 경로를 모두 추론한 완전한 출력 목록은 아니다.

초기 C++/임포트 소스 대조에서 중복을 제외한 참조 애셋 329개를 확인했다. 별도 Python 생성기의 로봇·퀘스트 표식 애셋을 포함하여 총 352개를 로드 검사했다. 정리 전 공개 Git 추적 콘텐츠 패키지 906개의 SHA-256을 기록했다.

과거 생성 코드는 정리 전 커밋 `3803c3d0`과 각 파일의 Git 이력에서 조회할 수 있다. 생성기를 다시 실행하기 전에 현재 애셋의 수동 편집 내용을 확인해야 한다.

## 검증

- UE 5.7 `TunaSweeperEditor Win64 Development` 빌드 통과. 의존성 정리 중 확인된 도구·테스트용 링크 의존성은 복구한 뒤 재빌드했다.
- 일반 에디터의 `-ExecutePythonScript`로 애셋 352개 로드 성공, 그중 BP/WBP/ABP 81개 generated class 확인, 실패 0개. 프로세스 종료 코드 0.
- 로드 검사는 콘텐츠 생성·수정·컴파일·저장 API를 호출하지 않는다. 검사 결과와 로그는 `TunaSweeper/Saved/AssetGenerationAudit`에 남긴다.
- 정리 전후 공개 Git 추적 콘텐츠 906개의 SHA-256 일치: 변경 0개. 일반 에디터 시작과 검증 실행 후에도 동일했다.
- 생성 전용 기존 파일 44개(C++/헤더/inl 41개, Python 3개)를 제거하고 재사용 임포트 2개 소스 파일을 분리했다. 플러그인/시작 코드 정리를 합쳐 소스 약 2.9만 줄을 줄였다.
- 자동화 6개 중 5개 통과: 스플라인 애셋·배치, 벽 코핑 배치, 물 애셋·바디, 타이틀 화면·전환. 타이틀 검사에는 기존 체크 기호 폰트 fallback 경고가 있었다.
- `SplineWorldBuilder.WallCoping.GeneratedAssets` 1개는 기존 저장 치수 불일치로 실패했다. 테스트는 50×38×14cm를 기대하지만 현재 저장된 메시와 박스 충돌은 모두 약 48.920246×46.041420×14cm다. 해당 애셋 해시는 정리 전후 동일하다. 길이/폭의 하드코딩 기대값 4개가 실패했고 벽 코핑 배치 검사는 통과했다. 애셋·기존 테스트를 임의로 수정하거나 재생성하지 않았다.
- 자동화 프로세스는 종료 코드 0을 반환했으나 JSON 결과의 실패 1개를 기준으로 위와 같이 기록했다. D3D 검증 시작에는 기존 Niagara 초기화 ensure도 기록되었다.
- 검증한 프로젝트 `TunaSweeper/TunaSweeper.uproject`의 일반 에디터를 실행했고 초기 맵·Asset Registry 로드를 확인했다. 전체 게임 플레이·쿠킹/패키징 검증은 수행하지 않았다.
- 삭제된 생성 함수·TaskId·BlueprintFactory·재생성 플래그의 실행 코드 잔여 참조 및 변경 파일 whitespace 검사를 완료했다.
