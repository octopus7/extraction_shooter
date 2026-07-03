# UE5 익스트랙션 슈터 제작 강의 문서

## 적용 범위

이 문서 트리는 강의 제작을 위한 커리큘럼 전용 자료이며, 실제 게임 개발의 기준 문서가 아니다. 강의 편의를 위해 단순화, 재정렬, 예시화한 내용은 `TunaSweeper` 프로젝트의 코드, 에셋, 설정, 개발 문서로 자동 전파하지 않는다. 실제 개발 변경은 별도 요청과 검토가 있을 때만 진행하며, 충돌이 있을 경우 프로젝트 코드와 기존 개발 문서를 우선한다.

## 강의 목표

이 강의는 `TunaSweeper` 프로젝트를 참고 사례로 삼아 UE5 기반 탑다운 익스트랙션 슈터를 설계하고 구현하는 과정을 설명한다. 수강자는 완성된 프로젝트를 그대로 복제하기보다, 장르 루프를 나누고 각 기능을 Unreal Engine의 C++, Blueprint, 데이터 에셋, JSON 데이터, UI, 에디터 툴로 연결하는 방식을 학습한다.

강의의 핵심 결과물은 다음과 같다.

- 벙커와 레이드 맵을 오가는 익스트랙션 루프 이해
- 플레이어, 상호작용, 아이템, 전투, AI, UI, 저장 시스템의 역할 분리
- 데이터 기반 스폰, 퀘스트, 상점, 제작, 월드 진행 구성 방식 이해
- UE5.7 기준 C++ 프로젝트에서 런타임 모듈과 에디터 모듈을 나누는 방식 이해
- 강의용 단순 구현과 실제 프로젝트 품질 기준의 차이 구분

## 수강 전제

- Unreal Engine 5의 기본 에디터 조작을 알고 있다.
- C++ 클래스, UObject, Actor, Component, Subsystem 개념을 대략 이해한다.
- Blueprint와 UMG를 C++ 기능의 조립/표현 계층으로 사용하는 흐름을 받아들일 수 있다.
- Git, JSON, Markdown 문서를 읽고 프로젝트 구조를 따라갈 수 있다.

## 진행 원칙

- 강의 문서는 실제 프로젝트의 모든 세부 구현을 그대로 설명하지 않는다.
- 각 장은 "왜 필요한가", "어떤 책임을 갖는가", "최소 실습으로 어떻게 확인하는가" 순서로 구성한다.
- 실습은 가능한 한 작은 단위로 끊고, 다음 장의 시스템과 연결되는 지점을 남긴다.
- 저장 데이터, 런타임 스폰, 게임 기준처럼 실제 개발 안정성에 영향을 주는 내용은 기존 개발 문서를 참조만 한다.

## 기준 프로젝트

- Unreal 프로젝트: `TunaSweeper/TunaSweeper.uproject`
- 엔진 기준: UE 5.7
- 주요 런타임 코드: `TunaSweeper/Source/TunaSweeper`
- 주요 에디터 코드: `TunaSweeper/Source/TunaSweeperEditor`
- 주요 데이터: `TunaSweeper/Content/Data`
- 개발 기준 문서: `Docs/game_conventions.md`, `Docs/save_persistence.md`, `Docs/runtime_actor_spawns.md`

## 운영 계획

- [강의 운영 계획](CoursePlan.md)

## 문서 구조

- [00. 오리엔테이션](00_Orientation/)
- [01. UE 프로젝트 설정](01_UEProjectSetup/)
- [02. 프로젝트 아키텍처](02_ProjectArchitecture/)
- [03. 플레이어, 카메라, 입력](03_PlayerCharacterCameraInput/)
- [04. 상호작용 시스템](04_InteractionSystem/)
- [05. 아이템, 인벤토리, 루팅](05_ItemsInventoryLoot/)
- [06. 전투, 무기, 데미지](06_CombatWeaponsDamage/)
- [07. 적 AI와 전투 조우](07_EnemyAIEncounters/)
- [08. 레이드 루프와 탈출](08_RaidExtractionFlow/)
- [09. 저장과 영속성](09_SaveLoadPersistence/)
- [10. 월드, 맵, 레벨 디자인](10_WorldMapsLevelDesign/)
- [11. UI, HUD, 메뉴](11_UIUXHUDMenus/)
- [12. 에셋, 머티리얼, VFX, 오디오](12_AssetsMaterialsVFXAudio/)
- [13. 에디터 툴과 자동화](13_EditorToolsAutomation/)
- [14. 테스트, 밸런싱, 패키징](14_TestingBalancingPackaging/)
- [99. 부록](99_Appendices/)
