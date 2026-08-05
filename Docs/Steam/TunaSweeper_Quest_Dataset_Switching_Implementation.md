# TunaSweeper 퀘스트 데이터셋 전환 구현

## 문서 상태

이 문서는 초기 단일 플러그인 공개/접근제한 저장소 분리 계획을 TunaSweeper에 실제 적용한 현재 구현 문서다. 일반화된 `Private` 모드 대신 프로젝트에서 확정한 `Public`, `ProductionDemo`, `ProductionRelease` 명칭을 사용한다.

현재 구현 범위:

- 공개 플러그인과 3종 데이터셋 선택 로직
- 별도 `ProductionPayload` Git 저장소
- 프로덕션 데이터 검증·동기화 스크립트
- 공개 저장소 누출 검사
- 데이터셋별 전체 세이브 namespace 분리
- 프로덕션 데모·정식 예시 3연퀘
- `TunaSweeper` 에디터 메뉴 전환 UI
- 플레이·Cook·Build 중 전환 차단과 재시작 안내
- 데이터셋 자동화 테스트

## 데이터셋

| 전환 값 | 데이터셋 ID | 세이브 namespace | 퀘스트 원본 |
|---|---|---|---|
| `Public` | `public` | 기존 공개 슬롯 | 공개 저장소 `Content/Data` |
| `ProductionDemo` | `production_demo` | `ProductionDemo` | 별도 `ProductionPayload` 저장소 |
| `ProductionRelease` | `production_release` | `ProductionRelease` | 별도 `ProductionPayload` 저장소 |

저장소 접근 권한과 런타임 데이터셋 이름은 서로 다른 개념이다. 별도 저장소는 접근 제한으로 운영하지만 코드·스크립트·로그·세이브에서는 `Production` 명칭만 사용한다.

## 공개 저장소 구현

```text
TunaSweeper/Plugins/QuestDatasetSwitcher/
├─ QuestDatasetSwitcher.uplugin
├─ README.md
├─ Scripts/
│  ├─ SwitchQuestDataset.ps1
│  └─ VerifyPublicSafety.ps1
└─ Source/
   ├─ QuestDatasetSwitcher/
   │  ├─ QuestDatasetSwitcher.Build.cs
   │  ├─ Public/QuestDatasetSwitcher.h
   │  └─ Private/QuestDatasetSwitcher.cpp
   └─ QuestDatasetSwitcherEditor/
      ├─ QuestDatasetSwitcherEditor.Build.cs
      └─ Private/QuestDatasetSwitcherEditorModule.cpp
```

`FQuestDatasetSwitcherModule`은 `Content/Data/QuestDatasetGenerated/active-dataset.json`을 읽는다. marker가 없으면 `Public`을 선택한다. marker가 유효하면 `ProductionDemo` 또는 `ProductionRelease`를 선택하고 해당 생성 데이터 경로를 퀘스트 서브시스템에 제공한다. 지원하지 않는 ID, 잘못된 marker, 누락된 JSON/CSV는 프로덕션 데이터로 인정하지 않고 공개 데이터로 안전하게 fallback한다.

`UTunaSweeperQuestSubsystem`은 더 이상 퀘스트 파일 경로를 직접 고정하지 않는다. 플러그인의 활성 descriptor에서 다음 경로를 받는다.

- `QuestDefinitionsPath`
- `QuestTextStringsPath`

## ProductionPayload 저장소

별도 저장소는 공개 플러그인 안의 ignored 경로에 clone한다.

```powershell
git clone <PRODUCTION_REPOSITORY_URL> `
  TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload
```

구조:

```text
ProductionPayload/
├─ payload-manifest.json
└─ Datasets/
   ├─ ProductionDemo/
   │  ├─ dataset-manifest.json
   │  ├─ QuestDefinitions.json
   │  └─ QuestTextOverrides.csv
   └─ ProductionRelease/
      ├─ dataset-manifest.json
      ├─ QuestDefinitions.json
      └─ QuestTextOverrides.csv
