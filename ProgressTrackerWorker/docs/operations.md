# 운영·배포·API 안내

프로젝트 루트 폴더 전체를 **별도 GitHub 리포의 루트**로 옮겨 Cloudflare Workers에 연결합니다. 상위 게임 리포나 QuestStudio 파일에 의존하지 않습니다.

주요 퀘스트·장소·데모 완료 기록을 D1에 저장하고 한국어 관리자 화면에서 조회합니다. 게임 전송부는 원래 Unreal 프로젝트에 남습니다.

## GitHub 연동 배포

로컬 Wrangler 실행이나 로그인이 필요하지 않습니다. **Cloudflare의 GitHub Builds는 서버에서 Wrangler로 배포**합니다.

1. 이 폴더의 내용(숨김 파일 포함)을 새 리포 루트에 복사하고 GitHub에 올립니다.
2. Cloudflare 대시보드에서 별도 **D1 데이터베이스**를 만듭니다.
3. D1의 Console에서 `migrations/0001_initial.sql` 내용을 실행합니다. 초기 SQL은 다시 실행해도 기존 기록을 삭제하지 않습니다.
4. `wrangler.jsonc`를 수정합니다.
   - `name`: 만들 Worker 이름과 일치시킵니다.
   - `d1_databases[0].database_name`: 생성한 D1 이름.
   - `d1_databases[0].database_id`: 생성한 D1 UUID. 현재의 0으로 채운 UUID는 자리표시자이며 교체해야 합니다.
   - 바인딩 이름 `DB`와 `ASSETS`는 유지합니다.
5. Workers & Pages에서 GitHub 리포를 연결합니다.
   - Root directory: 리포 루트
   - Build command: `npm ci && npm test && npm run check`
   - Deploy command: 기본 `npx wrangler deploy`
   - Node 버전: 22.18 이상(`NODE_VERSION=22.18.0` 설정 가능)
6. Worker의 Variables and Secrets에서 **Secret** `ADMIN_TOKEN`을 등록합니다. 공백 없는 무작위 영숫자 토큰(32바이트 이상 권장, 문자열 길이 24~256)을 사용합니다.
7. 배포 후 Worker 주소를 열고 관리자 토큰으로 연결합니다. `/api/health`는 Worker 실행 상태만 나타내며 D1 연결 검사 결과가 아닙니다.
8. D1 스키마 및 바인딩이 준비됐는지 관리자 화면으로 확인한 뒤 Unreal 설정을 활성화합니다.

ADMIN_TOKEN은 GitHub, 게임 설정, URL에 넣지 않습니다. 화면은 입력한 토큰을 메모리에만 보관하며 페이지 새로고침 시 다시 입력합니다. 통계 API 응답은 캐시하지 않습니다.

D1 SQL 변경은 자동으로 운영 DB에 적용하지 않습니다. 다음 마이그레이션이 생기면 내용을 검토한 뒤 D1 Console에서 적용합니다. 미리보기 브랜치를 운영 D1에 연결하지 않도록 별도 Worker/DB를 사용하거나 비운영 자동 배포를 끕니다.

