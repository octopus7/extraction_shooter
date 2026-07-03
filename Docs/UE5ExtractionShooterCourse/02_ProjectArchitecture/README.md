# 02. 프로젝트 아키텍처

## 학습 목표

- 익스트랙션 슈터를 기능별 소유자로 나누는 기준을 배운다.
- Actor, Component, Subsystem, GameInstance의 역할을 구분한다.
- 데이터 기반 스폰과 런타임 상태 관리가 왜 필요한지 이해한다.

## 강의 흐름

1. 전체 게임 루프를 시스템 책임으로 분해한다.
2. 영속 상태와 일시 상태를 구분한다.
3. 맵 로드 후 데이터 기반 액터 스폰 흐름을 설명한다.
4. UI와 게임플레이 코드가 직접 얽히지 않도록 중간 계층을 둔다.

## 핵심 개념

- `GameInstance`는 세이브 슬롯, 인벤토리, 재화, 진행 상태처럼 레벨을 넘어 유지되는 상태를 관리한다.
- `Subsystem`은 아이템 데이터, 퀘스트, 상호작용, 적 스폰처럼 기능별 서비스를 제공한다.
- `Actor`는 월드에 존재하는 개별 대상이다. 상자, 문, 추출 지점, 적, NPC가 여기에 해당한다.
- `Component`는 Actor에 붙는 재사용 기능이다. 상호작용, 체력, 무기 전투, 시야 처리가 여기에 적합하다.
- UI는 상태를 소유하기보다 현재 상태를 표시하고 사용자 입력을 라우팅한다.

## 기준 프로젝트에서 볼 지점

- `Private/Game/TunaSweeperGameInstance.cpp`
- `Private/Subsystem/TunaSweeperInteractionSubsystem.cpp`
- `Private/Subsystem/TunaSweeperEnemySpawnSubsystem.cpp`
- `Public/Inventory/TunaSweeperSaveGame.h`
- `Docs/runtime_actor_spawns.md`

## 실습

- "상자 열기" 기능을 Actor, Component, Subsystem, UI로 나누어 책임을 적어 본다.
- "레이드에서 획득한 아이템"이 레이드 중에는 어디에 있고, 탈출 후에는 어디에 저장되는지 흐름도를 만든다.
- 새 기능을 만들 때 저장 대상인지, 월드 임시 대상인지, UI 표시 대상인지 먼저 분류한다.

## 확인 포인트

- 기능을 전부 캐릭터 클래스에 넣지 않는 이유를 설명할 수 있다.
- Subsystem이 전역 싱글턴처럼 남용되면 안 되는 지점을 구분할 수 있다.
- 데이터 파일과 런타임 Actor 사이의 연결 방식을 설명할 수 있다.
