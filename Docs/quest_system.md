# 퀘스트/목표 진행 프레임워크

## 추출 지점을 통한 기지 귀환 목표 (2026-10-11)

`type: extraction_return`은 추출 지점의 대기 완료 후 **실제 기지 맵 로드 완료 시점**에 진행한다. `source_level`, `target_level`, `required_count` 필터를 사용하며 기지 외 목적지는 인정하지 않는다. 에디터 PIE 접두사는 제거한 뒤 목적지 이름을 정확히 비교한다.

일반 이동 액터의 귀환, 사망 후 구조 귀환, 이동 실패, 다른 맵 도착은 완료시키지 않는다. 출발 시 이미 수락한 목표만 기록하고 도착 시 한 번만 반영한다. 도중 저장 불러오기·새 게임·정의 재로드는 대기 기록을 폐기한다. 완료 목표는 기존 저장 체계로 저장하며, 보상 수령은 기존 제공/완료 NPC 흐름을 유지한다. 일반 `level_travel`의 요청 시점 판정은 변경하지 않는다.

> 현재 상태(2026-10-10): 사용자의 퀘스트툴 재구성 요청에 따라 기존 데모 퀘스트 5개를 제거하여 공개 `QuestDefinitions.json`은 `[]`다. 기존 데모의 고정 자동 수락 및 최종 퀘스트 연동 엔딩도 해제했다. 아래 퀘스트 설명은 프레임워크와 과거 예시 참고용이며 현재 활성 퀘스트가 아니다. 새 데이터 작성 절차는 `Docs/quest_and_runtime_actor_data_authoring_guide.md`를 참고한다.

## 범위

이 문서는 TunaSweeper의 범용 퀘스트/목표 진행 프레임워크를 정리한다.

이번 범위는 5챕터 전체 콘텐츠 제작이 아니라, 기존에 C++로 하드코딩되어 있던 `quest_first_outing`을 데이터 기반 정의와 범용 목표 이벤트 처리로 일반화하는 코드 작업이다.

## 데이터 정의

퀘스트 정의 경로는 BuildFlavor가 결정한다. Demo 타깃은 작업 저장소의 `TunaSweeper/Content/Data/QuestDefinitions.json`을 사용하고, Main Editor/PIE는 접근 제한 `TunaSweeper/External/MainPayload/Data/QuestDefinitions.json` 원본을 직접 읽는다. Main 패키징에서는 검증된 임시 staging 사본을 읽는다. 수동 데이터셋 전환은 없으며 자세한 운영 규칙은 `Docs/Steam/TunaSweeper_Build_Flavor_Data_Architecture.md`를 따른다.

기본 필드:

- `quest_id`: 저장과 이벤트 매칭에 쓰는 전역 양의 `int32` ID(1~2147483647). JSON 숫자로 작성하고 화면에만 `003`처럼 최소 세 자리로 표시한다. 런타임의 미지정 값은 `0`이다.
- `provider_id`: 퀘스트를 제공하는 소스 ID. 현재 두더지는 `provider.mole`을 쓴다.
- `sort_order`: 같은 제공자 안에서 새 퀘스트 후보를 고를 때 쓰는 낮은 값 우선 순서.
- `required_completed_quest_ids`: 수락 가능해지기 전에 보상 수령까지 끝나야 하는 선행 퀘스트 ID 목록.
- `authoring_tags`: `chapter:1` 등의 저작용 분류 태그. QuestStudio의 챕터 필터와 경계 표시에만 사용하며 현재 UE 로더는 이 필드를 실행 데이터로 해석하지 않는다. 게임의 선행 조건·목표·보상·세이브 판정에 영향을 주지 않는다.
- `title`, `description`: 퀘스트 UI와 HUD 추적에 표시할 텍스트.
- `auto_track_on_accept`: 수락 시 HUD 추적 대상으로 자동 설정할지 여부.
- `objectives`: 목표 목록.
- `rewards`: 코인과 아이템 보상.

아래 문자열 ID 예시는 제거된 데모의 과거 기록이며 신규 데이터에 사용하지 않는다. 현재 숫자 ID 작성 규칙은 위 정의를 따른다.

