# 능력치 연구 트리 설계 계획

> 구현 상태: 2026-08-28 기준 C++ 연구 서브시스템, save version 21, JSON 노드 데이터, 실제 UMG WBP 에셋 및 HUD 탭 연결까지 구현됨.

## 목표

- 화면 위에서 아래로 성장하는 수직 연구 트리를 제공한다.
- 한 행에는 최대 3개의 노드만 둔다.
- 가지는 짧게 갈라졌다가 다시 중앙 흐름으로 모이게 하여 화면이 지나치게 넓거나 복잡해지지 않게 한다.
- 연구 포인트는 사용하지 않는다. 연구 시간만 필요하며 최종적으로 모든 노드를 연구할 수 있다.
- 연구 가능 조건은 `완료 확정된 노드 수` 하나만 사용한다.
- 초기 노드는 요구 완료 수가 `0`이다.
- 개방된 노드는 개수 제한 없이 동시에 연구할 수 있으며 취소, 환불, 초기화 기능은 제공하지 않는다.
- 연구 시간이 끝나도 효과를 즉시 적용하지 않는다. 플레이어가 `완료` 버튼을 눌러야 효과가 반영된다.
- 연구 시간은 첫 노드 약 10초부터 시작해 최종 노드 3,600초(1시간)까지 증가한다.
- 연구 시작, 타이머 완료 감지, 완료 확정 때 현재 세이브 슬롯을 즉시 저장한다.
- 게임이 종료된 동안에도 절대 시각을 기준으로 연구 시간이 흐른다.
- UMG 구조를 C++에서 생성하지 않는다. 실제 WBP 에셋의 디자이너 트리를 사용한다.

## 확정 규칙

### 연구 비용과 진행 제한

- 별도의 연구 포인트, 스킬 포인트, 재화 비용은 두지 않는다.
- 개방 조건을 충족한 노드는 다른 노드의 진행 여부와 관계없이 동시에 연구할 수 있다.
- 타이머가 끝난 노드를 아직 완료 확정하지 않았더라도 다른 개방 노드의 연구를 시작할 수 있다.
- 시작한 연구는 취소할 수 없다.
- 모든 노드는 반복 연구할 수 없고 한 번만 완료할 수 있다.

이 규칙이면 제한된 포인트로 일부 분기만 고르는 빌드형 트리가 되지 않는다. 플레이어는 현재 개방된 노드를 원하는 만큼 병렬로 시작할 수 있고, 충분한 시간이 지나면 전부 완료할 수 있다.

### 개방 조건

노드의 연구 가능 여부는 아래 식 하나로 판단한다.

```text
완료 확정된 노드 수 >= 노드의 RequiredAppliedNodeCount
```

- `완료 확정`은 연구 시간이 끝난 상태가 아니라 `완료` 버튼까지 눌러 효과가 적용된 상태를 뜻한다.
- 연구 중인 노드와 완료 대기 노드는 개방 수량에 포함하지 않는다.
- 화면 연결선과 부모 노드 ID는 시각적 계보만 표현하며 개방 조건에는 사용하지 않는다.
- 특정 가지를 먼저 골랐다는 이유로 다른 가지가 영구 잠기지 않는다.
- 노드별 요구 수량은 데이터 검증을 통해 모든 노드가 최종 도달 가능한지 확인한다.

## 노드 상태

| 상태 | 조건 | 조작 | 효과 적용 | 저장 |
|---|---|---|---|---|
| `Locked` | 완료 수가 요구 수량보다 적음 | 불가 | 없음 | 없음 |
| `Available` | 수량 조건 충족, 아직 시작하거나 완료하지 않음 | 누르면 즉시 연구 시작 | 없음 | 시작 직후 즉시 저장 |
| `Researching` | 현재 시각이 종료 시각보다 이전 | 취소 불가 | 없음 | 시작 상태가 저장되어 있음 |
| `ReadyToClaim` | 연구 시간이 끝났지만 완료 버튼을 누르지 않음 | `완료` 버튼 사용 | 아직 없음 | 상태 최초 감지 시 즉시 저장 |
| `Applied` | 완료 버튼을 눌러 완료 확정됨 | 재연구 불가 | 적용됨 | 완료 확정 직후 즉시 저장 |

상태 흐름은 다음과 같다.

