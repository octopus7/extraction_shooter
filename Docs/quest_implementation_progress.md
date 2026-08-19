# Quest Implementation Progress

Last updated: 2026-05-25 17:34:26

이 문서는 TunaSweeper 퀘스트 시스템 구현 진행상황을 추적한다. 상세 설계는 `Docs/quest_system.md`를 기준으로 보고, 이 문서는 완료/남은 작업/결정 필요 항목만 정리한다.

## 완료된 것

- 체험판 데모 퀘스트 데이터가 `TunaSweeper/Content/Data/QuestDefinitions.json`과 `TunaSweeper/Content/Data/QuestTextStrings.csv`의 Q1~Q4 체인으로 갱신되었다. Q3-1/Q3-2는 Q2 완료 후 병렬 해금되고, Q4는 두 수리 퀘스트의 보상 완료 후 해금된다.
- 체험판 조사·청소·수리·참치캔 전달 목표는 `interaction.demo.*` 이벤트 ID를 사용하도록 등록되었다. 해당 이벤트를 실제 월드 인터랙션 액터가 발생시키는 연결은 남은 작업이다.

- 퀘스트 정의는 `TunaSweeper/Content/Data/QuestDefinitions.json`에서 데이터 기반으로 읽는다.
- 퀘스트 런타임 소유자는 `UTunaSweeperQuestSubsystem`이다.
- 퀘스트 상태 `Available`, `Accepted`, `RewardAvailable`, `RewardCompleted`가 구현되어 있다.
- 퀘스트 상태, 목표별 진행도, HUD 추적 퀘스트, 코인 재화 잔액이 세이브 슬롯별로 저장된다.
- 퀘스트 UI는 수락, 진행 상태 표시, 보상 수령, 완료 상태 표시를 지원한다.
- HUD는 추적 중인 퀘스트의 목표 진행도를 표시한다.
- 코인 보상이 구현되어 있다.
- 아이템 보상 정의가 있으며, 보상 완료 전에 인벤토리 빈 칸을 확인한다.
- `provider.mole` 기준 퀘스트 제공자 선택이 구현되어 있다.
- 제공자 선택은 보상 가능 퀘스트, 진행 중 퀘스트, 신규 수락 가능 퀘스트 순서로 우선한다.
- `required_completed_quest_ids` 기반 선행 퀘스트 조건이 구현되어 있다.
- 제공자 체인에 표시할 퀘스트가 없으면 두더지 퀘스트 상호작용이 숨겨진다.
- 나중에 퀘스트 로그나 회상 UI에서 쓸 수 있는 provider 체인 최종 퀘스트 조회 함수가 있다.
- 목표 타입 `level_travel`이 구현되어 있고 `source_level`, `target_level` 필터를 지원한다.
- 목표 타입 `item_acquired`가 구현되어 있고 `item_id` 필터를 지원한다.
- 목표 타입 `enemy_killed`가 구현되어 있고 `enemy_id` 필터를 지원한다.
- 목표 타입 `interaction_completed`가 구현되어 있고 `interaction_event_id`, `interaction_type` 필터를 지원한다.
- 목표 타입 `bunker_rescue_return`이 구현되어 있고 `source_level`, `target_level` 필터를 지원한다.
- 목표 타입 `warp_point_used`가 구현되어 있고 `source_level`, `warp_point_id`, `target_warp_point_id` 필터를 지원한다.
- 레벨 이동 목표는 `ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel`에 연결되어 있다.
- 아이템 획득 목표는 `UTunaSweeperGameInstance`의 인벤토리 추가 경로에 연결되어 있다.
- 적 처치 목표는 `ATunaSweeperEnemyCharacter::HandleDeath`에 연결되어 있다.
- 상호작용 완료 목표는 `UTunaSweeperInteractionSubsystem::RequestInteraction`에 연결되어 있다.
- 구급 카트 후송 복귀 목표는 `ATunaSweeperTopDownCharacter::HandleDeath`에 연결되어 있다.
- 워프 포인트 사용 목표는 `ATunaSweeperWarpPointActor::WarpInstigator`의 워프 성공 경로에 연결되어 있다.
- 현재 퀘스트 데이터에는 `quest_first_outing`이 있다.
- 현재 퀘스트 데이터에는 `quest_lumberjack_first_kill`이 있다.
- 현재 퀘스트 데이터에는 제공자에 연결하지 않은 `quest_rescue_cart_return`이 있다.
- 현재 퀘스트 데이터에는 제공자에 연결하지 않은 `quest_warp_point_used_test`가 있다.
- 벌목기 처치 퀘스트는 `quest_first_outing` 보상 완료 후 진행되도록 체인 연결되어 있다.
- 벌목기 스폰 데이터는 `enemy.lumberjack`과 연결되어 있다.
- 현재 `quest_first_outing`과 `quest_lumberjack_first_kill`은 퀘스트 프레임워크와 초기 흐름 검증용 데이터다. 본편 메인 퀘스트 최종 기준은 `Docs/SSOT/TunaSweeper_SSOT_Quest_Item_v0.6.md`의 M01~M20 체인을 따른다.

## 해야 할 것

