# 벙커 내부 시설과 NPC

이 문서는 `BunkerMap` 안에서 사용하는 시설, 시설 NPC, 단독 NPC의 역할과 상호작용 기준을 정리한다.

## 구분

### NPC 없는 시설

시설 액터 자체가 상호작용 지점이다.
하우징 모드에서는 메시와 배치 footprint만 보이고, 플레이 모드에서는 같은 시설 액터가 직접 상호작용을 제공한다.

현재 대상:

| 시설 id | 액터 | 역할 |
| --- | --- | --- |
| `housing_workbench` | `BP_Workbench` / `ATunaSweeperWorkbenchActor` | 제작, 분해, 설계도 등록 |
| `housing_piggy_bank` | `ATunaSweeperPiggyBankActor` | 옛 문명 동전/지폐 보관 |

### NPC가 붙는 시설

시설 액터는 배치 가능한 메시와 배치 기준만 가진다.
상호작용은 시설 메시가 아니라, 플레이 모드에서 시설 옆에 나타나는 NPC가 담당한다.

하우징 모드 규칙:

- 시설 메시만 보인다.
- NPC는 숨기거나 스폰하지 않는다.
- 하우징 모드 중 모든 상호작용 마커는 억제된다.

플레이 모드 규칙:

- 배치된 시설 옆에 NPC가 나타난다.
- 퀘스트/기능 진입은 NPC 상호작용에서 일어난다.
- 시설 메시 자체에는 기능 상호작용을 붙이지 않는다.

현재 대상:

| 시설 id | 시설 액터 | NPC | provider | 임시 퀘스트 | 역할 |
| --- | --- | --- | --- | --- | --- |
| `housing_signal_control` | `ATunaSweeperSignalControlFacilityActor` | `ATunaSweeperSignalBotActor` | `provider.signalbot` | `quest_signalbot_map_check` | 맵, 경로 정보, 워프포인트/순간이동 관리 |
| `housing_supply` | `ATunaSweeperSupplyFacilityActor` | `ATunaSweeperRicePotBotActor` | `provider.ricepotbot` | `quest_ricepotbot_supply_check` | 식량, 귀환 보상, 소모품 보급 |

### 단독 NPC

시설 배치와 무관하게 벙커에 존재하는 NPC다.
런타임 스폰 데이터나 별도 배치 규칙으로 생성되며, 특정 하우징 시설에 종속되지 않는다.

현재 대상:

| NPC | 스폰/배치 | provider | 역할 |
| --- | --- | --- | --- |
| 캔봇 | `BunkerCharacterSpawns.json`의 `TS_Bunker_LED_Robot` | `provider.canbot` | 벙커 기본 안내, 참치 선반 연출, 메인 퀘스트 접점 |

## 해금 규칙

최종 목표는 각 시설을 퀘스트 보상으로 해금하는 것이다.
현재 임시 규칙은 작업대 실제 배치를 기준으로 한다.

1. 플레이어가 하우징 모드에서 `housing_workbench`를 실제 배치한다.
2. 하우징 모드를 닫아 플레이 모드로 돌아오는 순간 `UTunaSweeperHousingSubsystem`이 배치 상태를 검사한다.
3. 배치된 작업대가 있으면 `housing_signal_control`, `housing_supply`를 순서대로 해금한다.
4. 각 시설 해금마다 토스트를 큐에 넣어 순차 표시한다.
5. 이미 해금된 시설은 다시 토스트를 띄우지 않는다.

이 임시 규칙은 실제 퀘스트 보상 체인이 준비되면 제거하거나 퀘스트 보상 기반 해금으로 대체한다.

## 저장

시설 배치와 보관 상태는 `UTunaSweeperSaveGame::HousingFacilities`에 저장된다.
시설 기능 해금 상태는 `UTunaSweeperSaveGame::UnlockedHousingFacilityIds`에 저장된다.

NPC 자체는 별도 저장 대상이 아니다.
NPC가 붙는 시설의 NPC는 배치된 시설 액터에서 플레이 모드 상태에 따라 런타임으로 나타나거나 사라진다.
