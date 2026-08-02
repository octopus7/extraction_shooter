# TunaSweeper 검토 문서

이 디렉터리는 TunaSweeper의 외부 설계·기술 도입 가능성, 현재 구현과의 충돌, 권장 아키텍처와 단계별 적용 계획을 기록한다.

## 문서 목록

| 문서 | 상태 | 요약 |
|---|---|---|
| [AI Native NPC v0.4.6 TunaSweeper 적용성 검토](ai_native_npc_applicability_review.md) | 검토 완료 | 기존 적 AI와 AI Native NPC의 공존, GameInstance 전역 선택, 실행 중 안전한 Brain 전환 구조 |

## 관리 규칙

- 검토 문서는 `Docs/reviews/` 아래에 둔다.
- 이 `README.md`를 검토 문서의 메인 인덱스로 사용한다.
- 새 검토 문서를 추가하면 문서 목록에 링크, 상태, 한 줄 요약을 함께 기록한다.
- 구현이 완료되어 검토 전제가 바뀌면 기존 결론을 조용히 덮어쓰지 않고 문서 상태와 변경 이력을 갱신한다.
- 프로젝트 전역 gameplay 규칙은 `Docs/game_conventions.md`, 저장 영속성은 `Docs/save_persistence.md`를 계속 단일 기준으로 사용한다.
