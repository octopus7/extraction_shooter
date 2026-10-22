# 리소스 경로 재배치 제안

- 문서 작성일: 2026-10-23
- 조사 기준일: 2026-10-22
- 대상: TunaSweeper UE 5.7 프로젝트의 공개 리소스, 제작 원본, 관련 코드·설정·도구
- 상태: **재배치 제안. 이 문서 작성 과정에서 실제 파일 이동이나 게임 코드·에셋 변경은 하지 않았다.**

## 1. 추천 방향

런타임 에셋은 **소유 기능별로 모으고**, 제작 원본은 기존 **`TunaSweeper/SourceArt`로 통일**한다.

현재 구조에는 파일 종류(`Meshes`, `Materials`), 기능(`Characters`, `Interaction`), 제작 도구·서비스(`Blender`, `chatgpt`, `gemini`), 임시 작업명(`AXTemp`, `LunaFacialTemp`)이 같은 단계에 섞여 있다. 같은 기능을 수정하려고 여러 폴더를 찾아다녀야 하고 원본·임포트 파일·미리보기를 구분하기 어렵다.

분류 순서는 **소유 기능 → 파일 역할 또는 종류**로 정한다. ATV 전용 소리와 파괴 효과는 ATV 폴더에 두고, 여러 기능이 실제로 공유하는 소리·효과만 공용 `Audio`, `Effects`에 둔다.

기존 `Core`, `Input`, `Characters`, `Environment`, `Interaction`, `Weapons`, `UI` 등의 의미 있는 구조는 활용한다. 전체를 새로운 `Assets` 또는 `Resources` 루트로 옮기거나, 모든 파일을 확장자별 폴더로 다시 나누는 방식은 권장하지 않는다.

## 2. 조사 범위와 현재 상태

공개 저장소 전체 파일 목록, Content 전체 경로, 제작 원본·미디어 경로와 코드·설정·JSON·임포트 도구의 경로 참조를 조사했다. 아래 수치는 2026-10-22 조사 시점의 스냅샷이며 이후 추가되는 작업은 별도로 반영해야 한다.

| 항목 | 조사 결과 |
| --- | ---: |
| `TunaSweeper/Content` 최상위 폴더 | 37개 |
| Content 전체 파일 | 2,032개 |
| `.uasset` | 1,909개 |
| `.umap` | 33개 |
| 기타 파일 | 90개 |
| 별도로 조사한 제작 원본·미디어 경로 | 11개 |
| `TunaSweeper/SourceArt` Git 추적 파일 | 1,557개 |

Content 집계는 `rg --files --hidden --no-ignore TunaSweeper/Content`의 전체 파일 목록을 기준으로 했다. 11개 제작 원본·미디어 경로는 `Art`, `ArtSource`, `Blender`, `GeneratedImages`, `Images`, `Sounds`, `Videos`, `chatgpt`, `gemini`, `spz`, `TunaSweeper/SourceArt`다. 이 외에도 저장소 루트의 이미지, `Content/SourceArt`, `Tools/SourceAssets`에서 원본이 확인되었다.

UE 바이너리 내부 참조, Asset Registry 전체 의존성, UE 재임포트 메타데이터, Blender 내부 외부 파일 참조를 전수 검사한 결과는 아니다. 에셋의 시각적 품질이나 미사용 여부도 이 조사만으로 확정하지 않았다.

### 대표적인 분산 사례

- ATV: BP, 메시, 사운드, 이펙트가 `Blueprints`, `Meshes`, `Audio`, `Effects`로 분산되어 있다.
- 두더지: BP는 `Characters/Mole`, 모델·애니메이션은 `Characters/NPC/Mole`에 있다.
- 자연물: `Nature`, `Environment/AnimeTree`, `Environment/AnimeBush`, `Environment/AquaticPlants` 등에 나뉘어 있다.
- 동일 분류: `Interaction/QuestFacility`와 `Interaction/QuestFacilities`, `Effects`와 `FX`가 공존한다.
- 제작 원본: `Art`, `ArtSource`, `Blender`, `GeneratedImages`, `TunaSweeper/SourceArt` 등이 각각 원본 보관소 역할을 한다.
- 원본과 런타임 혼재: PNG 원본이 `Content/SourceArt`에 있고, 도구용 CSV 내보내기 파일이 `Content/Data/ExcelExport`에 있다.

