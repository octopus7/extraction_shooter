# 07. 적 AI와 전투 조우

## 학습 목표

- 적 캐릭터, AIController, 스폰 데이터, 전투 조우 설계를 분리한다.
- 추적, 공격, 엄폐, 소리 반응 같은 AI 요소를 단계적으로 추가하는 방법을 배운다.
- 레이드 긴장감을 만드는 적 배치와 웨이브 구조를 이해한다.

## 강의 흐름

1. 가장 단순한 적은 플레이어를 감지하고 접근해 공격하는 Actor로 시작한다.
2. AIController는 판단과 이동 요청을 맡고, EnemyCharacter는 몸체와 전투 처리를 맡는다.
3. 스폰 Subsystem은 맵 데이터에 따라 적과 조우 장치를 배치한다.
4. 특수 적과 스포너는 조우 패턴을 다양하게 만드는 별도 Actor로 확장한다.

## 핵심 개념

- 적 AI는 처음부터 복잡한 행동 트리를 만들기보다 감지, 접근, 공격, 대기 상태를 명확히 나누는 것이 좋다.
- 적 스폰은 레벨에 직접 박아 두는 방식보다 데이터 기반 초기 배치가 관리하기 쉽다.
- 엄폐물과 폭발물은 AI 자체보다 조우 환경을 바꾸는 장치다.
- 웨이브 스포너는 플레이어가 머무를수록 위험이 커지는 레이드 압박을 만든다.

## 기준 프로젝트에서 볼 지점

- `Private/AI/TunaSweeperEnemyCharacter.cpp`
- `Private/AI/TunaSweeperEnemyAIController.cpp`
- `Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp`
- `Private/AI/TunaSweeperRollingBomberSpawner.cpp`
- `Private/Interaction/TunaSweeperSandbagCoverActor.cpp`
- `Docs/runtime_actor_spawns.md`

## 실습

- 기본 적 상태를 `Idle`, `Chase`, `Attack`, `Dead`로 나누어 전이 조건을 적는다.
- 데이터 기반 적 스폰 행에 들어갈 필드를 설계한다. 예: 레벨 이름, 위치, 적 ID, 수량.
- 3분짜리 레이드 구역에 고정 적, 순찰 적, 웨이브 스포너를 어떻게 배치할지 구상한다.

## 확인 포인트

- 적 캐릭터가 직접 모든 판단을 갖는 구조와 AIController를 나누는 구조의 차이를 설명할 수 있다.
- 적 배치가 난이도, 리스크, 루팅 동선을 동시에 바꾼다는 점을 이해한다.
- 특수 적은 코드보다 조우 의도에서 먼저 설계해야 한다는 점을 설명할 수 있다.
