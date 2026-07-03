# 04. 상호작용 시스템

## 학습 목표

- 모든 상호작용을 개별 키 입력 처리로 만들지 않고 공통 구조로 묶는다.
- 마커 표시, 포커스 후보, 실제 실행 요청을 분리한다.
- 문, 상자, NPC, 추출 지점, 상점처럼 서로 다른 대상이 같은 상호작용 흐름을 공유하는 방식을 이해한다.

## 강의 흐름

1. `InteractableComponent`가 표시 이름, 타입, 실행 조건을 제공하는 구조를 설명한다.
2. Interaction Subsystem이 현재 포커스 대상과 입력 요청을 관리하는 방식을 설명한다.
3. Actor별 실제 동작은 각 Actor가 처리하고, 공통 UI는 타입과 표시 데이터만 사용한다.
4. 상호작용 성공 이벤트가 퀘스트나 저장 상태로 연결되는 지점을 설명한다.

## 핵심 개념

- 상호작용은 "보인다", "선택된다", "실행된다", "결과가 반영된다" 단계로 나눌 수 있다.
- UI 마커가 보인다고 항상 실행 가능한 것은 아니다.
- 상호작용 타입은 UI와 입력 힌트에 쓰이고, 실제 효과는 Actor별 구현이 담당한다.
- 퀘스트 목표나 튜토리얼은 상호작용 완료 이벤트를 감지해 진행할 수 있다.

## 기준 프로젝트에서 볼 지점

- `Private/Interaction/TunaSweeperInteractableComponent.cpp`
- `Private/Subsystem/TunaSweeperInteractionSubsystem.cpp`
- `Private/Interaction/TunaSweeperLootContainerActor.cpp`
- `Private/Interaction/TunaSweeperExtractionPointActor.cpp`
- `Private/UI/TunaSweeperInteractionMarkerWidget.cpp`

## 실습

- `EInteractType` 같은 분류를 상상하고 문 열기, 아이템 줍기, NPC 대화, 추출을 분류한다.
- 상호작용 마커에 필요한 최소 데이터 4개를 정한다. 예: 이름, 거리, 키, 가능 여부.
- "상자 열기"가 성공했을 때 UI, 인벤토리, 퀘스트에 어떤 이벤트가 가야 하는지 순서로 적는다.

## 확인 포인트

- Actor가 UI를 직접 강하게 참조하면 유지보수가 어려워지는 이유를 설명할 수 있다.
- 포커스 후보와 실제 실행 대상이 달라질 수 있는 상황을 예로 들 수 있다.
- 상호작용 이벤트를 퀘스트 목표와 연결하는 장점을 이해한다.
