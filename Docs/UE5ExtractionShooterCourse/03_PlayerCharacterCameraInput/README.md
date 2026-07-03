# 03. 플레이어, 카메라, 입력

## 학습 목표

- 탑다운 캐릭터에서 이동, 조준, 카메라, 입력을 분리해 설계한다.
- 플레이어 상태와 HUD 표시가 연결되는 흐름을 이해한다.
- 캐릭터 주변 시야, 장애물 투명화, 커서 기준 조준 같은 탑다운 편의 기능을 설명한다.

## 강의 흐름

1. 플레이어 캐릭터의 기본 이동과 회전을 구현한다.
2. PlayerController가 입력과 HUD 모드를 조정하는 방식을 설명한다.
3. 카메라가 월드 방향과 화면 방향을 어떻게 연결하는지 정리한다.
4. 체력, 스태미나, 배고픔, 갈증 같은 생존 수치를 UI로 연결한다.

## 핵심 개념

- 탑다운 슈터에서는 이동 방향과 조준 방향이 항상 같지 않다.
- 마우스 커서 또는 화면 조준점은 발사 방향, 회전, 리코일 표시의 기준이 된다.
- 플레이어 입력은 게임플레이 모드, 인벤토리 모드, 상점/창고 모드에 따라 다르게 해석된다.
- 장애물에 가려지는 문제는 카메라보다 플레이어 가시성 시스템으로 푸는 편이 관리하기 쉽다.

## 기준 프로젝트에서 볼 지점

- `Private/Character/TunaSweeperTopDownCharacter.cpp`
- `Private/Player/TunaSweeperPlayerController.cpp`
- `Private/Component/TunaSweeperVitalsComponent.cpp`
- `Private/Component/TunaSweeperPlayerVisionComponent.cpp`
- `Private/Subsystem/TunaSweeperKeyboardInputSubsystem.cpp`

## 실습

- WASD 이동과 마우스 조준을 별도 함수로 나누어 설계한다.
- 체력과 스태미나를 가진 컴포넌트를 만들고 HUD에 표시할 값을 정한다.
- 카메라 기준 12시 방향과 월드 `+X` 북쪽 기준이 어떻게 연결되는지 그림으로 정리한다.

## 확인 포인트

- PlayerController와 Character에 각각 무엇을 넣을지 판단할 수 있다.
- 입력 모드 전환이 UI와 게임플레이 양쪽에 영향을 주는 이유를 설명할 수 있다.
- 탑다운 카메라에서 조준, 리코일, 투사체 방향이 어긋나는 문제를 예상할 수 있다.
