# 퀘스트 및 JSON 런타임 액터 데이터 작성 안내서

## 목적

이 문서는 현재 빈 상태인 퀘스트와 런타임 액터 배치 데이터를 나중에 다시 작성하는 절차를 설명한다. 기능 코드와 에셋은 보존되어 있으므로 JSON/CSV 행을 추가하고 새 게임 세션에서 확인하면 된다.

## 공통 원칙

- JSON 파일의 최상위 값은 항상 배열이다. 비어 있을 때는 `{}`나 빈 파일이 아니라 `[]`를 사용한다.
- `level_name`은 실제 월드 이름과 일치해야 한다. 현재 기본 이름은 `BunkerMap`, `RaidMap`이다.
- 위치와 회전의 단위/방향은 `Docs/game_conventions.md`를 따른다. 위치는 UE 센티미터이고 배열 순서는 `[X, Y, Z]`다.
- 클래스/에셋 경로는 Unreal의 Copy Reference로 얻은 경로를 사용한다. Blueprint 클래스는 보통 `...BP_Name.BP_Name_C`처럼 `_C`가 붙는다.
- `quest_id`, `objective_id`, `spawn_id`, `enemy_id`, `object_id` 같은 식별자는 저장 데이터와 이벤트 연결에 사용되므로 배포 후 이름을 바꾸지 않는다.
- 데이터 변경 뒤 이미 실행 중인 PIE에 핫 리로드될 것으로 가정하지 않는다. PIE를 종료하고 필요하면 에디터를 재시작한 뒤 새 세션에서 확인한다.

## 퀘스트 다시 추가하기

### 1. 빌드 타깃별 데이터 위치 확인

Demo 타깃은 다음 두 공개 파일을 읽는다.

- `TunaSweeper/Content/Data/QuestDefinitions.json`
- `TunaSweeper/Content/Data/QuestTextStrings.csv`

Main 타깃의 퀘스트 원본은 접근 제한 `TunaSweeper/External/MainPayload/Data/`에 작성한다. 수동 데이터셋 전환은 없으며 에디터 Build Target이 Demo/Main을 결정한다. 패키징 및 격리 규칙은 `Docs/Steam/TunaSweeper_Build_Flavor_Data_Architecture.md`를 따른다.

### 2. 문자열 추가

`QuestTextStrings.csv`에 한·영·일 문자열을 먼저 추가한다.

```csv
string_key,ko,en,ja
quest.demo.first.title,첫 임무,First Mission,最初の任務
quest.demo.first.description,첫 임무 설명,First mission description,最初の任務の説明
quest.demo.first.objective,레이드 구역 확인,Inspect the raid zone,レイド区域を確認
```

기존 `quest.ui.*` 행은 공용 UI 문자열이므로 유지한다. `string_key`는 중복시키지 않는다.

### 3. 퀘스트 정의 추가

`QuestDefinitions.json`의 배열에 퀘스트 객체를 추가한다.

```json
[
  {
    "quest_id": "demo_first_mission",
    "provider_id": "provider.mole",
    "sort_order": 10,
    "title_string_key": "quest.demo.first.title",
    "description_string_key": "quest.demo.first.description",
    "auto_track_on_accept": true,
    "objectives": [
      {
        "objective_id": "enter_raid",
        "type": "level_travel",
        "text_string_key": "quest.demo.first.objective",
        "required_count": 1,
        "source_level": "BunkerMap",
        "target_level": "RaidMap"
      }
    ],
    "rewards": {
      "coins": 50,
      "items": []
    }
  }
]
```

주요 선택 필드는 다음과 같다.

- `required_completed_quest_ids`: 선행 퀘스트 ID 배열
- `accept_presentation`, `reward_presentation`: 화자/대사/카메라 연출 단계
- `rewards.items`: `{ "item_id": 1001, "quantity": 1 }` 형식의 아이템 보상
- `rewards.housing_facility_unlocks`: 시설 ID 배열
- `rewards.workbench_recipe_unlocks`: 제작법 ID 배열

지원 목표 타입은 `level_travel`, `item_acquired`, `enemy_killed`, `interaction_completed`, `warp_point_used`, `bunker_rescue_return`이다. 각 타입의 필터 필드는 `Docs/quest_system.md`를 따른다.

### 4. 제공자 액터 연결 확인

`provider.mole` 퀘스트를 쓰려면 두더지 제공자 액터가 월드에 있어야 한다. 현재 `BunkerCharacterSpawns.json`도 비어 있으므로 아래 런타임 액터 절차로 제공자 액터를 먼저 복구하거나 다른 제공자 연결을 구현한다.

## 런타임 액터 다시 추가하기

### 파일별 역할과 필수 필드

