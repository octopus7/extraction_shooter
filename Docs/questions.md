# 질문 기록

## 2026-05-15 18:01:20 (소요시간: 1분 미만)

### 질문

벙커 레이드 전환간 동영상 재생하고 싶은데 어떻게 할까? 과정 하나씩 알려줘

### 답변

레벨 전환 액터가 바로 `OpenLevel`을 호출하지 않고, 전환 위젯에서 동영상을 재생한 뒤 재생 종료 이벤트에서 목표 레벨로 이동하는 구조가 적합하다.

1. `TunaSweeper/Content/Movies`에 전환용 MP4를 넣는다.
2. `FileMediaSource`를 전환 방향별로 만든다. 예: `MS_BunkerToRaid`, `MS_RaidToBunker`.
3. 공용 `MediaPlayer`와 `MediaTexture`를 만든다. 예: `MP_LevelTransition`, `MT_LevelTransition`.
4. 전체 화면 영상 표시용 `WBP_LevelTransitionVideo`를 만든다.
5. `UTunaSweeperLevelTransitionWidget`에서 MediaPlayer 재생, 종료 이벤트 수신, 종료 후 `OpenLevel(TargetLevelName)` 호출을 처리한다.
6. `ATunaSweeperLevelTravelInteractableActor`는 목표 레벨과 MediaSource를 전환 위젯에 넘기고, 직접 레벨을 열지 않도록 바꾼다.
7. 영상 로드나 재생 실패 시에는 바로 `OpenLevel`로 fallback한다.
8. 영상 재생 중에는 입력을 차단해 중복 상호작용을 막는다.

## 2026-05-15 18:36:56 (소요시간: 1분 미만)

### 질문

상호작용 목록이 나열되고 포커스된 상호작용은 마커와 같은 줄에 표시되는 식으로 목록이 위치 이동하는 표현인지 이해했는가?

### 답변

이해했다. 마커는 고정되고, 상호작용 목록 전체가 위아래로 이동해서 현재 포커스 항목만 마커의 가로 기준선에 정렬되는 방식으로 보면 된다.

## 2026-05-15 18:53:33 (소요시간: 1분 미만)

### 질문

NPC 캐릭터를 구현하고 퀘스트 상호작용을 달아서 구현 가능한가? 아직 결정이 더 필요한 것이 있는가? 스켈레탈 메시가 아직 없으니 일단 스태틱 캡슐 메시를 임시로 보이게 처리한다.

### 답변

구현 가능하다. 큰 설계 결정은 대부분 정해졌고, 남은 결정은 배치 위치, 첫 퀘스트 제공 NPC의 이름/표시명, 보상 없음 상태에서 보상 완료로 넘기는 UI 처리, 다중 상호작용 확장을 이번에 같이 할지 여부 정도다.