```text
Locked -> Available -> Researching -> ReadyToClaim -> Applied
            클릭          시간 종료          완료 클릭
```

`Researching`과 `ReadyToClaim` 상태인 노드가 여러 개 존재할 수 있다. 한 노드가 완료 대기 중이어도 다른 `Available` 노드의 연구 시작을 막지 않는다.

## 권장 트리 모양과 초기 규모

첫 구현은 7행, 13노드 정도로 제한한다. 중앙 주축을 유지하고 좌우 가지는 한두 행 이상 깊게 분리하지 않는다.

```text
행 0                         [A]
                             |
행 1                    [B]     [C]
                         \       /
행 2                         [D]
                             |
행 3                   [E]  [F]  [G]
                        \    |    /
행 4                     [H]   [I]
                          \   /
행 5                   [J]  [K]  [L]
                        \    |    /
행 6                         [M]
```

권장 개방 수량과 연구 시간 초안은 다음과 같다.

| 행 | 노드 수 | `RequiredAppliedNodeCount` | 노드당 연구 시간 | 총 연구량(모두 순차 진행 시) |
|---:|---:|---:|---:|---:|
| 0 | 1 | 0 | 10초 | 10초 |
| 1 | 2 | 1 | 30초 | 1분 10초 |
| 2 | 1 | 2 | 60초 | 2분 10초 |
| 3 | 3 | 3 | 180초 | 11분 10초 |
| 4 | 2 | 5 | 600초 | 31분 10초 |
| 5 | 3 | 7 | 1,800초 | 2시간 1분 10초 |
| 6 | 1 | 10 | 3,600초 | 3시간 1분 10초 |

이 수치는 시스템 검증용 초기값이며 마지막 열은 단순 연구 시간 합계다. 개방된 같은 단계 노드를 병렬로 시작할 수 있으므로 실제 전체 개방 경과 시간은 이 합계보다 짧아진다. 위 초안을 완료 즉시 확정하고 가능한 노드를 모두 바로 시작하는 경우 이론상 최단 경과 시간은 약 1시간 33분 40초다. 최종 노드의 초기값은 3,600초로 고정하되, 연구 시간 증가 공식을 C++에 넣지 않는다. 모든 노드의 시간은 JSON 데이터에 명시하여 사용자가 직접 개별 튜닝할 수 있게 한다.

## 데이터 설계

### 정의 파일

`Content/Data/StatResearchNodes.json` 한 파일을 사용한다. 노드 배치와 게임 규칙을 여러 파일로 나누지 않는다. 연구 시간, 개방 수량, 표시 행·열, 시각 부모, 아이콘, 현지화 키와 능력치 효과는 모두 이 JSON에서 수정할 수 있어야 하며 C++에는 개별 노드 값이나 시간표를 하드코딩하지 않는다.

```json
{
  "nodes": [
    {
      "node_id": "research_vitals_01",
      "row": 0,
      "column": 1,
      "visual_parent_ids": [],
      "required_applied_node_count": 0,
      "research_duration_seconds": 10,
      "name_key": "research.vitals_01.name",
      "description_key": "research.vitals_01.desc",
      "icon_path": "/Game/UI/Research/T_UI_Research_Vitals_01",
      "effects": [
        { "type": "max_health", "value": 5000 }
      ]
    }
  ]
}
```

### 필드 규칙

- `node_id`: 세이브에 기록되는 영구 식별자다. 출시 후 이름을 바꾸지 않는다.
- `row`: 위에서 아래로 증가한다.
- `column`: `0=왼쪽`, `1=중앙`, `2=오른쪽`만 허용한다.
- `visual_parent_ids`: 연결선 검증과 디자이너 참고용이며 연구 가능 판정에는 사용하지 않는다.
- `required_applied_node_count`: 이미 완료 확정된 노드 수 조건이다.
- `research_duration_seconds`: 실제 연구 시간이다. 1초 이상 정수로 두며 최종 노드의 초기값은 `3600`이다. 런타임은 이 값을 그대로 사용하고 별도의 시간 증가 공식을 적용하지 않는다.
- `name_key`, `description_key`: 기존 텍스트 서브시스템을 통해 현지화한다.
- `icon_path`: 노드 WBP에 표시할 소프트 텍스처 경로다.
- `effects`: 완료 확정 후 합산할 능력치 효과 목록이다.

