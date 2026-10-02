# SSOT

이 경로는 공용 시스템의 설계 기준과 이전 본편 기획의 보관본을 관리한다. 본편 서사의 현재 기준은 접근 제한 저장소에서 관리한다.

## 현재 본편 기준

- [본편 설계 SSOT](../../TunaSweeper/External/MainPayload/Docs/SSOT/main_game_design.md)가 현재 우선 기준이다. 이 링크는 접근 제한 저장소가 있는 로컬 작업 환경에서만 열린다.
- 2026-10-03 사용자 지시에 따라 신규 제공 문서를 우선하고, 충돌하는 이전 본편 기획의 효력을 대체했다. 작업안과 미정 사항은 확정으로 올리지 않는다.
- 본편 서사, 원문 자료, 비교 분석, 관련 작업 기록은 접근 제한 저장소 안에 둔다. 현재 로컬 경로와 빌드 경계는 [Demo/Main 빌드 데이터 구조](../Steam/TunaSweeper_Build_Flavor_Data_Architecture.md)를 따른다.
- Demo 데이터와 공용 런타임의 변경은 별도 구현 작업이다. 설계 문서 갱신을 구현 완료로 해석하지 않는다.

## 공용 시스템 기준

- [조제법과 설계도 해금](workbench_recipe_blueprint_unlocking.md), [데이터 작성 규칙](data_authoring_conventions.md), [헤드샷 판정](headshot_hit_zone_design.md), [원거리 적 전투 패턴](ranged_enemy_combat_pattern.md)은 각 공용 시스템의 기준으로 유지한다.
- 현재 본편 설계가 명시적으로 교체하지 않은 공용 규칙은 계속 적용한다. 본편의 미정 항목을 과거 기획으로 임의 확정하지 않는다.

## 이전 본편 기획 보관본

- `TunaSweeper_SSOT_Quest_Item_v0.6.md`: 이전 퀘스트 구조와 아이템 기획.
- `TunaSweeper_Quest_Dialogue_v0.6.1.md`: 이전 퀘스트별 대화.
- `area_unlocking.md`: 이전 지역 진행과 엔딩 조건.
- `README_SSOT_v0.6.1.md`: 이전 패키지 설명.

이 보관본의 메인 체인, 선행조건, 엔딩 조건과 대화는 현재 본편의 구현 지시가 아니다. 기존 아이템·선택 콘텐츠 전체를 삭제한다는 의미도 아니며, 재사용 여부는 현재 본편 기준에서 판단한다.