| 파일 | 역할 | 최소 필드 |
| --- | --- | --- |
| `BunkerCharacterSpawns.json` | 벙커 동료 캐릭터 | `level_name`, `spawn_id`, `location` |
| `EnemySpawns.json` | 적 | `level_name`, `location` |
| `LootContainerSpawns.json` | 고정 루트 컨테이너 | `level_name`, `location`, `container_definition_id`, `contents_id` |
| `GameplayInteractionSpawns.json` | 상호작용/게임플레이 액터 | `level_name`, `spawn_id`, `spawn_type`, `location` |
| `MemoSpawns.json` | 수집 메모 액터 | `level_name`, `memo_id`, `location` |
| `TransparentObstacleSpawns.json` | 투명 장애물 | `level_name`, `obstacle_id`, `location` |
| `WarpPointSpawns.json` | 양방향/단방향 워프 | `level_name`, `warp_point_id`, `target_warp_point_id`, `location` |
| `WorldProgressObjectSpawns.json` | 재료 투입형 월드 진행 오브젝트 | `level_name`, `object_id`, `info_id`, `location` |

모든 파일은 `TunaSweeper/Content/Data` 아래에 있다. `rotation`, `scale`, 클래스 경로와 타입별 세부 필드는 선택 또는 타입별 추가 필드다.

### 벙커 캐릭터 예시

```json
[
  {
    "level_name": "BunkerMap",
    "spawn_id": "TS_Bunker_Mole",
    "actor_class": "/Game/Characters/Mole/BP_Mole.BP_Mole_C",
    "location": [700.0, -320.0, 0.0],
    "rotation": [0.0, 180.0, 0.0],
    "scale": [1.0, 1.0, 1.0]
  }
]
```

### 적 예시

```json
[
  {
    "level_name": "RaidMap",
    "enemy_id": "enemy.demo_guard",
    "combat_profile_id": "enemy.rifle_anchor",
    "location": [1200.0, 300.0, 90.0],
    "rotation": [0.0, 180.0, 0.0]
  }
]
```

`combat_profile_id`는 `EnemyCombatProfiles.json`에 존재해야 한다. 특정 처치 퀘스트가 이 적을 구분해야 할 때 `enemy_id`를 반드시 지정한다.

### 게임플레이/상호작용 액터 예시

```json
[
  {
    "level_name": "BunkerMap",
    "spawn_id": "TS_Travel_DeployToRaid",
    "spawn_type": "level_travel",
    "actor_class": "/Game/Interaction/BP_Interact_LevelTravel.BP_Interact_LevelTravel_C",
    "location": [580.0, 360.0, 4.0],
    "rotation": [0.0, 0.0, 0.0],
    "target_level_name": "RaidMap"
  }
]
```

정식 `spawn_type` 값은 다음과 같다.

- `level_travel`, `pickup_item`, `item_spawn`
- `loot_container`, `loot_container_spawn`
- `shop`, `workbench`, `piggy_bank`
- `periodic_noise_emitter`, `difficulty_adjustment`, `self_destruct`
- `rolling_bomber_spawner`, `extraction_point`
- `sandbag_cover`, `explosive_barrel`, `static_mesh_prop`
- `shooting_practice_dummy`

타입별 상세 필드와 지도 오버레이 설정은 `Docs/runtime_actor_spawns.md`를 참고한다.

### 메모 예시

```json
[
  {
    "level_name": "RaidMap",
    "memo_id": 1,
    "spawn_id": "memo_demo_001",
    "location": [520.0, -360.0, 36.0]
  }
]
```

같은 `memo_id`가 `MemoDefinitions.json`에 있어야 한다. 이미 획득한 메모 ID는 저장 상태 때문에 다시 나타나지 않을 수 있으므로 새 저장 슬롯으로 확인한다.

### 연결 데이터 주의사항

- 루트 컨테이너의 `container_definition_id`와 `contents_id`는 각각 컨테이너/내용 정의에 존재해야 한다.
- 워프는 대상 `target_warp_point_id` 행이 실제로 존재하는지 확인한다.
- 월드 진행 오브젝트의 `required_item_id`는 `ItemTable.json`에 존재해야 한다.
- `static_mesh_prop`은 추가로 `static_mesh`가 필요하다.
- 같은 `spawn_id`를 중복 사용하지 않는다.

## 최소 검증 절차

데이터 작업에는 다음 정도만 수행한다.

1. 수정한 JSON이 파싱되고 최상위 배열인지 확인한다.
2. 같은 파일 안의 안정 ID가 중복되지 않았는지 확인한다.
3. 참조한 클래스/에셋 경로와 연결 ID가 존재하는지 확인한다.
4. 해당 맵 하나를 PIE로 열어 예상 수량만 스폰되는지 확인한다.
5. 퀘스트는 새 저장 슬롯에서 수락, 목표 1회 진행, 보상 수령 흐름만 확인한다.

전체 회귀 테스트나 대량 자동화 테스트는 콘텐츠 행 추가만으로는 만들지 않는다. C++ 로더나 저장 스키마를 바꿀 때만 관련 빌드/테스트 범위를 늘린다.

## 빈 상태로 되돌리기

특정 범주의 배치를 전부 끄려면 해당 JSON을 `[]`로 바꾼다. 퀘스트를 전부 끄려면 `QuestDefinitions.json`을 `[]`로 만들고, 사용하지 않는 퀘스트 콘텐츠 문자열만 CSV에서 제거한다. `quest.ui.*` 공용 문자열은 유지한다.