과거 첫 퀘스트:

| 항목 | 값 |
| --- | --- |
| QuestId | `quest_first_outing` |
| ProviderId | `provider.mole` |
| 제목 | `첫 외출` |
| 목표 | `leave_bunker` |
| 목표 타입 | `level_travel` |
| 달성 조건 | `BunkerMap -> RaidMap` |
| 보상 | 코인 100 |

두더지 후속 적 처치 퀘스트:

| 항목 | 값 |
| --- | --- |
| QuestId | `quest_lumberjack_first_kill` |
| ProviderId | `provider.mole` |
| 선행 조건 | `quest_first_outing` 보상 완료 |
| 제목 | `벌목기 제거` |
| 목표 | `kill_lumberjack` |
| 목표 타입 | `enemy_killed` |
| 달성 조건 | `enemy.lumberjack` ID를 가진 적 처치 |
| 보상 | 코인 150 |

두더지 액터는 고정된 퀘스트 하나만 열지 않고 `provider.mole`에 연결된 퀘스트 중 현재 플레이어 상태에 맞는 항목을 선택한다. 보상 수령 가능 퀘스트가 가장 우선이고, 진행 중 퀘스트, 선행 조건을 만족한 신규 퀘스트 순서로 선택한다.

provider 체인에 보상 수령 가능, 진행 중, 신규 수락 가능 퀘스트가 모두 없으면 해당 퀘스트 상호작용은 마커와 포커스 후보에서 제외한다. 완료된 마지막 퀘스트를 다시 보여주는 동작은 두더지 퀘스트 상호작용 흐름에 연결하지 않는다.

`UTunaSweeperQuestSubsystem::TryGetLatestQuestInProviderChain`은 진행 중이거나 보상 가능하거나 이미 보상 완료된 provider 체인의 최종 퀘스트를 조회하는 내부용 함수다. 현재 UI/상호작용 흐름에서는 호출하지 않고, 나중에 퀘스트 로그나 회상 UI가 필요할 때 사용할 수 있도록 둔다.

워프 포인트 사용 목표 검증용 데이터:

| 항목 | 값 |
| --- | --- |
| QuestId | `quest_warp_point_used_test` |
| ProviderId | 없음 |
| 제목 | `워프 포인트 사용 확인` |
| 목표 | `use_raid_warp_point_a` |
| 목표 타입 | `warp_point_used` |
| 달성 조건 | `RaidMap`에서 `raid_warp_point_a -> raid_warp_point_b` 워프 성공 |
| 실제 사용 여부 | 제공자에 연결하지 않아 현재 플레이 흐름에서는 제공되지 않음 |

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
- 코인 재화 잔액 `QuestCoinBalance`

세이브 슬롯을 바꾸거나 새 게임을 시작하면 퀘스트 진행, 추적 대상, 코인 재화가 슬롯별로 분리된다.

## 목표 이벤트

v1 목표 타입:

- `level_travel`: 레벨 이동 요청 성공. `source_level`, `target_level`로 필터링한다.
- `item_acquired`: 플레이어가 아이템을 획득했을 때. `item_id`가 비어 있으면 모든 아이템을 허용한다.
- `item_submitted`: 지정 NPC에게 아이템을 제출했을 때. 양의 정수 `item_id`, `required_count`와 비어 있지 않은 `target_provider_id`를 명시한다. 획득/일반 상호작용 이벤트로 완료되지 않는다.
- `enemy_killed`: 플레이어가 적을 처치했을 때. 선택적 `enemy_id`와 `enemy_class_paths`는 함께 적용하며, 클래스 경로는 실제 적 클래스와 정확히 일치해야 한다. 선택적 Boolean `requires_vehicle_collision`의 기본값은 `false`다. `true`이면 살아 있는 플레이어 운전자가 탑승한 ATV의 충돌 피해로 처치해야 진행된다. 총격, 다른 AI의 처치, 이미 죽은 적에 대한 반복 피해는 충돌 처치로 인정하지 않는다. 이 필드는 `enemy_killed`에서만 허용하며 Boolean 이외의 JSON 값은 로더 오류로 처리한다.
- `interaction_completed`: 상호작용 처리가 성공했을 때. `interaction_event_id`나 `interaction_type`으로 필터링한다.
- `warp_point_used`: 워프 포인트 액터가 플레이어 워프를 성공시켰을 때. `source_level`, `warp_point_id`, `target_warp_point_id`로 필터링한다. `source_level`은 워프가 발생한 현재 맵을 의미한다.

