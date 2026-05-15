# 퀘스트 시스템 계획

## 범위

이 문서는 TunaSweeper의 첫 퀘스트 구현 계획을 정리한다.

대화 시스템은 이번 범위에서 제외한다. 퀘스트 상호작용을 실행하면 퀘스트 UI가 바로 열리고, UI에서 수락 버튼을 누르면 퀘스트 상태가 수락됨으로 바뀐다. 이후 벙커에서 레이드 맵으로 이동하면 첫 목표가 완료된다.

## 첫 퀘스트

| 항목 | 값 |
| --- | --- |
| QuestId | `quest_first_outing` |
| 제목 | `첫 외출` |
| 내용 | `이제 들어왔으니 나가서 한번 산책하고 들어와` |
| 목표 | `벙커 밖으로 이동` |
| 달성 조건 | `BunkerMap -> RaidMap` 레벨 이동 요청 성공 |
| 초기 상태 | `Available` |
| 보상 | 첫 단계에서는 없음. 단, 보상 상태 흐름은 유지 |

## 퀘스트 상태

런타임 상태는 명시적인 enum 하나로 관리한다.

```cpp
enum class ETunaSweeperQuestState : uint8
{
	Available,
	Accepted,
	RewardAvailable,
	RewardCompleted
};
```

상태 전이는 다음과 같다.

```text
Available
  -> AcceptQuest
Accepted
  -> CompleteObjective(BunkerMap -> RaidMap)
RewardAvailable
  -> ClaimReward
RewardCompleted
```

상태별 규칙:

- `Available`: 받기 가능 상태. 퀘스트 UI에서 `수락` 버튼을 누를 수 있다.
- `Accepted`: 수락됨 상태. 목표가 활성화되어 있고, UI에서는 진행 중으로 표시한다.
- `RewardAvailable`: 보상 가능 상태. 목표는 완료되었고, UI에서 보상 수령 액션을 제공한다.
- `RewardCompleted`: 보상 완료 상태. 일반적으로 상호작용 목록에서 숨긴다.
- 디버그/초기화 도구가 아닌 일반 게임 흐름에서는 상태를 뒤로 되돌리지 않는다.

## 런타임 소유자

`UTunaSweeperQuestSubsystem`을 `UGameInstanceSubsystem`으로 만든다.

이유:

- 현재 프로젝트는 지속 상태를 다루는 시스템에 `UGameInstanceSubsystem`을 이미 사용하고 있다.
- 퀘스트 상태는 `BunkerMap`과 `RaidMap` 사이의 `OpenLevel` 이후에도 유지되어야 한다.
- 첫 구현에서는 월드별 퀘스트 소유가 필요하지 않다.

핵심 API 초안:

```cpp
ETunaSweeperQuestState GetQuestState(FName QuestId) const;
bool CanAcceptQuest(FName QuestId) const;
bool AcceptQuest(FName QuestId);
bool CanClaimQuestReward(FName QuestId) const;
bool ClaimQuestReward(FName QuestId);
void NotifyLevelTravelRequested(FName SourceLevelName, FName TargetLevelName);
```

서브시스템이 소유할 데이터:

```text
퀘스트 정의:
- 첫 구현용 quest_first_outing 정적 정의

퀘스트 런타임 상태:
- Map<FName, ETunaSweeperQuestState>
```

첫 단계에서는 퀘스트 정의를 C++에 하드코딩해도 된다. 퀘스트 수가 늘어나 에디터에서 관리할 필요가 생기면 `UPrimaryDataAsset` 기반으로 옮긴다.

## 레벨 이동 완료 처리

첫 퀘스트는 플레이어가 벙커에서 레이드로 이동할 때 완료된다.

현재 연결 지점:

```text
ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel
```

처리 계획:

1. `TravelToTargetLevel`이 유효한 `TargetLevelName`을 받으면 현재 맵 이름을 확인한다.
2. PIE 실행 시 맵 이름에 접두사가 붙을 수 있으므로 맵 이름 비교는 방어적으로 처리한다.
3. 전환 영상 재생 또는 fallback `OpenLevel` 호출 전에 퀘스트 서브시스템에 레벨 이동 요청을 알린다.
4. 현재 맵이 `BunkerMap`이고 목표 맵이 `RaidMap`이면 `quest_first_outing` 상태를 `Accepted`에서 `RewardAvailable`로 바꾼다.

첫 구현에서는 `RaidMap` 로드 후 배치 액터가 완료를 감지하는 방식보다 이 방식이 낫다. `GameInstanceSubsystem`은 레벨 이동 전후에 유지되고, 레벨 이동 의도를 이미 알고 있는 단일 지점에서 이벤트를 발생시킬 수 있기 때문이다.

## 퀘스트 상호작용

퀘스트 상호작용은 대화와 분리한다.

기대 흐름:

```text
플레이어가 NPC 또는 퀘스트 제공자를 포커스
-> 관련 퀘스트가 Available, Accepted, RewardAvailable 중 하나면 상호작용 목록에 "퀘스트" 표시
-> 플레이어가 "퀘스트" 선택
-> 퀘스트 UI 열림
-> 플레이어가 "수락" 선택
-> 퀘스트 상태가 Accepted로 변경
```