## 3. 런타임 에셋 재배치안

아래 경로는 UE 콘텐츠 경로다. `/Game`은 `TunaSweeper/Content`에 대응한다. 제안 경로는 목표 위치이며 실제 이동 목록은 참조 조사 후 에셋 단위로 확정한다.

| 대상 | 현재 위치 | 제안 위치 |
| --- | --- | --- |
| 두더지 | `/Game/Characters/Mole` + `/Game/Characters/NPC/Mole` | `/Game/Characters/NPC/Mole` |
| 사족 로봇 적 | `/Game/Blueprints`의 적 BP·변형 + `/Game/Characters/Robot` | `/Game/Characters/Enemies/Quadruped` |
| ATV | `/Game/Blueprints/Vehicles/ATV`, `/Game/Meshes/Props/ATV`, `/Game/Audio/ATV`, `/Game/Effects/ATV` | `/Game/Vehicles/ATV` |
| 취수시설 | `/Game/Interaction/BP_WaterIntake`, `/Game/Meshes/Props/WaterIntake*`, `/Game/Environment/Water` | `/Game/Interaction/WaterIntake` |
| 퀘스트 시설 | `/Game/Interaction/QuestFacility` + `/Game/Interaction/QuestFacilities` | `/Game/Interaction/QuestFacilities` |
| 자연물 | `/Game/Nature` + 환경 폴더의 나무·덤불·수생식물 | `/Game/Environment/Nature/<종류>` |
| 무기 | `/Game/Weapons`, `/Game/Meshes/Weapons`, 무기 전용 재질 | `/Game/Weapons/<무기 또는 제품군>` |
| 공용 이펙트 | `/Game/Effects` + `/Game/FX` | `/Game/Effects/<효과 종류>` |
| UI 위젯 | `/Game/UI` 루트의 `WBP_*`, `/Game/Blueprints/UI` | `/Game/UI/HUD`, `Inventory`, `Quests`, `Indicators` 등 |
| 플레이 맵 | `/Game/Maps`, `/Game/MainRaid`, 일부 환경 폴더 | 장기적으로 `/Game/Maps/<용도 또는 지역>` |

`Environment/Water`에는 일반 물 표현이 아니라 취수시설 스크린의 메시·재질·텍스처가 있다. `Shader_Water`, `Environment/TestWater`, `Prototype/Water`, `Environment/ShallowPuddle`, `FX/Waterfall` 역시 같은 기능이라고 단정하지 않고 공용 물 표현·시설·실험용 자산을 구분한다.

기능 폴더 내부는 다음처럼 구성할 수 있다. 파일이 적으면 불필요한 하위 폴더를 만들지 않는다.

```text
/Game/Vehicles/ATV/
  Blueprints/
  Meshes/
  Materials/
  Textures/
  Animations/
  Audio/
  Effects/
```

공용 리소스를 기능 폴더로 복제하지 않는다. 전용 리소스만 해당 기능에 모으고, 실제로 공유하는 리소스는 공용 위치에 두어 참조한다. `Meshes`, `Materials`, `Blueprints`에 남은 파일은 소유 기능을 확인해 배치하되, 공용 여부가 불명확한 파일은 먼저 조사한다.

## 4. 제작 원본 통합안

기존 `TunaSweeper/SourceArt`가 가장 큰 원본 보관소이고 관련 임포트 도구와 문서도 있으므로 기준 경로로 유지한다. 별도의 새 원본 루트를 추가하지 않는다.