초기 효과 타입은 현재 플레이어 성장 코드와 충돌 없이 합칠 수 있는 아래 5개만 지원한다.

- `max_health`
- `max_food`
- `max_hydration`
- `max_stamina`
- `carry_strength`

공격력과 방어력은 현재 프로젝트 규칙상 장비와 탄약으로만 보정하므로 초기 연구 효과에 포함하지 않는다. 나중에 해당 규칙을 바꾸기로 확정했을 때 효과 타입을 추가한다.

체력·배부름·수분·스태미나 값은 기존 고정소수점 기준 `10000 = 1.0`을 따른다. 운반 힘은 기존과 동일하게 실수 증가량으로 처리한다.

### 데이터 검증

에디터 자동화 테스트 또는 데이터 검증 함수에서 아래를 거부한다.

- 중복되거나 비어 있는 `node_id`
- 한 행에 4개 이상의 노드
- 같은 `row`와 `column` 중복
- `column`이 0~2 범위를 벗어남
- 1초 미만 연구 시간
- 음수 개방 수량 또는 음수 효과 값
- 존재하지 않거나 같은 행/아래 행을 가리키는 시각 부모 ID
- 도달 불가능한 요구 수량 배열

최종 도달 가능성은 요구 수량을 오름차순으로 정렬한 뒤, 인덱스 `i`의 요구 수량이 `i` 이하인지 검사하면 된다. 하나라도 초과하면 해당 지점부터 연구 가능한 노드가 없어질 수 있으므로 데이터 오류로 처리한다.

## 런타임 구조

### 단일 연구 서브시스템

`UTunaSweeperResearchSubsystem : UGameInstanceSubsystem` 하나가 정의 로딩, 상태 판정, 시간 처리, 세이브 입출력, 효과 합산을 담당한다. 별도의 타이머 액터나 레벨별 매니저는 만들지 않는다.

주요 API는 다음 정도로 제한한다.

```text
GetAllNodeViews()
GetNodeView(NodeId)
TryStartResearch(NodeId)
TryClaimResearch(NodeId)
GetAppliedStatBonuses()
LoadResearchProgressFromSave(...)
ExportResearchProgressForSave(...)
ResetResearchProgressForNewGame()
RefreshTemporalState(NowUtcTicks)
```

서브시스템은 `OnResearchStateChanged`와 `OnResearchEffectsChanged` 두 이벤트만 제공한다. UI는 상태 이벤트를 구독하고, 플레이어 능력치 적용부는 효과 이벤트를 구독한다.

### 시간 진행

남은 초를 계속 저장하지 않고 진행 중인 각 노드의 시작·종료 절대 시각을 저장한다.

```text
Node.FinishUtcTicks = Node.StartUtcTicks + Node.ResearchDuration
Node.ReadyToClaim = EffectiveUtcNowTicks >= Node.FinishUtcTicks
```

- 절대 시각은 `FDateTime::UtcNow().GetTicks()`를 사용한다.
- 게임 실행 중에는 GameInstance 범위의 코어 티커가 약 0.25~1초 간격으로 상태를 확인한다. 레벨 전환에 종속된 월드 타이머 액터는 사용하지 않는다.
- 게임이 꺼진 동안에는 아무 코드도 실행되지 않지만 각 종료 시각이 절대값으로 저장되어 있으므로 다음 실행 시 모든 활성 연구의 경과가 계산된다.
- 게임이 꺼져 있던 동안 하나 이상의 연구가 끝난 경우, 세이브 로드가 완전히 끝난 다음 첫 티커에서 해당 항목들을 `ReadyToClaim`으로 정규화하고 한 번 즉시 다시 저장한다. 실행 중이 아닌 순간에 파일을 직접 쓸 수는 없으므로 이것이 가능한 가장 이른 저장 시점이다.
- 실행 중 시스템 시계가 뒤로 이동하더라도 `FPlatformTime` 기반 세션 경과값과 마지막 관측 UTC를 함께 사용해 남은 시간이 역행하지 않게 한다.
- 완전한 오프라인 방식에서는 사용자가 시스템 시계를 앞으로 조작하는 행위를 확실히 막을 수 없다. 서버 시각 검증을 추가하지 않는 한 이 제한은 명시적으로 수용한다.

### 세이브 데이터

`UTunaSweeperSaveGame`에 다음 상태를 추가한다.