공식 안내: [Workers Builds](https://developers.cloudflare.com/workers/ci-cd/builds/configuration/), [D1 대시보드 설정](https://developers.cloudflare.com/d1/get-started/).

## Unreal 설정

원래 게임 리포의 `TunaSweeper/Config/DefaultGame.ini`:

```ini
[TunaSweeper.ProgressTracker]
Enabled=True
AllowDevelopment=False
Endpoint=https://YOUR-WORKER.YOUR-SUBDOMAIN.workers.dev/api/events
BuildId=demo-2026.09.30
```

- 기본값은 비활성이며 URL·BuildId도 비어 있습니다. 배포 후 실제 값을 설정합니다.
- Demo 빌드에서만 전송합니다. Main·전투 테스트는 차단합니다.
- Development/PIE에서 확인하려면 `AllowDevelopment=True`를 명시하고 별도 테스트 Worker/DB를 사용합니다.
- BuildId와 체크포인트 ID는 최대 128자, ASCII 영숫자로 시작하고 이후 영숫자·`_`·`.`·`:`·`-`만 사용합니다.
- 설치별 임의 playerId와 세이브별 runId를 유지합니다. 새 게임은 새 runId를 만들며 이어하기는 그대로 사용합니다.
- 저장 버전에 필드가 없던 기존 세이브는 ID만 보완하고 과거 퀘스트 이벤트를 소급 전송하지 않습니다.
- 주요 퀘스트는 **보상 수령** 시점입니다. 목표 달성만 하고 보상을 받지 않은 상태와 구분합니다.
- 위치 기록은 기존 `AchievementLocationTrigger`에 이름이 있는 LocationId만 연결합니다. 이름 없는 트리거를 새로 배치하거나 맵을 수정하지 않았습니다.
- 데모 완료는 엔딩의 farewell 표시 후 완료 세이브를 삭제하기 전에 전송합니다.
- 전송 시도한 단계를 저장하며 HTTP 실패 시 재시도·로컬 대기열·게임 오류 표시는 없습니다. 종료 직전 요청도 누락될 수 있습니다.
- 플레이 시간은 게임의 누적 슬롯 시간이며 현재 구현상 일시정지 시간도 포함됩니다.

## 이벤트 API

`POST /api/events`, `Content-Type: application/json`

```json
{
  "eventId": "d4104ae3-1f36-425e-964c-57a1955b40bf",
  "playerId": "10000000-0000-4000-8000-000000000001",
  "runId": "20000000-0000-4000-8000-000000000001",
  "buildId": "demo-2026.09.30",
  "dataset": "demo",
  "checkpointId": "quest.demo_q2_clear_water_screen",
  "category": "quest",
  "playtimeSeconds": 125
}
```

정확히 위 8개 필드를 받습니다. UUID는 하이픈 형식이며 서버가 소문자로 정규화합니다. 시간은 유한한 숫자 0~315576000초입니다. 본문은 16 KiB 이하입니다.

| 종류 | 체크포인트 |
| --- | --- |
| start | game.start |
| quest | quest.<QuestId> |
| location | location.<LocationId> |
| complete | demo.complete |

- 201: 신규 저장, 200: 동일 eventId·동일 내용의 중복.
- 409: 동일 eventId에 다른 내용. 기존 기록은 보존.
- 400/413/415: 입력·크기·형식 오류.
- 503: 저장 불가. 게임은 실패를 무시하고 다음 체크포인트에서 새 이벤트를 보냅니다.
- 수신은 공개 API입니다. 관리자 토큰을 게임에 포함하지 않습니다. 입력값은 사용자 측에서 조작 가능하므로 결제·보상·치트 판정의 근거로 사용하지 않습니다. 큰 공개 배포 전에는 Cloudflare에서 수신 경로의 트래픽 제한을 설정할 수 있습니다.

## 통계와 미수신 의미

- 도달률 분모는 선택한 빌드에서 **하나 이상의 기록이 수신된 고유 playerId 수**입니다. 전체 설치·다운로드 수가 아닙니다.
- 시작 기록 인원은 따로 보여 줍니다. 시작 이벤트가 없어도 뒤 단계 기록은 도달 인원에 포함됩니다.
- 고유 ID 수는 사람 수와 같지 않을 수 있습니다. 설치 ID 재생성, 다른 PC나 복사된 세이브는 집계 기준에 영향을 줍니다.
- 진행은 playerId + runId + buildId + dataset으로 구분합니다.
- 단계 시간은 같은 진행·단계의 가장 작은 누적 시간, 화면의 평균은 진행별 첫 도달 시간의 평균입니다.
- q2는 q1, q3a/q3b는 q2, q4는 q3a와 q3b, 완료는 q4 기록을 선행 조건으로 표시합니다.
- 필수 관계는 `src/catalog.js`에 명시합니다. 표시 순서만으로 누락을 추정하지 않습니다.
- 예: B가 수신되고 필수 선행 A가 없으면 **B 도달을 인정하고 A 기록 미수신을 표시**합니다. A 이벤트는 만들어 넣지 않습니다. A가 나중에 들어오면 해당 미수신 표시가 사라집니다.
- 미수신은 실제 미도달이나 전송 오류를 확정하는 진단이 아닙니다.
- 카탈로그에 없는 장소 등도 저장하고 ID와 '분류 미등록'을 표시합니다. 필수 관계를 모르면 누락을 추정하지 않습니다.
- 대시보드의 진행 목록은 페이지당 25개입니다. 빌드 선택은 정렬된 최초 200개를 표시합니다. 현재 데모 규모를 대상으로 하며 단계/진행 상세에는 별도 페이지 분할이 없으므로 카탈로그·기록이 크게 늘면 조회 제한과 보관 정책을 추가해야 합니다.
- 실제 수신 데이터가 없으면 표본이 없는 상태로 표시합니다. 로컬 미리보기 예시 데이터는 배포에 포함되는 DB 데이터가 아닙니다.

## 로컬 검증과 미리보기

Node 22.18 이상에서 실행합니다. npm 런타임 의존성은 없습니다.

```text
npm ci
npm test
npm run check
npm run preview
```

`http://127.0.0.1:8788`에서 확인합니다. 토큰은 **미리보기 전용** `local-preview-token-32-characters`입니다. 운영 비밀값으로 사용하지 마세요. `node scripts/preview.mjs --empty`는 빈 데이터로 시작합니다.

미리보기는 Node의 메모리 SQLite와 Worker 코드를 사용합니다. Cloudflare에 접속하거나 기록을 전송하지 않으며 종료하면 예시 데이터가 사라집니다. Node 22에서는 SQLite experimental 경고가 나올 수 있습니다.

테스트는 누락 허용, 퀘스트 분기, 중복·충돌, 빌드/진행 분리, 관리자 인증, 입력 제한과 문자열 키를 검증합니다. 실제 Cloudflare 인증·배포·운영 D1 연결은 사용자의 GitHub 연동 후 별도 확인 대상입니다.

## 파일 구성

- `src/`: 이벤트 수신·집계·관리자 인증·체크포인트 카탈로그.
- `public/`: 관리자 화면과 `strings/ko.json` 문자열 테이블.
- `migrations/`: D1 SQL.
- `test/`: Node 테스트 및 실제 SQLite 쿼리 어댑터.
- `scripts/`: 구문·설정 검사와 로컬 미리보기.
- `docs/`: 승인한 설계와 구현 계획.

이 폴더 밖의 절대 경로나 다른 리포의 인증 파일은 필요하지 않습니다.