| 현재 분류 | 통합 방향 |
| --- | --- |
| `Art`, `ArtSource`, `Blender` | 캐릭터·환경·무기 등 실제 대상별 `TunaSweeper/SourceArt` 원본 폴더 |
| `GeneratedImages`, `Images`, 저장소 루트의 아이콘 PNG | `SourceArt/UI` 또는 해당 에셋의 텍스처·참고 자료 |
| `chatgpt`, `gemini` | UI·음악·영상 등 결과물의 용도별 폴더 |
| 분산된 `ForestProps` | `SourceArt/Environment/Nature/ForestProps` |
| `SourceArt/Effects` + `SourceArt/FX` | `SourceArt/Effects`를 기준으로 정리 |
| `Sounds`, 게임에 쓰이는 영상 원본 | `SourceArt/Audio`, `SourceArt/Cinematics`의 해당 대상별 폴더 |
| `Tools/SourceAssets`의 모델·이미지 | 해당 에셋 원본 폴더. 재사용 임포트 코드는 `Tools`에 유지 |
| `Content/SourceArt`의 원본 PNG | 해당 `TunaSweeper/SourceArt` 폴더 |

게임 원본, 홍보용 미디어, 도구 의존성은 역할이 다르다. 홍보 전용 파일은 용도를 확인해 별도 관리하고, `Videos/ffmpeg`처럼 로컬 실행 도구인 파일을 게임 원본으로 이관하지 않는다. 무시된 백업·후보·캐시도 추적 중인 원본과 구분한다.

제작 도구·생성 서비스 이름은 최상위 분류 대신 README나 기존 제작 기록에 남긴다. 생성 프롬프트, 출처, 버전, 제공된 라이선스 등은 에셋과 함께 보존한다.

```text
TunaSweeper/SourceArt/Vehicles/ATV/
  Source/       # 편집 가능한 원본
  Exports/      # UE에 임포트하는 FBX 등
  Textures/     # 원본 텍스처
  References/   # 컨셉·참고 이미지·생성 기록
  Previews/     # Blender 및 UE 미리보기
  Validation/   # 검증 결과
  README.md
```

원본과 임포트용 출력, 컨셉과 실제 게임 캡처의 역할을 명확히 기록한다. 기존 `Models`, `Source`, `Renders`, `Previews` 등은 이 기준에 맞춰 정리하되 빈 폴더를 일괄 생성하지 않는다.

원본 파일을 옮길 때는 임포트 스크립트, UE 임포트 메타데이터, Blender의 외부 텍스처 경로, 문서 링크도 함께 갱신한다. 원본 통합을 위해 제거된 일회성 생성기나 에디터 시작 시 재생성 경로를 복구하지 않는다.

## 5. 유지하거나 별도로 판단할 경계

### 5.1 역할이 명확한 기존 경로

`Core`, `Input`, `UI/Icons`, `Slate/Fonts`는 우선 유지한다. 특히 아이콘과 일부 효과는 이름으로 경로를 조합하고, 폰트는 파일 시스템 경로로 읽으므로 문자열 검색 결과만으로 이동 영향을 모두 판단할 수 없다.

### 5.2 플러그인과 외부 팩

플러그인 고유 콘텐츠는 해당 플러그인 마운트에 유지한다. 프로젝트 전용 외형·설정과 플러그인 내부 기본 리소스를 구분한다.

`BallisticsVFX`, `NiagaraExamples` 등 팩 형태의 자산은 공급처·라이선스·업데이트 방식을 확인하기 전까지 기존 경로를 유지한다. 폴더 이름만으로 외부 자산이라고 확정하거나 일괄 이동하지 않는다.

### 5.3 임시 이름과 동일 파일명

`Prototype`, `LunaFacialTemp`, `AXTemp`, `untitled_category`는 사용처 확인 대상이다. `Prototype`에는 실제 적 캐릭터와 효과가 사용하는 런타임 재질이 있고 AlwaysCook 설정도 있다.

개발 전용임이 확인된 자료만 `EditorOnly` 등 적절한 위치로 분리한다. 이름에 Temp 또는 Prototype이 있다는 이유로 이동·삭제·쿠킹 제외를 하지 않는다. 같은 파일명 역시 동일 내용이나 동일 용도를 보장하지 않는다.

### 5.4 데이터와 지역화