```text
AppliedResearchNodeIds: TArray<FName>
ActiveResearchStates: TArray<FTunaSweeperActiveResearchSaveData>
LastObservedResearchUtcTicks: int64
```

- `FTunaSweeperActiveResearchSaveData`는 `NodeId`, `StartUtcTicks`, `FinishUtcTicks`, `bTimerCompleted`를 가진다.
- `AppliedResearchNodeIds`만 실제 효과와 개방 수량의 기준이다.
- 각 활성 항목의 `bTimerCompleted`는 해당 타이머 완료 상태를 처음 감지했을 때 즉시 저장했다는 사실을 보존한다.
- 연구 시작 시 새 활성 항목을 배열에 추가한다. 같은 NodeId는 중복 추가하지 않는다.
- 완료 확정 시 해당 노드의 활성 항목만 제거하고 노드 ID를 `AppliedResearchNodeIds`에 한 번만 추가한다. 다른 진행 중 연구는 유지한다.
- 효과 수치 자체는 저장하지 않는다. 완료 ID와 현재 정의 데이터에서 매번 재계산하여 중복 적용을 방지한다.
- 새 게임과 슬롯 변경 때 연구 런타임 상태를 반드시 초기화한다.
- 저장 버전은 `CurrentSaveVersion`을 21로 올리되 `MinimumSupportedSaveVersion`은 20으로 유지한다. 기존 버전 20 세이브는 빈 연구 상태로 정상 로드하고 삭제하지 않는다.
- 구현이 끝나면 `Docs/save_persistence.md`에 필드와 상태 전이를 추가한다.

### 즉시 저장 시점

1. `Available -> Researching`: 해당 노드의 UTC 시작·종료 시각을 활성 연구 배열에 추가한 직후 `SaveGameState()` 호출.
2. `Researching -> ReadyToClaim`: 실행 중 최초 감지 또는 다음 실행의 로드 완료 직후 해당 활성 항목의 완료 플래그를 설정하고 `SaveGameState()` 호출. 같은 틱에 여러 연구가 끝나면 모두 갱신한 뒤 한 번의 즉시 저장으로 함께 기록한다.
3. `ReadyToClaim -> Applied`: 완료 ID 추가와 해당 활성 항목 제거 직후 `SaveGameState()` 호출.

기존 안전 저장과 백업 경로를 그대로 사용한다. 로드 도중 재귀 저장이 발생하지 않게 오프라인 완료 정규화 저장은 인벤토리/슬롯 로드가 완료된 다음 틱으로 미룬다.

## 능력치 적용

- 경험치 레벨 보너스와 연구 보너스를 별도 데이터 원천으로 유지하되, 캐릭터에 적용하기 직전에 하나의 진행 보너스로 합산한다.
- 기존 `ApplyExperienceLevelStatBonuses()`는 역할에 맞게 `ApplyProgressionStatBonuses()`로 정리하고 경험치 보너스와 연구 보너스를 함께 읽는다.
- 연구 타이머가 끝난 순간에는 연구 보너스를 합산하지 않는다.
- 완료 버튼을 눌러 `AppliedResearchNodeIds`에 들어간 뒤에만 `OnResearchEffectsChanged`를 발생시키고 캐릭터 수치를 재계산한다.
- 최대 체력·배부름·수분·스태미나 변경 시 현재 비율 보존 정책은 기존 경험치 레벨 보너스 적용 경로와 동일하게 유지한다.
- 운반 힘 변경 후에는 최대 운반 중량과 중량 디버프를 즉시 다시 계산한다.
- 로드 시에도 완료 ID에서 파생하여 한 번만 적용하므로 저장과 재접속을 반복해도 보너스가 중첩되지 않는다.

## UMG/WBP 구성

### 원칙

- C++에서 `WidgetTree->ConstructWidget`, `RebuildWidget()` 기반 레이아웃 생성, 네이티브 폴백 UI를 만들지 않는다.
- 런타임에 행이나 노드 위젯을 코드로 생성해 컨테이너에 추가하지 않는다.
- 실제 배치, 간격, 크기, 연결선, 애니메이션은 WBP 디자이너에서 편집한다.
- C++은 `BindWidget`으로 이미 존재하는 위젯을 참조하고 텍스트, 색상, 진행률, 활성 여부만 갱신한다.
- 필수 BindWidget이 빠진 WBP는 조용히 코드 UI로 대체하지 않고 에디터 검증 오류로 표시한다.

