# 대전차 로켓 무기

## 구현 계획

- [x] 6019 사용·조준·취소·잠금과 1회 소모, 20번 발사 도전과제를 연결한다.
- [x] 교체 가능한 메시와 푸른 배출 이펙트를 가진 BP 로켓을 만든다.
- [x] 수직 상승 후 목표 지면에 푸른 데칼을 표시하고 목표 상공에서 수직 급강하한다.
- [x] 슬롯 변경·취소·타깃 소실·비행 단계·도전과제·저장 애셋을 검증한다.

사용자가 지정한 모델링 담당은 GPT-6.1 Sol / xhigh다. 메시 원본은 SourceArt/Weapons/TopAttackRocket에 두며 메인 구현은 기존 아이템 사용과 HUD 경로를 확장한다. 새 위젯 트리는 만들지 않는다. UE 5.7을 사용하며 UI 문구는 문자열 키로 해석한다.

초기 조정값은 구현자가 정한 잠금 1.2초, 상승 0.8초·18m, 상공 유도 0.8초, 급강하 0.45초, 자동 탐색 반경 35m, 피해 200·폭발 반경 3m다. 발사 후 잠금 대상이 사라지면 마지막 목표 지점을 유지하며 비잠금 발사는 상승 중 주변 적을 탐색한다. 적을 찾지 못하면 조준 지점에 내려찍는다. 급강하 진입 때 지점을 확정해 수직 낙하를 유지한다.

취소·실패는 소모와 달성을 발생시키지 않는다. 발사는 선택한 슬롯의 아이템 UID를 재검증하며 아이템 소모는 기존 인벤토리 저장 경로를 사용한다. 비행 중 로켓과 조준 상태는 일시 상태다.

## 애셋과 조정

`/Game/Weapons/Rocket/BP_TopAttackRocket`의 BodyMesh를 교체한다. ExhaustOffset은 메시 로컬 좌표의 배출구이며 기본 (-40, 0, 0)cm다. 메시 기수는 +X, 크기는 80 × 21.8 × 21.8cm다. Blender FBX 원본의 미터 단위에 대해 최초 가져오기 배율 100을 적용한다.

ExhaustMaterial은 푸른 발광·맥동 배출, GuidanceMaterial은 지면에 투사되는 동심원·십자 표식이다. AscentSeconds/AscentHeight/GuidanceSeconds/DiveSeconds/SearchRadius/Damage/DamageRadius는 BP 기본값에서 조정한다. 잠금 시간·화면 반경·거리는 플레이어 클래스의 RocketLockSeconds/RocketLockRadiusPixels/RocketLockRange다.

## 검증

2026-11-17 UE 5.7 Development Editor 빌드와 `TunaSweeper.Rocket+TunaSweeper.Achievement` 자동화 20개 통과. 로켓 5개는 취소·퀵슬롯·UID 변경·클래스 실패·중복 발사·단일 소모·도전과제, 비행 단계·긴 프레임·천장 충돌, 저장 BP, 적 자동 선택·잠금 대상 이동·소실, 실제 폭발 피해·아군 보호를 검증한다. 사용자 커서의 실제 잠금 조작과 Steam 서버 전송은 자동화 범위 밖이다. Steam 플랫폼 ID 매핑은 기존 상태를 따른다.
