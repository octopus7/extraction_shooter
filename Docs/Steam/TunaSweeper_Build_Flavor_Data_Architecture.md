# TunaSweeper Demo/Main 빌드 데이터 구조

## 기준

수동 데이터셋 선택 상태는 사용하지 않는다. 패키징 타깃의 `TUNASWEEPER_DEMO` 정의와 에디터의 Build Target 설정이 실행 중인 빌드 성격을 결정하는 유일한 기준이다.

- Demo 타깃: 공개 작업 저장소의 데이터와 Demo 새 게임 진입 흐름을 사용한다.
- Main 타깃: 접근 제한 `MainPayload` 원본과 Main 새 게임 진입 흐름을 사용한다.

`Public`, `Production`, marker, 생성 데이터셋 전환 메뉴는 존재하지 않는다.

## 데이터 경로

Demo 퀘스트 데이터는 공개 프로젝트 파일을 직접 읽는다.

- `TunaSweeper/Content/Data/QuestDefinitions.json`
- `TunaSweeper/Content/Data/QuestTextStrings.csv`

Main 원본은 프로젝트 내부이지만 Unreal 자동 탐색 경로 밖에 둔 독립 Git 저장소에서 관리한다.

- 로컬 루트: `TunaSweeper/External/MainPayload/`
- manifest: `main-payload.json`
- 퀘스트 정의: `Data/QuestDefinitions.json`
- Main 전용 문자열: `Data/QuestTextStrings.csv`

Editor/PIE의 Main 타깃은 원본 일반 JSON/CSV를 직접 읽는다. Main 패키징 직전에는 `BuildScripts/BuildFlavorData.ps1`이 필수 파일과 형식을 검증하고 `Content/Data/MainPayloadStaged/`에 필요한 파일만 새로 복사한다. 패키징이 끝나면 임시 폴더를 제거한다.

Demo 패키징 직전에는 Main 임시 폴더를 제거하고, 패키징 후 결과 경로에서 Main manifest 또는 staging 경로가 발견되면 실패시킨다. 접근 제한 원본 저장소는 Content, Plugins, Source 밖에 있으므로 Cook 대상이 아니다.

공개 문자열과 Main 전용 문자열은 서로 다른 키만 가질 수 있다. 중복 키를 override로 해석하지 않으며 런타임에서 오류로 처리한다.

## 새 게임과 타이틀

난이도 확정 후 Demo는 기존 오프닝 흐름으로, Main은 `main-payload.json`의 `initial_level`로 진입한다. Main manifest를 읽을 수 없거나 값이 비어 있으면 `BunkerMap`을 안전한 기본값으로 쓴다.

Demo 타깃의 타이틀 화면에는 `DemoBuildImage`라는 별도 이미지 위젯을 노출한다. Main 타깃에서는 같은 위젯을 접는다. 이 판정도 BuildFlavor에서 직접 가져오며 데이터 문자열 키와 관계없다.

## 세이브 격리

슬롯명은 양쪽에서 동일하지만 파일 루트가 물리적으로 분리된다.

- Demo: `Saved/SaveGames/Demo/`
- Main: `Saved/SaveGames/Main/`

각 루트는 슬롯, 최근 선택 슬롯 설정, `Backups/`, `AutoDeletedSaveLog.txt`를 독립적으로 가진다. 세이브 객체에도 `BuildFlavor`를 기록하고 현재 타깃과 다르면 로드와 덮어쓰기를 거부한다.

버전 20 미만 세이브 정리는 현재 타깃 루트 안에서만 재귀적으로 수행한다. 과거 `Saved/SaveGames/` 바로 아래의 평면 `.sav` 파일은 이관하지 않고 시작 시 삭제하며, 현재 타깃의 자동 삭제 로그에 파일명을 남긴다.

## 패키징 명령

직접 확인할 때는 다음 스크립트를 사용한다.

```powershell
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode PrepareMain
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode Clean
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode PrepareDemo
.\TunaSweeper\BuildScripts\BuildFlavorData.ps1 -Mode VerifyDemo -ArchiveDirectory <DemoArchivePath>
```

Steam/STOVE 패키징 배치와 에디터 Build Target 패키징 도구는 이 절차를 자동 호출한다.