- 현재 두 개의 초기 퀘스트를 넘어 본편 진행용 실제 퀘스트 데이터를 추가한다.
- 설정된 지역에 도달하는 `visit_location` 목표 타입을 추가한다.
- 방문 목표는 `level_name`, `area_id`, `center`, `radius`, 선택적 `z_tolerance`를 저장한다.
- TunaSweeper가 탑다운 게임인 점을 고려해 방문 판정은 기본적으로 XY 2D 거리로 처리한다.
- gameplay map에서 플레이어 위치를 기준으로 방문 목표 진행도를 갱신한다.
- 레벨 배치 변경으로 방문 목표가 조용히 깨지지 않도록 데이터 검증이나 에디터/디버그 시각화를 추가한다.
- 필요하면 단순 레벨 이동과 생환 귀환을 구분하는 귀환/추출 목표를 추가한다.
- 수리/월드 진행 완료 목표 타입 또는 이벤트 훅을 추가한다.
- 부서진 다리 수리 같은 월드 진행 오브젝트 완료를 퀘스트 진행도와 연결한다.
- 참치 선반 등록, 좌표 등록, 보급 상자 등록 같은 반납/등록 목표를 추가한다.
- 리소스 획득이 아니라 반납이 필요한 퀘스트를 위해 선택적 아이템 소모 목표를 추가한다.
- 스캐너 기반 신호 추적과 숨겨진 보급함 탐색을 유지한다면 스캐너/신호 목표를 추가한다.
- 퀘스트가 시나리오 연출이나 엔딩 컷신 완료를 기다려야 한다면 시나리오/컷신 완료 목표를 추가한다.
- 보스전이 단순 named enemy 처치보다 별도 상태가 필요하면 보스 전용 목표를 추가한다.
- 오프닝 챕터를 퀘스트 기반으로 만들 경우 벙커 조사, 동면 장치 조사, 단말기 확인, 기본 무기 회수 퀘스트 데이터를 추가한다.
- `Docs/SSOT/TunaSweeper_SSOT_Quest_Item_v0.6.md` 기준 본편 메인 퀘스트 M01~M20과 선택/편의 퀘스트 S01~S20 데이터를 추가한다.
- M06~M08 중 2개 이상 완료 후 M09가 열리는 병렬 선행 조건을 데이터로 표현한다.
- 자판기 상품 해금 조건, `자판기 폭발물` 획득, 콘크리트 더미 영구 제거, 고급보안구역 진입, 보스 처치, 고급참치 확보, M20 일상 엔딩 상태를 퀘스트/월드 진행 데이터로 연결한다.
- 포탈/워프포인트는 메인 퀘스트에서 직접 안내하지 않는다. 워프포인트 목표 타입은 테스트/검증용 또는 별도 선택 목표에만 사용한다.
- 보급창고는 메인 관문이 아니라 선택형 S04/S05 편의 퀘스트로 구현한다.
- 시설 복구 1/2 진행축은 사용하지 않는다. 하우징 관련 해금은 선택 퀘스트 보상 또는 하우징 시설 해금으로 처리한다.
- 두더지 외 퀘스트 제공자가 필요하면 provider 데이터를 확장한다.
- 완료 퀘스트를 나중에 다시 볼 필요가 있으면 퀘스트 로그/히스토리 UI를 추가한다.
- 목표 매칭, 선행 조건, 보상 수령, 저장/로드 복구를 검증하는 테스트나 검증 경로를 추가한다.

## 결정이 필요한 것

- `visit_location`을 순수 데이터 좌표로만 둘지, 레벨 편집이 쉬운 트리거/마커 액터와 함께 쓸지 결정한다.
- 방문 목표의 기본 반경 값을 결정한다. 예: 정확한 오브젝트 조사, 작은 지역 도착, 넓은 구역 발견.
- 방문 판정에 시야 확보, 전투 중 제외 같은 조건이 필요한지 결정한다.
- `RaidMap -> BunkerMap` 레벨 이동만으로 귀환 목표를 처리할지, 생환 추출 전용 이벤트가 필요한지 결정한다.
- 구급 카트 후송 복귀 목표를 어떤 실제 퀘스트 체인에서 사용할지 결정한다.
- 수리 목표를 범용 `interaction_completed`로 처리할지, `world_progress_completed` 또는 `repair_completed` 같은 새 타입으로 분리할지 결정한다.
- 아이템 수집 퀘스트의 기준을 인벤토리 추가 시점으로 볼지, 루팅 컨테이너에서 루트 인벤토리로 이동한 시점을 계속 획득 기준으로 볼지 결정한다.
- 반납/등록 목표가 아이템을 소모할지, 인벤토리에 보유만 요구할지, 벙커 영구 상태로 이동시킬지 결정한다.
- 제작/작업대 목표를 유지할지 결정한다. 최신 범위 문서에서는 복잡한 제작을 줄이거나 제거하는 방향도 제안되어 있다.
- 스캐너/신호 추적을 실제 플레이 메커니즘으로 만들지, 상호작용/스토리 목표로만 처리할지 결정한다.
- 보스 목표에 보스 전용 상태와 UI가 필요한지, `enemy_killed`와 보스 `enemy_id`만으로 충분한지 결정한다.
- SSOT 지역 개방 15단계 중 첫 플레이 범위에 어느 정도를 메인 퀘스트로 구현할지 결정한다.
- 퀘스트 완료가 대화, 월드 상태 변경, 언락, 영상 재생을 직접 트리거할지, 별도 스토리 진행 레이어가 소유할지 결정한다.
