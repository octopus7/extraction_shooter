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
