# 외부 식량 창고와 식당 엔딩

## 배치와 조절

- `/Game/Interaction/DemoEnding/BP_FoodWarehouse`: `ATunaSweeperFoodWarehouseActor` 기반. 현재 사용하는 `DemoBoxRaidMap` 시작 지점 근처에 배치했다. 다른 외부 맵을 사용하는 경우 그 맵에도 배치한다.
- `WarehouseMesh`의 Static Mesh를 바꾸면 외형을 교체할 수 있다. 생성자나 Construction Script가 메시를 다시 덮어쓰지 않는다. 교체 메시의 충돌과 상호작용 마커 높이도 필요에 따라 조절한다.
- 임시 모델은 200×96×약 200cm의 단순 선반형 식량 보관함이다. 메시와 머터리얼은 같은 DemoEnding 폴더에 저장했다.
- `/Game/Interaction/DemoEnding/BP_DemoDinnerEnding`: `BunkerMap`의 기존 `SM_KitchenTable` 위치에 배치했다. BP 전체를 이동하거나 컴포넌트 `EndingCamera`, `LunaPosition`, `MolePosition`을 따로 조절한다. 위치 기준점은 캐릭터 액터 원점이다(루나는 캡슐 중심, 두더지는 액터 원점).
- 벙커에는 엔딩 BP를 한 개만 둔다. `DinnerDialogue`, `FarewellIllustration`, `FadeSeconds`는 BP/배치 인스턴스에서 편집 가능하다.

## 플레이 흐름

1. 외부 창고 상호작용으로 참치캔(3004) 1개를 얻는다. 가방이 가득 차면 지급과 수집 상태 변경을 모두 하지 않는다.
2. 한 번 꺼낸 창고는 그 월드에서 재획득할 수 없다. 외부 맵 재입장 시 보충하여 사망·섭취로 캔을 잃어도 진행할 수 있다.
3. 마지막 퀘스트 `demo_q4_todays_reward`를 수락한 상태에서 두더지의 대화 또는 퀘스트 상호작용을 선택하면 캔 1개를 소모하고 기존 `demo.canned_tuna.deliver / world_progress` 목표를 완료한다.
4. 보상을 수령한 뒤 식당으로 0.2초 암전/페이드인 전환한다. 보상 수령에 실패하면 RewardAvailable에 남아 다음 상호작용에서 재시도하며 캔을 다시 요구하지 않는다.
5. 식당에서 루나와 두더지의 8줄 대화를 진행한다. 대화가 끝나면 다시 암전하고 작별 화면을 표시한다. 기존 화장실 최종 장면 대신 이번 요청의 식당 장면을 사용한다.
6. 원본 삽화를 비율 유지로 화면 높이 2/3 영역에 표시하고 “본편에서 만나요” 메시지를 함께 표시한다. 새 키 입력(게임패드 포함), 마우스 버튼 또는 휠로 페이드 후 `IntroMap` 타이틀로 돌아간다. 진입 직후 0.5초 및 키 반복 입력은 이전 대화 입력으로 화면이 즉시 닫히는 것을 막는다.

## 저장과 검증

- 인벤토리·퀘스트 진행은 기존 저장 경로를 사용한다. 별도 저장 스키마는 추가하지 않았다.
- 작별 화면 표시 시 `demo.ending.farewell_seen`을 기존 시나리오 완료 플래그에 저장한다. 최종 보상을 받은 뒤 엔딩 도중 종료했다면 다음 벙커 진입 시 엔딩을 다시 시작한다. 이미 작별 화면까지 본 세이브는 자동 재생하지 않는다.
- 연출용 캐릭터 이동은 임시이며 작별 화면 표시 전 원래 위치를 복원한 뒤 저장한다.
- 원본 삽화: `TunaSweeper/SourceArt/UI/DemoEnding/T_DemoFarewell.png`.
- UI 애셋: `/Game/UI/DemoEnding/T_DemoFarewell`.
- 자동 검사: `Automation RunTests TunaSweeper.DemoEnding`. 창고 획득·중복 차단·외형 교체, 텍스처 원본 크기/UI 설정, 삽화 영역 비율, 진입/키 반복 차단, 키보드·마우스 복귀를 검사한다. 화면 렌더는 `TunaSweeper/Saved/DemoEndingFarewellPreview.png`에 저장한다.
- Demo 퀘스트 동기화 상태 조회는 보호 토큰 읽기 오류로 실패했다. 기존 퀘스트 데이터 수정/게시 없이 기존 이벤트에 연결했고 `quest:validate`는 성공했다.
- 일회성 메시/BP 생성·배치 코드는 애셋과 함께 커밋한 뒤 제거한다. 런타임은 저장된 애셋을 사용한다.
