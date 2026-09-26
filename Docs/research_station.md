# 욕실 연구 스테이션

## 배치

- 콘텐츠 브라우저에서 `/Game/Interaction/BP_ResearchSinkInteraction`을 벙커 욕실에 직접 배치한다. 이 작업은 맵에 액터를 자동 배치하지 않는다.
- 형상은 세면대 위 벽에 붙이는 진단 거울이다. 본체는 폭 64cm × 높이 90cm × 깊이 약 10cm이며 피벗은 하단 중앙이다. 앞면은 로컬 `+X`, 폭은 `Y`, 높이는 `Z`다.
- 기존 상호작용 컴포넌트가 연구 UI를 연다. 상단 HUD에는 연구 진입 버튼과 빈 자리 모두 남기지 않는다. 연구 화면의 세로 스크롤과 다른 HUD 메뉴는 유지한다.
- BP와 메시·재질은 실물 에셋이다. 런타임에 메시나 UMG 위젯 트리를 생성하지 않는다.

## 반복 동작

- 본체와 스캔바는 각각 `SM_ResearchMirror`, `SM_ResearchScanBar` 스태틱 메시다. 스켈레탈 메시나 애니메이션 시퀀스는 사용하지 않는다.
- 게임 실행 중 스캔바의 상대 Z 위치만 부드럽게 왕복한다. 기본 주기는 4초, 중심은 `(8.3, 0, 48)cm`, 이동 폭은 중심에서 위아래 각각 28cm다.
- BP 기본값 또는 배치 인스턴스의 `Research Station | Scan`에서 `Animate Scan`, `Scan Origin`, `Scan Travel`, `Scan Cycle Seconds`를 편집한다.
- 스캔바는 충돌하지 않는다. 본체는 단순 박스 충돌을 사용한다. 반복 동작은 장식이며 연구 시간이나 저장 데이터와 독립적이다.

## 에셋과 원본

- 배치 BP: `/Game/Interaction/BP_ResearchSinkInteraction`
- 메시·재질: `/Game/Environment/Bunker/ResearchMirror/`
- Blender 원본: `TunaSweeper/SourceArt/Environment/ResearchMirror/ResearchMirror.blend`
- FBX: 같은 디렉터리의 `Models/`
- 모델 미리보기: 같은 디렉터리의 `Previews/` (Blender 렌더이며 게임 화면 캡처가 아님)
- 모델 치수·재질 명세: 같은 디렉터리의 `model_manifest.json`

## 연구 데이터 안전성

- `Content/Data/StatResearchNodes.json` 수정 시 전체 파일 검증을 통과해야 정의가 교체된다. 실패한 재로드는 마지막 정상 정의를 유지한다.
- 정의가 일시적으로 없어진 노드의 완료·진행 저장 기록은 보존하지만, 현재 정의에 없는 노드는 효과와 개방 수에 포함하지 않는다. 같은 ID의 정의가 돌아오면 기록을 다시 사용할 수 있다.
- 미지원 효과 타입은 오류로 처리하며 체력 증가 효과로 대체하지 않는다.
- 새 게임은 이전 슬롯의 시계 보정 기준도 초기화한다. 동일 슬롯의 오프라인 진행과 시계 되돌림 방지는 유지한다.
- 시작과 완료 확정 저장, 동시 연구, 완료 대기 상태, 초기화 중 알림을 다음 틱으로 미루는 재진입 방지 구조는 유지한다.

## 검증

`TunaSweeper.Research` 자동화 테스트는 저장 복원, 시계 초기화, JSON 검증, 3개 언어 문자열, 실제 WBP 노드·스크롤, 상단 연구 버튼 부재, BP 메시와 스캔 반복 동작을 검사한다.