런타임 JSON·문자열 CSV는 프로젝트 로컬 원본을 기준으로 유지한다. 다음 파일은 런타임 데이터와 분리할 후보이며 관련 도구의 읽기·쓰기 경로를 함께 수정해야 한다.

- `Content/Data/ExcelExport`: 도구용 CSV 교환·내보내기 파일.
- `Content/Data/FMSoundPresets.json`: FM 사운드 편집 도구 설정.

현재 설정은 `Content/Data` 전체를 NonUFS 스테이징 대상으로 지정한다. 실제 배포 포함 여부·용량은 추후 스테이징 목록에서 확인한다.

문자열 CSV와 같은 이름의 `Translations` 보조 CSV는 하나의 묶음으로 관리한다. 문자열 키와 기존 로딩 규약을 보존하며 중복 파일로 취급하지 않는다. 이번 정리를 이유로 퀘스트 데이터를 자동 동기화하거나 가져오기·내보내기하지 않는다.

접근 제한된 `ProductionPayload` 저장소와 내러티브 데이터셋 경계는 유지한다. 공개 저장소로 원본 팩을 복사하거나 경로 정리와 함께 데이터셋 정책을 바꾸지 않는다.

### 5.5 맵과 배포 설정

맵 이동은 마지막에 수행한다. `MainRaid`에는 맵뿐 아니라 이동 시설·소품·프리뷰 등이 있으며, 현재 데모 설정은 `/Game/MainRaid`를 NeverCook 경로로 사용한다. 이 경로 밖으로 에셋을 이동하면 배포 포함 범위를 함께 확인해야 한다.

먼저 맵별 용도, 실제 패키지 경로, 데모/본편 사용 범위, 쿠킹 포함 여부를 목록화한다. 이후 기본·타겟별 설정, 런타임 로더, 에디터 도구, JSON, 검증 스크립트를 함께 수정한다.

맵이나 액터를 재생성하는 방식으로 정리하지 않는다. 저장 데이터와 연결되는 액터 이름, PlacementId, 게임플레이 식별자를 보존한다. 내부 `Bunker` 이름 변경은 이번 경로 정리와 묶지 않으며, 플레이어에게 보이는 한국어는 기존 규칙대로 `기지`를 사용한다.

## 6. 권장 실행 순서

| 단계 | 작업 | 확인 사항 |
| --- | --- | --- |
| 1 | 이전 경로 → 새 경로 목록 작성 | 소유 기능, 공유 여부, 참조, 원본·런타임 구분 |
| 2 | 두더지·퀘스트 시설 등 작은 기능 통합 | BP·모델·재질 참조와 관련 기능 로드 |
| 3 | ATV·취수시설 등 분산 기능 통합 | 코드·데이터 경로, 오디오·효과, 실제 기능 |
| 4 | 제작 원본 통합 | 임포트 도구, 재임포트, Blender 외부 파일, 문서 링크 |
| 5 | 자연물·공용 재질·이펙트 정리 | 공용 참조 보존, 임시 자산의 실제 사용 여부 |
| 6 | 맵·쿠킹 경계와 개발 전용 자산 정리 | 맵 로드·전환, 타겟별 포함 범위, 저장 식별자 |

한 번에 전체를 옮기지 않고 기능 단위로 이동·검증·커밋한다. 각 단위의 경로 변경, 관련 코드·설정 변경, 필요한 임시 도구 정리, 검증 기록을 함께 완료한다.

UE 에셋은 에디터의 에셋 이동 기능 또는 대응 에셋 API로 처리하고 참조 패키지를 재저장한 뒤 리다이렉터를 정리한다. 파일 탐색기 이동이나 Git 경로 변경만으로 UE 참조 갱신을 대신하지 않는다. C++·JSON·설정·스크립트의 문자열 경로는 별도로 수정한다.

실제 실행 시 에디터를 탐지·열기·닫기 전에 [Unreal Editor coordination](unreal_editor_coordination.md)을 따른다.

## 7. 이동 단위별 검증 기준