현재 상호작용 시스템은 `UTunaSweeperInteractableComponent` 하나와 `ETunaSweeperInteractionType`을 사용한다. 첫 구현에서는 `Quest` 상호작용 타입을 추가하거나, 퀘스트 전용 인터랙터블 액터를 만들어 같은 상호작용 처리 흐름으로 라우팅한다.

이후 다중 상호작용 시스템이 구현되면 같은 동작을 하나의 상호작용 항목으로 표현한다.

```text
InteractionEntry
- Type: Quest
- DisplayName: 퀘스트
- QuestId: quest_first_outing
- Execute: OpenQuestPanel(quest_first_outing)
```

퀘스트 항목 표시 조건:

```text
Quest state is Available
or Quest state is Accepted
or Quest state is RewardAvailable
```

퀘스트 항목 숨김 조건:

```text
Quest state is RewardCompleted
```

## 퀘스트 UI

한 번에 하나의 퀘스트를 보여주는 퀘스트 UI 위젯을 만든다.

권장 클래스와 위젯:

```text
UTunaSweeperQuestWidget
WBP_Quest
```

표시 내용:

```text
제목: 첫 외출
내용: 이제 들어왔으니 나가서 한번 산책하고 들어와
목표: 벙커 밖으로 이동
상태에 따른 기본 버튼
닫기 버튼
```

기본 버튼 동작:

| 상태 | 버튼 텍스트 | 활성화 | 동작 |
| --- | --- | --- | --- |
| `Available` | `수락` | 예 | `AcceptQuest(quest_first_outing)` |
| `Accepted` | `진행 중` | 아니오 | 없음 |
| `RewardAvailable` | `보상 받기` | 예 | `ClaimQuestReward(quest_first_outing)` |
| `RewardCompleted` | `완료` | 아니오 | 없음 |

첫 퀘스트에 실제 보상이 없더라도 `RewardAvailable -> RewardCompleted` 단계는 유지한다. 그래야 상태 모델을 검증할 수 있고, 이후 보상이 있는 퀘스트와 같은 흐름을 사용할 수 있다.

## 플레이어 컨트롤러 연동

상호작용 서브시스템이 퀘스트 UI를 직접 생성하지 않는 편이 좋다.

권장 흐름:

```text
UTunaSweeperInteractionSubsystem
-> ATunaSweeperPlayerController::OpenQuestPanel(QuestId)
-> 게임 HUD 또는 퀘스트 위젯 열기
```

이는 현재 루팅 UI가 `ATunaSweeperPlayerController`를 통해 열리는 구조와 맞다.

## 구현 순서

1. 퀘스트 상태 enum과 기본 퀘스트 데이터 구조를 추가한다.
2. `UTunaSweeperQuestSubsystem`을 추가하고, 첫 퀘스트 정의와 런타임 상태 맵을 넣는다.
3. `AcceptQuest`, `ClaimQuestReward`, `NotifyLevelTravelRequested`를 추가한다.
4. 퀘스트 UI를 여는 `Quest` 상호작용 라우팅을 추가한다.
5. `UTunaSweeperQuestWidget`과 `WBP_Quest`를 추가한다.
6. 플레이어 컨트롤러에 `OpenQuestPanel(QuestId)`를 추가한다.
7. `ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel`에서 퀘스트 서브시스템에 레벨 이동 요청을 알린다.
8. 벙커에 퀘스트 상호작용 제공자를 배치하거나 설정한다.
9. 전체 상태 흐름을 테스트한다.

## 완료 확인 기준

- `quest_first_outing`의 시작 상태가 `Available`이다.
- 퀘스트 상호작용을 누르면 퀘스트 UI가 열린다.
- `수락`을 누르면 상태가 `Accepted`로 바뀐다.
- `BunkerMap`에서 `RaidMap`으로 이동하면 상태가 `RewardAvailable`로 바뀐다.
- 완료 후 퀘스트 UI를 열면 `보상 받기`가 표시된다.
- `보상 받기`를 누르면 상태가 `RewardCompleted`로 바뀐다.
- `RewardCompleted` 이후에는 최종 상호작용 UI 정책에 따라 퀘스트 상호작용이 숨겨지거나 비활성화된다.

## 이후 확장

퀘스트 수가 늘어나면 다음을 추가한다.

- 퀘스트 정의를 `UPrimaryDataAsset`으로 이전한다.
- 아이템 획득, 적 처치, 구역 진입, 특정 상호작용 완료 같은 목표 타입을 추가한다.
- `SaveGame` 저장을 추가한다.
- NPC가 퀘스트, 상점, 업그레이드 같은 여러 상호작용 항목을 노출하게 한다.
- 대화는 선택 사항으로 유지하고 이벤트 기반으로 연결한다. 나중에 대화가 퀘스트 액션을 발생시킬 수는 있지만, 퀘스트 시스템이 대화 시스템에 의존하지는 않게 한다.
