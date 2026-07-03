# 08. 레이드 루프와 탈출

## 학습 목표

- 익스트랙션 슈터의 핵심인 출동, 탐색, 루팅, 탈출, 정산 흐름을 구현 단위로 나눈다.
- 레벨 이동, 추출 지점, 실패 복귀, 전환 UI가 어떻게 연결되는지 이해한다.
- 레이드 중 임시 상태와 탈출 성공 후 저장 상태를 구분한다.

## 강의 흐름

1. 벙커에서 레이드로 출동하는 레벨 이동 상호작용을 설명한다.
2. 레이드 맵에서 획득한 아이템과 경험치를 임시 상태로 모은다.
3. 추출 지점에 머무르는 방식과 즉시 복귀 상호작용의 차이를 설명한다.
4. 성공 복귀와 후송 복귀가 저장 데이터에 다른 결과를 남기는 이유를 설명한다.

## 핵심 개념

- 출동은 단순 레벨 이동이 아니라 장비와 인벤토리를 위험 상태로 전환하는 시작점이다.
- 추출은 일정 시간, 위치, UI 피드백, 방해 가능성 같은 조건을 가진 상호작용이다.
- 성공 탈출은 레이드 중 획득한 보상을 저장 상태로 확정한다.
- 후송 복귀는 실패를 완곡하게 표현하면서도 위험과 손실 규칙을 반영한다.
- 레벨 전환 영상과 페이드는 게임 규칙이 아니라 플레이어에게 상태 변화를 납득시키는 연출이다.

## 기준 프로젝트에서 볼 지점

- `Private/Interaction/TunaSweeperLevelTravelInteractableActor.cpp`
- `Private/Interaction/TunaSweeperExtractionPointActor.cpp`
- `Private/Subsystem/TunaSweeperLevelTransitionSubsystem.cpp`
- `Private/Subsystem/TunaSweeperRaidExperienceReturnSubsystem.cpp`
- `Private/UI/TunaSweeperExtractionProgressWidget.cpp`
- `Docs/save_persistence.md`

## 실습

- 레이드 성공 시 보존할 데이터와 실패 시 잃을 데이터를 표로 나눈다.
- 추출 지점 상호작용을 "진입, 체류, 진행률 표시, 완료, 취소" 단계로 나눈다.
- 레벨 이동 전후에 호출해야 할 저장/정산 함수를 의사 코드로 작성한다.

## 확인 포인트

- `OpenLevel` 호출만으로는 익스트랙션 루프가 완성되지 않는 이유를 설명할 수 있다.
- 레이드 중 보상과 저장된 보상의 차이를 구분할 수 있다.
- 탈출 UI가 실제 게임 규칙과 동기화되어야 하는 이유를 이해한다.
