# UE 적 행동 이식 기록

## 2026-07-09 1차 이식

대상 구현은 `ATunaSweeperEnemyAIController` 중심으로 제한했다. 플레이어 캐릭터와 플레이어 컨트롤러 코드는 수정하지 않았다.

### 적용한 행동

- 비전투 상태를 `Idle`과 `Wander`로 분리했다.
- 비전투 적은 항상 플레이어를 바라보지 않고, 랜덤 시간 동안 대기와 배회를 반복한다.
- 교전 진입은 거리만 보지 않고 전방 100도 시야각, 2300cm 인지 거리, 발사선을 함께 평가한다.
- 교전 중에는 3600cm 초과 시 전투를 해제한다.
- 총기 적은 교전 최초 진입 시 즉시 접근하지 않고 `HoldFire`로 들어가 사격을 먼저 시도한다.
- 총기 적의 선호 사격권은 650-1000cm로 조정했다.
- 총기 적의 근접 위험권은 430cm로 조정했다.
- 전진은 선호 사격권 하한이 아니라 상한인 1000cm를 기준으로 제한한다.
- 기존 홀드 시간은 2배로 늘려 원거리 1.6-2.4초, 중거리 2.4-3.6초, 선호 사격권 2.8-4.4초를 사용한다.
- 근접형 벌목기 적의 인지 거리도 2300cm로 맞췄다.

### 수정 파일

- `TunaSweeper/Source/TunaSweeper/Public/AI/TunaSweeperEnemyAIController.h`
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyAIController.cpp`
- `TunaSweeper/Source/TunaSweeper/Private/AI/TunaSweeperEnemyCharacter.cpp`
- `Docs/SSOT/ranged_enemy_combat_pattern.md`

### 검증

- `TunaSweeperEditor Win64 Development` 빌드 성공.
- UE 5.7 에디터 실행 명령 성공.

### 다음 평가 포인트

- 실제 레벨에서 첫 인지 시 총기 적이 접근보다 사격을 먼저 하는지 확인한다.
- 100도 시야각이 너무 답답하거나 너무 넓은지 확인한다.
- 3600cm 해제 거리가 전투 유지에 과하거나 부족한지 확인한다.
- 650-1000cm 선호 사격권이 현재 무기 명중률과 플레이 감각에 맞는지 확인한다.
- 배회 속도 120cm/s와 아이들/배회 시간이 레벨 밀도에 맞는지 확인한다.
