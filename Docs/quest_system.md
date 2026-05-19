# 퀘스트/목표 진행 프레임워크

## 범위

이 문서는 TunaSweeper의 범용 퀘스트/목표 진행 프레임워크를 정리한다.

이번 범위는 5챕터 전체 콘텐츠 제작이 아니라, 기존에 C++로 하드코딩되어 있던 `quest_first_outing`을 데이터 기반 정의와 범용 목표 이벤트 처리로 일반화하는 코드 작업이다.

## 데이터 정의

퀘스트 정의는 `TunaSweeper/Content/Data/QuestDefinitions.json`에서 읽는다.

기본 필드:

- `quest_id`: 저장과 이벤트 매칭에 쓰는 안정적인 퀘스트 ID.
- `title`, `description`: 퀘스트 UI와 HUD 추적에 표시할 텍스트.
- `auto_track_on_accept`: 수락 시 HUD 추적 대상으로 자동 설정할지 여부.
- `objectives`: 목표 목록.
- `rewards`: 코인과 아이템 보상.

현재 이전된 첫 퀘스트:

| 항목 | 값 |
| --- | --- |
| QuestId | `quest_first_outing` |
| 제목 | `첫 외출` |
| 목표 | `leave_bunker` |
| 목표 타입 | `level_travel` |
| 달성 조건 | `BunkerMap -> RaidMap` |
| 보상 | 코인 100 |

## 런타임 상태

런타임 소유자는 `UTunaSweeperQuestSubsystem`이다.

퀘스트 상태:

```cpp
enum class ETunaSweeperQuestState : uint8
{
	Available,
	Accepted,
	RewardAvailable,
	RewardCompleted
};
```

상태 전이:

```text
Available
  -> AcceptQuest
Accepted
  -> all objectives complete
RewardAvailable
  -> ClaimQuestReward
RewardCompleted
```

저장되는 진행 데이터:

- `QuestId`
- `State`
- 목표별 `ObjectiveId`, `CurrentCount`
- HUD 추적 대상 `TrackedQuestId`
- 퀘스트 코인 잔액 `QuestCoinBalance`

세이브 슬롯을 바꾸거나 새 게임을 시작하면 퀘스트 진행, 추적 대상, 퀘스트 코인이 슬롯별로 분리된다.

## 목표 이벤트

v1 목표 타입:

- `level_travel`: 레벨 이동 요청 성공. `source_level`, `target_level`로 필터링한다.
- `item_acquired`: 플레이어가 아이템을 획득했을 때. `item_id`가 비어 있으면 모든 아이템을 허용한다.
- `enemy_killed`: 플레이어가 적을 처치했을 때. `enemy_id`가 비어 있으면 모든 적 처치를 허용한다.
- `interaction_completed`: 상호작용 처리가 성공했을 때. `interaction_event_id`나 `interaction_type`으로 필터링한다.

현재 연결 지점:

- `ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel`
- `UTunaSweeperGameInstance::AddItemToFirstAvailableInventorySlot`
- `UTunaSweeperGameInstance::AddItemToPreferredAvailableSlot`
- `UTunaSweeperGameInstance::MoveItemBetweenSlots`의 루트 컨테이너 획득 경로
- `ATunaSweeperEnemyCharacter::HandleDeath`
- `UTunaSweeperInteractionSubsystem::RequestInteraction`

이벤트는 `Accepted` 상태의 퀘스트에만 진행도를 반영한다. 목표 진행도는 목표별 `RequiredCount`까지만 증가하고, 모든 목표가 완료되면 상태가 `RewardAvailable`로 바뀐다.

## UI와 보상

`UTunaSweeperQuestWidget`은 한 퀘스트의 제목, 설명, 다중 목표 진행도, 보상을 표시한다.

기본 버튼 동작:

| 상태 | 버튼 | 동작 |
| --- | --- | --- |
| `Available` | `수락` | `AcceptQuest` |
| `Accepted` | `진행 중` | 없음 |
| `RewardAvailable` | `보상 받기` | `ClaimQuestReward` |
| `RewardCompleted` | `완료` | 없음 |

`UTunaSweeperGameHudWidget`은 추적 중인 퀘스트가 있으면 제목과 목표 진행도를 간단히 표시한다. 수락 시 `auto_track_on_accept`가 켜진 퀘스트는 자동 추적된다.

보상 수령은 실패 시 상태를 바꾸지 않는다. 아이템 보상은 플레이어 인벤토리 빈 칸을 먼저 확인하고, 공간이 없으면 지급하지 않는다. 현재 첫 퀘스트는 코인 보상만 사용한다.

## 추가 규칙

- 새 퀘스트를 추가할 때는 `QuestDefinitions.json`에 정의하고, 필요한 목표 이벤트 소스가 이미 연결되어 있는지 확인한다.
- `enemy_killed` 목표에서 특정 적만 요구하려면 적 스폰 데이터에 `enemy_id`를 추가한다.
- `interaction_completed` 목표에서 특정 상호작용만 요구하려면 해당 `UTunaSweeperInteractableComponent::ObjectiveEventId`를 설정한다.
- 저장 필드를 변경하면 `Docs/save_persistence.md`도 같은 변경에서 갱신한다.