```

각 예시 데이터셋에는 기존 취수 시설 상호작용 이벤트를 사용하는 직렬 3연퀘가 들어 있다. 선행 관계는 `Q1 → Q2 → Q3`이며 프로덕션 데모와 정식은 서로 다른 퀘스트 ID와 세이브 namespace를 사용한다.

`QuestTextOverrides.csv`는 동기화할 때 공개 `QuestTextStrings.csv` 위에 key 기준으로 병합된다. 공통 UI 문자열은 공개 저장소에서 계속 관리하고 프로덕션 전용 퀘스트 문자열만 별도 저장소가 소유한다.

## 에디터 메뉴 전환

Unreal Editor의 최상위 `TunaSweeper` 메뉴에서 `Data Tools` 섹션의 `Quest Dataset Switcher`를 연다. 패널은 에디터가 현재 메모리에 로드한 데이터셋과 디스크에 적용된 데이터셋을 따로 표시한다.

1. `공개`, `프로덕션 데모`, `프로덕션 정식` 중 하나를 선택한다.
2. 적용 버튼 옆의 세이브 분리 및 재시작 주의사항을 확인한다.
3. `적용`을 누르고 확인 대화상자를 승인한다.
4. 디스크 적용 상태에 `에디터 재시작 필요`가 표시되면 즉시 Unreal Editor를 종료하고 다시 실행한다.

UI 적용도 아래 PowerShell 스크립트를 실행하므로 manifest·퀘스트·문자열 검증 규칙은 명령줄과 동일하다. 적용 직후 현재 세션의 런타임 데이터셋을 강제로 reload하지 않는다. 따라서 데이터와 세이브 namespace는 다음 에디터 시작부터 함께 전환된다.

다음 상태에서는 적용 버튼이 비활성화되며 상태별 차단 사유가 버튼 옆에 표시된다.

- PIE, Simulate, Standalone 실행 또는 실행 예약
- Cook 또는 Package
- 에디터 Build
- C++ Hot Reload
- Live Coding 컴파일

디스크 marker가 손상돼 상태를 판독할 수 없을 때에는 프로덕션 적용을 막고 `공개` 적용만 허용해 생성 폴더를 제거하고 복구할 수 있다.

## 명령줄 전환

자동화 또는 복구 작업에서는 Unreal Editor를 종료한 상태로 공개 저장소 루트에서 실행한다.

```powershell
# Public
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset Public

# Production demo
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionDemo

# Production release
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionRelease
```

프로덕션 전환 시 스크립트는 다음을 수행한다.

1. payload와 dataset manifest의 플러그인·데이터셋 ID를 검증한다.
2. 퀘스트 ID 중복과 선행 퀘스트 참조를 검증한다.
3. 공개 공통 문자열과 프로덕션 문자열 override를 병합한다.
4. 선택한 데이터셋 하나만 `Content/Data/QuestDatasetGenerated/`에 생성한다.
5. 데이터 revision, 세이브 호환성 ID, production Git revision을 marker에 기록한다.

`Public` 전환은 생성된 프로덕션 디렉터리만 삭제하며 `ProductionPayload` 원본 저장소는 보존한다. 전환 뒤에는 Live Coding이나 Hot Reload에 의존하지 말고 에디터를 다시 시작한다.

변경 없이 검증만 하려면 `-VerifyOnly`를 사용한다.

```powershell
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset ProductionDemo -VerifyOnly
```

## 세이브 분리

데이터셋 전환은 퀘스트 진행도만 바꾸지 않고 전체 세이브 프로필을 전환한다. 퀘스트 보상으로 얻은 아이템·코인·시설·제작법·월드 진행도 역시 데이터셋 사이에 섞이지 않는다.

공개 세이브는 기존 사용자의 진행도를 보존하기 위해 파일명을 바꾸지 않는다.

```text
TunaSweeperSave_Slot01
TunaSweeperSave_Slot02
TunaSweeperSave_Slot03
```

프로덕션 세이브:

```text
TunaSweeperSave_ProductionDemo_Slot01
TunaSweeperSave_ProductionRelease_Slot01
```

최근 선택 슬롯 설정과 백업 디렉터리도 데이터셋별로 분리된다. 세이브 버전 19부터 다음 metadata를 기록한다.

- `DatasetId`
- `DatasetRevision`
- `SaveCompatibilityId`

버전 19 이전에 생성되어 `DatasetId`가 없는 기존 세이브는 `Public` 소속으로만 인정한다. 파일 내부 ID가 활성 데이터셋과 일치하지 않으면 로드와 덮어쓰기를 거부한다.

일반적인 호환 데이터 수정은 `dataset_revision`만 올린다. 기존 진행도를 의도적으로 사용할 수 없는 변경일 때만 `save_compatibility_id`를 바꾼다.

## 공개 저장소 안전 규칙

공개 `.gitignore`는 다음 경로를 제외한다.

```text
TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload/
TunaSweeper/Content/Data/QuestDatasetGenerated/
```

공개 commit 또는 push 전:

```powershell
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\SwitchQuestDataset.ps1 `
  -Dataset Public
.\TunaSweeper\Plugins\QuestDatasetSwitcher\Scripts\VerifyPublicSafety.ps1
git status --short
```