후송 복귀 목표 타입:

- `bunker_rescue_return`: 작전 중 이상 상태 후 구급 카트 후송 절차로 벙커에 돌아온 상황을 표현한다. `source_level`, `target_level`로 필터링한다. 현재 플레이어 체력이 0이 되어 벙커 복귀 전환을 시작하는 시점에 `ATunaSweeperTopDownCharacter::HandleDeath`에서 감지 이벤트를 보낸다. UI 문구와 퀘스트 설명에서는 직접적인 실패 표현을 쓰지 않고 `구급 카트에 실려 벙커로 돌아오기`, `후송 절차로 복귀`처럼 완곡하게 표현한다.

현재 연결 지점:

- `ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel`
- `UTunaSweeperGameInstance::AddItemToFirstAvailableInventorySlot`
- `UTunaSweeperGameInstance::AddItemToPreferredAvailableSlot`
- `UTunaSweeperGameInstance::MoveItemBetweenSlots`의 루트 컨테이너 획득 경로
- `ATunaSweeperEnemyCharacter::HandleDeath`
- `UTunaSweeperInteractionSubsystem::RequestInteraction`
- `ATunaSweeperTopDownCharacter::HandleDeath`의 구급 카트 후송 복귀 경로
- `ATunaSweeperWarpPointActor::WarpInstigator`

이벤트는 `Accepted` 상태의 퀘스트에만 진행도를 반영한다. 목표 진행도는 목표별 `RequiredCount`까지만 증가하고, 모든 목표가 완료되면 상태가 `RewardAvailable`로 바뀐다.

## UI와 보상

### 아이템 제출

```json
{
  "objective_id": "deliver_canned_tuna",
  "type": "item_submitted",
  "text_string_key": "quest.demo_q4.objective.deliver",
  "item_id": 3004,
  "required_count": 1,
  "target_provider_id": "provider.mole"
}
```

두더지와 시설 NPC의 퀘스트 상호작용은 실제 액터의 퀘스트 제공자 ID를 공용 `TrySubmitItemsToProvider`에 전달한다. 퀘스트 발급자와 제출 대상은 서로 달라도 된다. 수락한 퀘스트 중 해당 대상으로 제출할 목표를 찾아, 한 번의 상호작용에서 퀘스트 하나를 처리한다. 보상 대기 중인 제출 퀘스트를 먼저 확인하고 나머지는 `sort_order`, 퀘스트 ID 순으로 확인한다.

선택한 퀘스트에서 해당 대상에게 아직 제출하지 않은 수량을 모두 요구한다. 같은 아이템을 여러 목표가 요구하면 합산한다. 인벤토리 → 보조 가방 → 창고 순서로 물품을 사용한다. 세 곳의 수량을 합산하며 장착 슬롯과 물품 수령함은 제외한다. 수량이 부족하면 그 퀘스트는 아무것도 소모하거나 진행시키지 않는다. 아이템 소모와 목표 진행을 모두 갱신한 뒤 인벤토리·퀘스트 변경 알림과 저장 요청을 보낸다.

목표를 모두 달성하면 기존 보상 수령 경로를 호출한다. 퀘스트 보상 중 인벤토리에 들어가지 못한 수량은 물품 수령함에 저장하므로 공간 부족으로 퀘스트 보상 완료가 막히지 않는다. 일반 퀘스트 패널에서도 기존 보상 버튼을 사용할 수 있다.

