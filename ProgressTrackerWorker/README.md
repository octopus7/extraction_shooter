# TunaSweeper 진행도 트래커

Unreal 게임에서 보내는 **주요 퀘스트·장소 도달·데모 완료** 이벤트를 Cloudflare Workers로 받아 D1에 저장하고, 관리자 웹 화면에서 통계를 조회하는 독립 프로젝트입니다.

이 폴더의 내용 전체를 새 리포의 루트로 옮겨 사용합니다. Unreal 전송 코드는 원래 게임 리포에 있으며, 이 리포는 게임 소스 없이 테스트·배포할 수 있습니다.

## 새 에이전트가 먼저 알아둘 규칙

- 전송 누락은 허용합니다. 게임은 실패 시 재시도·전송 대기열·오류 UI를 사용하지 않습니다.
- A 기록 없이 B가 도달해도 **B를 정상 집계**합니다. 필수 선행 A는 '기록 미수신'으로만 표시하고 도달 기록을 추정해서 만들지 않습니다.
- 도달률 분모는 선택한 빌드에서 한 번 이상 관측된 고유 playerId 수입니다.
- playerId는 설치별 임의 ID, runId는 새 게임별 ID입니다. 진행은 playerId/runId/buildId/dataset으로 구분합니다.
- 같은 eventId의 동일 내용은 중복 처리하고, 다른 내용이면 기존 데이터를 덮어쓰지 않습니다.
- 화면 문구는 `public/strings/ko.json`에 추가하고 키로 조회합니다.
- 관리자 Secret `ADMIN_TOKEN`은 서버에서만 사용합니다. 게임·Git·URL에 넣지 않습니다.

## 코드 위치

| 경로 | 역할 |
| --- | --- |
| `src/worker.js` | HTTP 라우팅 |
| `src/events.js`, `src/database.js` | 입력 검증과 이벤트 저장 |
| `src/catalog.js`, `src/statistics.js` | 체크포인트 선행 관계와 통계 |
| `src/auth.js` | 관리자 인증 |
| `public/` | 한국어 관리자 화면 |
| `migrations/0001_initial.sql` | D1 초기 스키마 |
| `test/`, `scripts/` | 자동화 테스트·검사·로컬 미리보기 |
| `wrangler.jsonc` | Cloudflare 배포 설정 |

수신 경로는 `POST /api/events`입니다. 정확히 `eventId, playerId, runId, buildId, dataset, checkpointId, category, playtimeSeconds` 8개 필드를 받습니다. 현재 dataset은 `demo`만 지원합니다. 상세 규격과 예시는 아래 운영 안내에 있습니다.

## 확인과 배포

Node 22.18 이상에서 `npm ci`, `npm test`, `npm run check`로 확인합니다. `npm run preview`는 예시 데이터가 있는 로컬 화면을 엽니다.

배포는 **새 GitHub 리포 → Cloudflare Workers Git 연동**으로 진행합니다. 로컬 Wrangler 인증·실행은 사용하지 않으며, Cloudflare 빌드 서버의 기본 배포 명령은 Wrangler입니다.

옮긴 뒤 필요한 설정:
1. D1 생성 및 초기 SQL 실행.
2. `wrangler.jsonc`의 Worker 이름·DB 이름·DB UUID 교체. 현재 DB UUID는 자리표시자입니다.
3. Cloudflare에서 `ADMIN_TOKEN` Secret 설정.
4. 배포 후 게임 리포에서 Endpoint·BuildId를 채우고 전송 활성화.

## 상세 문서와 현재 상태

- [운영·배포·API 안내](docs/operations.md): GitHub 연결, D1, 게임 설정, 통계 의미.
- [설계](docs/design.md), [완료된 구현 계획](docs/implementation-plan.md), [검증 기록](docs/verification.md).

구현 당시 Worker 테스트 19개, Unreal 빌드·자동화 3개가 통과했습니다. **실제 Cloudflare 배포와 게임의 실서비스 전송은 아직 검증하지 않았습니다.** 변경 후에는 테스트를 다시 실행하고 운영 연결을 별도로 확인하세요.
