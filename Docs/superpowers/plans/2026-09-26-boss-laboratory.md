# Boss Laboratory Implementation Plan

**Goal:** 메인 메뉴 연구실에서 로컬 보스 조립·저장·싱글 시험을 제공하고 멀티는 개발 중 토스트만 표시한다.
**Architecture:** 내장 부품 카탈로그/검증 가능한 설계도, GameInstance 슬롯 라이브러리, 월드 조립 런타임, 편집 위젯, 타이틀 메뉴를 분리한다.
**Tech Stack:** UE 5.7 C++, UMG, JSON, 기존 전투·현지화·시험 저장 보호 기능.
**Spec:** ../specs/2026-09-26-boss-laboratory-design.md

## Global Constraints

- 실제 네트워킹 구현 없음. 정의와 전투 상태 분리, 안정적인 ID와 크기/실행 제한만 준비한다.
- 새 시스템 UI 문구는 UITextStrings.csv 키로만 해결한다.
- 12 슬롯, 32 부품, 깊이 8, JSON 256 KiB, 이름 64자.
- 본편 저장에 시험 장비/진행을 기록하지 않는다. 기존 다른 작업 파일을 덮어쓰거나 커밋에 포함하지 않는다.
- task 단일 커밋. 임시 에셋 생성기 없음.

## Tasks

- [x] 1. BossLab/TunaSweeperBossDefinition.h 및 Private 대응 파일: 타입/카탈로그/트리 변환/JSON 검증. TunaSweeperBossLabTests.cpp에 실제 잘못된 입력·왕복 테스트를 작성하고 실행.
- [x] 2. UTunaSweeperBossLabSubsystem: 12개 슬롯 파일, 검증 저장/불러오기, 초안/선택 모드, 연구실 맵 진입. 임시 폴더 파일 왕복과 실패 보존 검증.
- [x] 3. ATunaSweeperModularBoss 및 ATunaSweeperBossLabGameMode: 미리보기/싱글 전투, 부위 피해·기능 상실·기본 전술, 임시 장비 복구, 타이틀 복귀. 부위 파괴와 재시작 자동화.
- [x] 4. UTunaSweeperBossLabWidget: 슬롯 선택·저장·부품 연결/삭제·전술·미리보기·시험 버튼, 편집/플레이 모드 분리. 부적합 선택과 미저장 변경을 처리.
- [x] 5. IntroMenu 연구실 버튼/서브메뉴, 개발 중 토스트, 전체 ko/en/ja 문자열, MapsToCook. 기존 제목 메뉴 스타일/입력 복귀와 유지.
- [x] 6. 통합 빌드·관련 자동화·독립 코드 검토·가능한 UI 확인. Docs/save_persistence.md와 사용자 안내, Docs/requests.md 추가 후 이번 변경만 한 커밋. 에디터를 프로젝트 경로로 연다.

## Shared Interfaces

정의 타입과 직렬화는 BossLab/TunaSweeperBossDefinition.h, 세션/슬롯 메서드는 BossLab/TunaSweeperBossLabSubsystem.h에 먼저 고정한다. 런타임 모드는 RefreshPreview, StartBattle, StopBattle, ReturnToTitle 및 IsBattleActive를 제공하고 위젯은 RefreshFromSession을 제공한다. 세 부문은 각자 파일을 소유하고 루트가 통합한다.

## Review Focus

손상 파일 로드 시 초안/정상 슬롯 보존, 무기 없는 보스의 종료 가능성, 부모 파괴 시 자식 공격 정지, 연속 시험의 투사체/예고 정리, 세이브를 선택하지 않고 메인 메뉴에서 진입했을 때 시험 종료의 본편 상태 복구를 검사한다.

## Progress

- 2026-09-26: 사용자가 이전 제안에서 네트워크 구현을 제외하고 연구실 진입/싱글 기능 구현을 지시했다. 이 범위를 기준으로 구현을 진행한다.
- 최종 검증: UE 5.7 Development Editor 빌드 성공, 관련 자동화 17/17 성공(실패·미실행 0). 실제 맵 PIE의 반복 전투/카메라/입력/저장 복구와 렌더 화면을 확인했다. 기존 문자열 중복·PIE DLSS·테스트 월드 NavMesh 경고는 남아 있다.
- 독립 검토의 낮은 지지부 파괴 후 코어 높이 문제와 전투 재시작 시 인벤토리 입력 상태 문제를 수정하고 회귀 검사했다. 비대칭 조립의 경계/충돌, 편집 복귀 카메라, Canvas HUD 폰트 및 좁은 화면 안내도 보완했다.
- 프로젝트 에디터를 열고 이번 구현·문서·요청 기록만 단일 커밋으로 정리한다. 코옵 통합 담당 작업에는 최종 커밋과 공유 파일 수정 종료를 전달한다.