데모 참치캔 목표는 제출 조건을 위 데이터에서 읽는다. 보상은 기존 `rewards`의 참치캔 1개와 코인 250을 유지한다. 기존 목표 ID를 유지하므로 이미 제출한 세이브의 진행도도 보존한다. 데모 엔딩 액터는 아이템 보유 검사·소모를 하지 않으며, 마지막 퀘스트 보상 수령 후 기존 엔딩 연결이 실행된다.

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
- `enemy_killed` 목표에서 특정 적만 요구하려면 적 스폰 데이터에 `enemy_id`를 추가한다. 현재 벌목기 스폰은 `enemy.lumberjack`을 사용한다.
- `interaction_completed` 목표에서 특정 상호작용만 요구하려면 해당 `UTunaSweeperInteractableComponent::ObjectiveEventId`를 설정한다.
- `warp_point_used` 목표에서 특정 워프만 요구하려면 워프 포인트 스폰 데이터의 `id`와 `target_id`를 각각 `warp_point_id`, `target_warp_point_id`에 설정한다.
- `bunker_rescue_return`은 직접적인 실패 표현 대신 구급 카트 후송/복귀 표현을 사용한다.
- 저장 필드를 변경하면 `Docs/save_persistence.md`도 같은 변경에서 갱신한다.

## Quest UI and Presentation Update

- Quest panel display now uses the shared HUD `Quest` mode instead of a standalone popup widget.
- The panel shows a detail view on the left and a quest list on the right.
- The right list is split into `Available`, `InProgress`, and `RewardCompleted` tabs.
- Quest text is string-key based. `Content/Data/QuestDefinitions.json` stores keys such as `title_string_key`, `description_string_key`, and objective `text_string_key`; localized text lives in `Content/Data/QuestTextStrings.csv`.
- Quest presentation steps can be attached to a quest through `accept_presentation` or `reward_presentation`.
- A presentation step uses `speaker_name_string_key` and `dialogue_text_string_key`; optional `use_camera_focus`, `camera_focus_location`, and `camera_blend_seconds` reuse the existing dialogue camera flow.
- Accepting a quest calls `PlayQuestPresentation(..., OnAccept)` after the quest is accepted. Claiming rewards can similarly call `OnRewardClaim` when reward presentation data exists.
- Quest rewards can include `housing_facilities` or `housing_facility_unlocks` arrays. Reward claim unlocks those facility ids in `UTunaSweeperGameInstance`, and the housing UI only shows locked facilities after their ids are unlocked.

### 수락 즉시 완료하는 전달형 퀘스트

`complete_on_accept: true`를 명시하고 `objectives`는 비워 둔다. 수락 시 `RewardAvailable`이 되며 보상은 자동 지급하지 않는다. `completion_provider_id`로 지정한 NPC에서 보상을 수령해야 `RewardCompleted`가 되어 후속 퀘스트가 해금된다. 목표와 해당 옵션의 혼용은 로더 오류로 처리한다. 옵션이 없는 목표 미정의 퀘스트는 계속 실행 등록에서 제외한다.

### 역·차량 도착 트리거

`ATunaSweeperQuestArrivalTrigger`를 네이티브 액터로 배치하거나 BP 자식으로 사용한다. `ArrivalArea` 박스는 플레이어 중심 위치를 검사하며 회전·크기·높이를 반영한다. 게임 중 충돌과 길찾기를 방해하지 않는다. 목적지마다 안정적인 `LocationId`를 지정하고, 다른 장소에 같은 ID를 재사용하지 않는다.