검사 스크립트는 원본 payload와 생성 데이터가 공개 Git에서 추적되거나 staged되지 않았는지, 필수 ignore 규칙이 실제로 동작하는지 확인한다.

공개 패키지는 프로덕션 파일이 한 번도 존재하지 않았던 깨끗한 workspace에서 만들어야 한다. `git add -f`로 ignored 경로를 추가하지 않는다.

## 별도 저장소 작업 순서

프로덕션 데이터는 nested 저장소에서 독립적으로 commit한다.

```powershell
git -C TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload remote add origin `
  <PRODUCTION_REPOSITORY_URL>
git -C TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload status
git -C TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload add Datasets
git -C TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload commit -m "Update production quests"
git -C TunaSweeper/Plugins/QuestDatasetSwitcher/ProductionPayload push -u origin main
```

프로덕션 동기화와 빌드를 검증한 공개 commit hash와 production commit hash를 함께 릴리스 기록에 남긴다. 권장 태그 쌍은 `public/<version>`과 `production/<version>`이다.

`ProductionPayload`의 원격 URL, 브랜치 추적 상태, commit hash 같은 운영 정보는 공개 구현 문서에 기록하지 않고 접근 권한이 있는 작업자만 별도로 관리한다.

## 검증

구현에 포함된 자동화 테스트:

- `TunaSweeper.QuestDataset.Namespaces`
- `TunaSweeper.QuestDataset.ActiveData`

`ActiveData`는 활성 파일 존재와 JSON 파싱, 프로덕션 예시 퀘스트 수 3개, 중복 ID 없음, 두 개의 유효한 선행 간선으로 구성된 단일 체인을 검사한다.

구현 과정에서 다음 스크립트 검증을 통과했다.

- `ProductionDemo -VerifyOnly`
- `ProductionRelease -VerifyOnly`
- `Public` 전환 및 public verify
- `ProductionDemo`와 `ProductionRelease` 실제 materialization
- 공개 저장소 누출 검사

2026-08-04 최종 검증 결과:

- UE 5.7 `TunaSweeperEditor Win64 Development -NoHotReload` 전체 빌드 성공
- `QuestDatasetSwitcher`, `QuestDatasetSwitcherEditor`, `TunaSweeper`, `TunaSweeperEditor` DLL 링크 성공
- `ProductionDemo`: 자동화 테스트 2개 성공
- `ProductionRelease`: 자동화 테스트 2개 성공
- `Public`: 자동화 테스트 2개 성공
- 공개 데이터셋 상태에서 Win64 Development `BuildCookRun`의 Build·Cook·Stage·Pak 성공
- 최종 활성 상태를 `Public`로 복귀하고 `QuestDatasetGenerated/` 제거 확인
- `git diff --check` 통과