### 실제 에셋

#### `WBP_ResearchNode`

부모는 `UTunaSweeperResearchNodeWidget`으로 둔다. 다음 요소를 실제 디자이너 트리에 만든다.

- `NodeButton`
- `IconImage`
- `NameText`
- `RequirementText`
- `RemainingTimeText`
- `ResearchProgressBar`
- `ActionText`
- `LockedOverlay`
- `ResearchingOverlay`
- `ReadyOverlay`
- `AppliedCheckImage`

각 인스턴스는 에디터에서 `NodeId`를 지정한다. 상태별 색, 애니메이션, 테두리, 폰트는 WBP에서 자유롭게 수정할 수 있다. `NodeButton`은 `Available`일 때 연구 시작, `ReadyToClaim`일 때 완료 확정으로 동작한다.

#### `WBP_ResearchTree`

부모는 `UTunaSweeperResearchTreeWidget`으로 둔다. 다음 구조를 WBP 디자이너에서 직접 만든다.

```text
RootOverlay
  Background
  MainVerticalBox
    HeaderArea
    ResearchStatusSummary
    ResearchScrollBox
      ResearchRowsVerticalBox
        Row0Overlay -> 최대 3개의 WBP_ResearchNode 인스턴스
        Row1Overlay -> 최대 3개의 WBP_ResearchNode 인스턴스
        ...
```

- 행, 빈 칸, 노드 인스턴스와 연결선 Image를 전부 에디터에서 배치한다.
- 각 행은 3개의 같은 폭 열 기준으로 맞춘다. 사용하지 않는 열은 Spacer로 남긴다.
- 연결선은 상태 판정용이 아니므로 WBP의 장식 Image로 둔다.
- 루트 C++ 위젯은 이미 배치된 `UTunaSweeperResearchNodeWidget`을 수집해 `NodeId`와 데이터 정의를 연결할 뿐 트리를 재구성하지 않는다.
- WBP에 중복 NodeId가 있거나 JSON 정의가 WBP에 없으면 에디터/개발 빌드에서 오류를 낸다.

#### `WBP_GameHud`

- `ETunaSweeperHudMode`에 `Research`를 추가한다.
- 기존 인벤토리·지도·메모·퀘스트와 같은 유틸리티 탭에 연구 버튼을 하나 추가한다.
- `WBP_ResearchTree` 인스턴스를 `WBP_GameHud`에 실제로 배치하고 `BindWidget`으로 연결한다.
- 연구 모드가 아닐 때는 `Collapsed`, 연구 모드일 때만 표시한다.
- 별도 액터나 별도 레벨을 거치지 않고 현재 HUD 안에서 연다.

## 코드 파일 계획

구조가 과도하게 갈라지지 않도록 아래 정도로 제한한다.

```text
Public/Research/TunaSweeperResearchTypes.h
Public/Subsystem/TunaSweeperResearchSubsystem.h
Private/Subsystem/TunaSweeperResearchSubsystem.cpp
Public/UI/TunaSweeperResearchWidgets.h
Private/UI/TunaSweeperResearchWidgets.cpp
Content/Data/StatResearchNodes.json
Content/Data/ResearchTextStrings.csv
Content/UI/WBP_ResearchNode.uasset
Content/UI/WBP_ResearchTree.uasset
```

기존 파일 수정 범위는 세이브 게임, GameInstance 세이브 입출력, 플레이어 진행 보너스 적용, HUD 모드와 `WBP_GameHud` 연결로 한정한다.

## 테스트 계획

### 자동화 테스트

시간을 실제로 기다리지 않고 `NowUtcTicks`를 주입해 아래를 검증한다.