- 탑승물 조건 `VehicleCondition=None`: `location_reached` 목표에 보고한다. 도보 또는 탑승 중인 실제 로컬 플레이어가 영역 안에 있으면 인정한다. 빈 열차·차량과 NPC만 도착한 경우에는 진행되지 않는다.
- 탑승물 조건 `VehicleCondition=ATV`: `vehicle_location_reached` 목표에만 보고한다. 살아 있는 로컬 플레이어가 파괴되지 않은 ATV의 실제 좌석에 부착되어 있어야 한다. 선택 항목 `RequiredVehicleTag`로 배치 차량을 제한할 수 있다. 조건은 BP와 배치 인스턴스에서 enum으로 선택하며 기본값은 `None`이다.
- `RequiredCompletedQuestIds`: 모든 지정 퀘스트가 보상 수령 완료(`RewardCompleted`)여야 한다. `RequiredCompletedProgressIds`: 모든 지정 월드 진행 ID가 `Completed`여야 한다. 빈 배열은 제한 없음이며 잘못된 ID·없는 상태는 판정을 차단한다. 복구/급유 완료 조건을 연결할 수 있지만 실시간 연료 잔량을 검사하는 기능은 아니다.
- 매 프레임 물리 갱신 이후 현재 플레이어 위치를 확인한다. 이미 영역 안에서 수락·로드·탑승·선행 완료가 일어나도 다음 판정에 반영한다. 영역을 통과한 과거 경로를 추정하지 않으므로 고속 차량 경로에는 프레임 간 이동보다 충분히 넓은 영역을 배치한다.

목표는 아래처럼 연결한다. 예시 ID·문자열 키는 설명용이며 실제 데이터에 등록하지 않았다. UI 문구는 해당 데이터셋 문자열 테이블에 등록한 키로 연결한다.

```json
{
  "objective_id": "visit_east",
  "type": "location_reached",
  "location_id": "example.station.east",
  "required_count": 1,
  "text_string_key": "example.objective.visit_east"
}
```

차량 목표는 `type`을 `vehicle_location_reached`로 바꾼다. 두 타입은 서로의 목표를 진행시키지 않는다. 두 역 방문은 각기 다른 `location_id`를 갖는 목표 두 개로 작성한다. 도착 목표의 `required_count`는 생략 또는 1만 허용하며 장소 없는 정의·반복 횟수·잘못된 숫자 형식은 거절한다. 수락 중인 퀘스트만 진행하고 같은 장소의 재판정은 중복 저장·보상 지급을 일으키지 않는다. 저장은 기존 목표 진행값을 사용한다.

재사용 BP는 `/Game/Interaction/BP_QuestArrivalTrigger`다. 마을의 세 승강장에는 역별 도착 영역을 배치하며, 연결되는 퀘스트 목표와 문자열은 해당 데이터셋에서 관리한다. 복구/연료 시스템은 이 트리거가 직접 구현하지 않는다. 편집기에서 네이티브 `TunaSweeperQuestArrivalTrigger` 액터를 직접 배치해 설정할 수도 있다.

BP의 `ArrivalFlagIcon` Billboard는 레벨 뷰포트에서 흰색 깃발로 위치를 표시한다. 에디터 전용·게임 중 숨김·충돌 없음으로 설정한다. 텍스처 `/Game/Editor/Icons/Flag`는 UE 5.7의 `PerformanceCaptureWorkflow/Content/Common/Icons/Flag.uasset`을 복사한 기존 엔진 리소스이며 플러그인 활성화 없이 사용한다.

### 퀘스트 조건 시설 액터