- 이전 → 새 경로 목록과 실제 이동 파일이 일치한다.
- 관련 에셋과 맵을 다시 로드하고 누락 참조·리다이렉터 상태를 확인한다.
- 직접 경로 문자열과 이름으로 조합하는 경로를 모두 확인한다.
- 관련 BP, 런타임 기능, 오디오·효과·UI 표시를 확인한다.
- 원본을 옮겼다면 실제 재임포트와 Blender 외부 파일 로드를 확인한다.
- 맵 관련 변경은 저장 식별자와 이동 전후 배치를 비교한다.
- 지역화 문자열 키, CSV와 번역 보조 파일의 결합을 보존한다.
- 쿠킹 경로를 변경했다면 해당 배포 타겟의 포함·제외 결과를 검증한다.
- 다른 작업의 변경은 커밋에 포함하지 않는다.

전체 에셋 참조를 중복 관리하는 새 시스템보다는 작은 이동 목록과 기존 검증 도구를 활용한다. 기존 `Tools/RaidLevels/verify_migration.py`의 이동 전후 비교 방식은 참고할 수 있다.

## 8. 조사 근거

아래 링크는 조사에 사용한 주요 파일이다. 이후 코드가 변경될 수 있으므로 실제 실행 전 현재 내용을 다시 확인한다.

| 근거 | 관련 내용 |
| --- | --- |
| [DefaultGame.ini](../TunaSweeper/Config/DefaultGame.ini) | MapsToCook, AlwaysCook, EditorOnly 제외, Data·Slate 스테이징 |
| [Demo/DefaultGame.ini](../TunaSweeper/Config/Custom/Demo/DefaultGame.ini) | MainRaid의 데모 제외 경계 |
| [TunaSweeperATVActor.cpp](../TunaSweeper/Source/TunaSweeper/Private/Vehicle/TunaSweeperATVActor.cpp) | ATV 메시·사운드의 직접 경로 참조 |
| [TunaSweeperATVDamage.cpp](../TunaSweeper/Source/TunaSweeper/Private/Vehicle/TunaSweeperATVDamage.cpp) | ATV 파편·파괴 효과 경로 |
| [TunaSweeperEnemyCharacter.cpp](../TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp) | Prototype 재질의 런타임 사용 |
| [TunaSweeperItemDataSubsystem.cpp](../TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperItemDataSubsystem.cpp) | 아이콘 경로 조합과 데이터 로드 |
| [TunaSweeperAdditionalTranslations.cpp](../TunaSweeper/Source/TunaSweeper/Private/Subsystem/TunaSweeperAdditionalTranslations.cpp) | 원본 CSV 옆 Translations 파일 로드 규약 |
| [TunaSweeperUIFont.cpp](../TunaSweeper/Source/TunaSweeper/Private/UI/TunaSweeperUIFont.cpp) | Slate/Fonts의 파일 경로 |
| [TunaSweeperFMSoundTool.cpp](../TunaSweeper/Source/TunaSweeperEditor/Private/TunaSweeperFMSoundTool.cpp) | Data/FMSoundPresets.json 편집 도구 설정 |
| [ProjectPaths.cs](../Tools/ItemDataCsvViewer/ProjectPaths.cs) | Data/ExcelExport 경로 |
| [취수시설 임포터](../Tools/WaterIntakeDemo/import_unreal.py) | SourceArt의 PNG·FBX 원본 경로 |
| [기존 이동 검증 도구](../Tools/RaidLevels/verify_migration.py) | 이동 전후 자산·레벨 비교 |
| [지역화 규칙](localization.md) | 문자열 키와 번역 보조 CSV |
| [저장 상태 규칙](save_persistence.md) | 저장 식별자와 액터 이름 보존 |
| [일회성 생성 코드 정리 기록](asset_generation_cleanup_2026-09-06.md) | 저장된 에셋 유지와 자동 재생성 제거 원칙 |
| [Epic UE 5.7 Redirectors](https://dev.epicgames.com/documentation/en-us/unreal-engine/asset-redirectors-in-unreal-engine?application_version=5.7) | 이동·이름 변경 후 리다이렉터와 참조 재저장 |