1. 완료 수 0에서 요구 수 0 노드만 `Available`이다.
2. 완료 버튼을 누르기 전 `ReadyToClaim` 노드는 완료 수에 포함되지 않는다.
3. 개방된 두 개 이상의 노드를 동시에 시작할 수 있고 각 노드의 종료 시각이 독립적으로 유지된다.
4. 10초 연구가 종료 시각 전에는 `Researching`, 종료 시각 이상에서는 `ReadyToClaim`이다.
5. 게임 종료를 가정해 각 종료 시각보다 늦은 UTC로 로드하면 해당 노드들이 각각 완료 대기가 된다.
6. 시스템 시계가 뒤로 이동해도 실행 중 남은 시간이 증가하지 않는다.
7. 완료 확정 전에는 능력치가 변하지 않고 확정 후 한 번만 변한다.
8. 한 노드가 완료 대기 중이어도 다른 개방 노드의 연구를 시작할 수 있다.
9. 저장 후 재로드해도 복수 활성 연구와 효과가 중복 적용되지 않는다.
10. 새 게임과 다른 슬롯 전환 시 연구 상태가 섞이지 않는다.
11. 모든 노드 요구 수량 배열이 최종 전체 완료 가능한지 검증한다.
12. 한 행 3개 초과, 중복 ID, 중복 좌표, 중복 활성 연구, 잘못된 부모와 효과 타입을 거부한다.
13. 버전 20 세이브가 빈 연구 상태로 로드되고 버전 21로 다시 저장된다.

### 에디터/실행 검증

1. `WBP_ResearchTree`의 노드 배치가 위에서 아래로 스크롤되는지 확인한다.
2. 1920x1080과 최소 지원 해상도에서 한 행 3개가 잘리지 않는지 확인한다.
3. 모든 WBP NodeId가 JSON과 정확히 1:1 대응하는지 확인한다.
4. 서로 다른 종료 시각을 가진 연구 여러 개를 시작하고 각각 독립적으로 진행되는지 확인한다.
5. 연구 시작 직후 종료해 다시 실행했을 때 복수 진행 상태가 유지되는지 확인한다.
6. 게임을 연구 종료 시각 이후 다시 실행했을 때 각 완료 노드에 `완료` 버튼이 보이고 효과는 아직 적용되지 않았는지 확인한다.
7. 한 노드의 완료 버튼을 눌러도 다른 진행 중·완료 대기 노드 상태가 유지되는지 확인한다.
8. 완료 버튼을 누른 직후 캐릭터 능력치와 세이브 파일이 함께 갱신되는지 확인한다.
9. 레벨 전환 중에도 모든 연구 시간이 유지되는지 확인한다.
10. 연구 UI를 열지 않은 상태에서도 실행 중 복수 타이머 완료가 감지되고 저장되는지 확인한다.

## 구현 순서

1. 연구 타입, JSON 로더, 데이터 검증과 도달 가능성 테스트를 만든다.
2. 연구 서브시스템의 상태 전이와 주입 가능한 UTC 시간 평가를 만든다.
3. SaveGame 필드, 버전 21 호환 로드, GameInstance import/export와 새 게임 초기화를 연결한다.
4. 시작·타이머 완료·완료 확정의 세 즉시 저장 지점을 연결한다.
5. 완료 확정 ID에서 연구 보너스를 파생하고 기존 경험치 보너스와 합산한다.
6. `WBP_ResearchNode`와 `WBP_ResearchTree` 실제 에셋을 만들고 BindWidget만 연결한다.
7. `WBP_GameHud`에 연구 탭과 실제 연구 WBP 인스턴스를 배치한다.
8. 자동화 테스트와 오프라인 진행 수동 검증을 통과시킨다.
9. `Docs/save_persistence.md`와 `Docs/game_conventions.md`를 실제 구현 기준으로 갱신한다.
10. 빌드 후 Unreal Editor를 열어 WBP 디자이너 수정 가능 상태와 런타임 동작을 확인한다.

## 완료 기준

- 한 행 최대 3개인 수직 WBP 연구 트리가 실제 에셋으로 존재한다.
- C++ 어디에도 연구 UI의 WidgetTree 생성 코드가 없다.
- 초기 요구 수량 0부터 완료 수량 조건만으로 모든 노드가 최종 도달 가능하다.
- 첫 노드는 약 10초, 최종 노드는 JSON 기본값 3,600초로 동작하며 모든 연구 시간을 JSON에서 수정할 수 있다.
- 개방된 노드는 개수 제한 없이 병렬 연구할 수 있으며 취소와 리셋이 없다.
- 연구 시작, 타이머 완료 감지, 완료 확정이 각각 즉시 저장된다.
- 게임을 종료해도 UTC 종료 시각을 기준으로 연구가 진행된다.
- 타이머 종료만으로 효과가 적용되지 않고 완료 버튼을 눌러야 적용된다.
- 기존 세이브 슬롯, 경험치 레벨 보너스, 레벨 전환과 충돌하지 않는다.