- 콘텐츠 브라우저의 `/Game/Interaction/QuestFacility/BP_QuestFacility`를 배치한다. 기반 클래스는 `ATunaSweeperQuestFacilityActor`이며 648삼각형 복셀 새끼 멧돼지 임시 메시를 기본으로 사용한다. `VisualMesh`는 시설 모델로 교체할 수 있다. 실제 시설 서비스나 특정 퀘스트 ID는 템플릿에 지정하지 않는다.
- `UnlockConditions`에 `QuestId`와 `AllowedStates`를 추가한다. 모든 항목이 만족되어야 상호작용할 수 있다. 기본 허용 상태는 `RewardCompleted`(보상 수령 완료)다. 빈 조건, 존재하지 않는 ID, 0 이하 ID, 빈 허용 상태 목록은 잠금으로 처리한다.
- 진행 중에만 쓰는 시설은 `Accepted`, 목표 달성 후 보고 전에는 `RewardAvailable`을 선택한다. 수락 이후 계속 열어 두려면 `Accepted`, `RewardAvailable`, `RewardCompleted`를 모두 넣는다. 상태는 정확히 일치해야 하며 자동 누적 조건이 아니다.
- 잠금 중에도 메시·충돌은 유지하고 상호작용 프롬프트만 제거한다. 실행 시 조건을 다시 검사하므로 오래된 프롬프트로 우회할 수 없다.
- `InteractionStringKey`는 UI 문자열 키이며 기본은 `ui.interaction.use_facility`다. 표시 문구를 코드에 직접 쓰지 않는다.
- BP에서 `ExecuteFacilityInteraction`을 재정의해 시설 UI나 서비스를 연결한다. 성공 시 true를 반환하면 `OnFacilityUsed`가 발생하고, 일반 상호작용 경로에서 `InteractionObjectiveEventId` 및 `quest_facility_use` 타입으로 퀘스트 목표에 알린다. 실패는 false로 반환한다. 기본 템플릿은 성공 이벤트만 제공하며 서비스·아이템 소모·추가 해금 상태를 만들지 않는다.
- 최초 진입·퀘스트 변경·저장 불러오기·언어 변경 때 갱신한다. 실행 중 BP로 조건을 편집하면 `RefreshQuestState`를 호출한다.

### 퀘스트 완료 조명 제어 액터

- `/Game/Interaction/QuestFacility/BP_QuestLightController`를 제어할 조명과 같은 레벨에 배치한다. 기반 클래스는 `ATunaSweeperQuestLightController`다.
- 인스턴스의 `LightGroups` 각 행에 `CompletedQuestId`와 `LightActors` 목록을 지정한다. 레벨에서 조명 액터를 선택하거나 스포이트로 참조를 연결한다. 한 액터의 여러 조명 컴포넌트도 함께 제어한다.
- `bVisibleBeforeCompletion`과 `bVisibleAfterCompletion`은 기본 false/true다. 반대로 설정하면 완료 후 소등한다. 완료는 `RewardCompleted`이며 `RewardAvailable`은 완료 전으로 취급한다. 잘못된 ID나 아직 없는 퀘스트는 완료 전 상태를 사용한다.
- Movable 조명을 권장한다. Static 조명은 건드리지 않으며, Stationary 조명의 이미 구워진 간접광은 이 액터로 제거할 수 없다. 조명 컴포넌트의 Visibility만 바꾸고 밝기·색·메시·액터의 Hidden In Game 값은 유지한다. 대상 조명 자체의 Hidden In Game은 꺼 두어야 한다.
- 같은 조명을 여러 행에 넣으면 아래쪽 행이 우선한다. 여러 컨트롤러가 같은 조명을 제어하지 않도록 배치한다. null·삭제된 참조는 무시한다.
- BeginPlay와 퀘스트 변경·불러오기·새 게임 초기화 이벤트에서 갱신하며 매 프레임 순회하지 않는다. 스트리밍 서브레벨마다 컨트롤러를 함께 배치한다. 실행 중 목록이나 조명 컴포넌트를 바꾸면 `RefreshQuestLights`를 호출한다. 에디터 구성 단계에서는 레벨 조명을 변경하지 않는다.

### 상점 상품별 퀘스트 완료 해금

- `Content/Data/ShopDefinitions.json`의 상품 항목에 선택 필드 `required_completed_quest_ids`를 넣는다. 예: `"required_completed_quest_ids": [900001, 900002]` (설명용 ID이며 실제 데이터는 변경하지 않음).
- 모든 지정 퀘스트가 존재하고 보상 수령 완료(`RewardCompleted`)여야 목록에 표시되고 구매할 수 있다. 수락(`Accepted`)과 보상 대기(`RewardAvailable`)는 해금하지 않는다. 필드 생략 또는 빈 배열은 기존처럼 제한 없는 판매다.
- ID는 JSON 숫자인 양의 int32여야 한다. 문자열·0·음수·소수·범위 초과·null 등 잘못된 조건은 상점 데이터 로드를 실패시킨다. 잘못된 상품만 건너뛰어 저장 재고의 슬롯 번호가 밀리는 동작은 하지 않는다. 중복 조건 ID는 하나로 정리한다. 형식은 유효하지만 현재 퀘스트 데이터셋에 없는 ID는 잠금 상태다.
- 잠긴 상품은 UI 목록에서 제외하지만 원본 배열의 `SlotIndex`는 유지한다. 구매·상세 조회에서도 조건을 재검사하므로 예전 UI 슬롯이나 직접 구매 호출로 우회할 수 없다. 잠금 실패 시 아이템·코인·재고를 변경하지 않는다.
- 열린 상점 UI는 퀘스트 변경·저장 복원·새 게임 초기화 알림에서 즉시 목록을 갱신한다. 기존 상품의 판매, 가격, 재고 수량, 상점 이용 장소 제한은 유지한다. 이 기능은 상품 조건만 제공하며 실제 상품·가격·퀘스트 ID 연결이나 자판기 이전 배치를 추가하지 않는다.


### 소유 상태 기반 제출과 물품 수령함

`item_possessed`는 양의 정수 `item_id`, `required_count`, 제출 NPC의 `target_provider_id`를 요구한다. 완료 NPC(`completion_provider_id`, 생략 시 `provider_id`)를 제출 대상과 동일하게 설정한다. 수락 시 이미 보유한 물품도 인정하며 인벤토리·보조 가방·창고의 현재 수량으로 진행값을 다시 계산한다. 같은 아이템의 여러 목표는 합산 수량이 있어야 모두 완료된다.

전체 목표를 만족하는 순간 보상 대기 상태와 완료 토스트를 표시한다. 제출 전에 사용·판매·버리기 등으로 수량이 부족해지면 다시 진행 중 상태가 된다. NPC에게 제출/보상 수령할 때 소유 목표의 요구량을 일괄 차감하고 저작된 보상을 지급한다. 소유 목표는 획득 이벤트를 누적하지 않는다. 완료 후 보상을 다시 받을 수 없고, 저장 복원 시 이미 보상 완료된 기록은 그대로 유지한다.

`/Game/Interaction/BP_QuestRewardLocker`는 기지에서 F로 여는 물품 수령함이다. UI는 아이콘·이름·수량과 항목별 받기 버튼을 그리드로 표시하며 넣기/드래그 입고 기능은 없다. 보상은 가능한 수량을 인벤토리에 먼저 넣고 남은 수량을 일반 스택 단위로 보관한다. 받기는 선택 스택 전체가 들어갈 때만 실행하며 공간 부족 시 보관 항목을 유지하고 현지화 토스트를 표시한다. 미수령 보상은 제출 소유량에 포함하지 않는다. ESC·상호작용 거리 이탈·사망·보관함 소멸 시 UI를 닫는다.

기지 배치: `Bunker_QuestRewardLocker`, 소파 뒤 벽면 스크린 위치 (235.436851, 331.808671, 0)cm, yaw -118.196304. 소파에 앉은 방향의 오른쪽에 별도 배출구를 두며 곡면 벽에 맞춘 `OutletTransform`을 액터 속성으로 저장한다. 내부 식별자와 수령 전용 기능은 유지한다.


### 목표 없는 퀘스트의 표시 전용 안내

`objective_display_string_key`는 실제 목표 목록이 없는 퀘스트의 상세 목표 영역에 표시하는 선택적 문자열 키다. 기존 퀘스트 문자열 및 추가 번역 테이블에서 현재 언어로 해석한다. 이 필드는 목표 진행·완료 판정·NPC 보상 수령·저장 데이터에 관여하지 않으며, 실제 목표가 있으면 기존 목표 진행 표시를 우선한다. 키가 없으면 기존 빈 목표 표시를 유지한다.

### 완료 효과의 보상 안내

퀘스트 rewards의 선택 필드 display_string_key는 퀘스트 문자열 테이블에서 해석해 보상 영역에 표시한다. 아이템·코인·시설 보상과 함께 표시할 수 있으며, 문자열 자체가 게임 상태를 변경하지는 않는다. 완료 상태를 사용하는 시스템의 효과를 안내하는 용도다.
