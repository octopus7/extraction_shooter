# Quest Flow Simulator

퀘스트 이동 거리와 플레이 시간을 근사 계산하는 브라우저 시뮬레이션 도구입니다.

- 서비스: https://quest.oc7.workers.dev
- Worker: `quest`
- D1: `quest-flow-simulator`
- 시뮬레이션: 브라우저 Web Worker
- 저장/API: Cloudflare Worker + D1
- 배포: 프로젝트 로컬 Wrangler 수동 배포
- R2 및 GitHub Actions: 사용하지 않음

## 로컬 실행

```powershell
npm install
npm run db:migrate:local
npm run db:seed:local
npm run dev
```

## 수동 배포

Windows에서는 `deploy.bat`을 더블클릭하면 배포 로그를 콘솔에서 바로 볼 수
있습니다. 성공하거나 실패해도 마지막에 일시정지하므로 창이 자동으로 닫히지
않습니다.

```powershell
.\deploy.bat
```

기본 실행은 빌드, Wrangler dry run, 원격 D1 마이그레이션과 퀘스트 시드,
Worker 배포까지 수행합니다. D1을 변경하지 않고 Worker만 배포하려면 다음
옵션을 사용합니다.

```powershell
.\deploy.bat --worker-only
```

개별 명령으로 배포하려면 다음 순서로 실행합니다.

```powershell
npm run db:migrate:remote
npm run db:seed:remote
npm run deploy:dry
npm run deploy
```

퀘스트 원본이 바뀌면 `npm run seed:generate`가 데모, M01~M20, 현재 UE5
스냅샷 카탈로그를 다시 생성합니다. 시드는 UPSERT 방식이므로 같은 버전을
재적용할 수 있습니다.

## 중앙 캔버스 모드

중앙 영역은 다음 세 모드를 제공합니다.

- `퀘스트 체인`: 선행 조건과 퀘스트 노드만 표시합니다. 기존 노드 좌표는 유지하며, 좌표가 없는 입력만 초기 자동 배치합니다. `노드 자동 배치` 버튼을 눌렀을 때만 선행 조건 기준으로 현재 노드를 재배치하고, 한 줄에 최대 20개 노드를 배치합니다. 노드 좌표는 그래프 배치용이며 카드에는 표시하지 않고, 퀘스트 식별자와 이름은 헤더에 나란히 표시합니다. 카드 좌우의 원형 소켓은 연결되지 않으면 회색, 연결되면 입출력 방향 색상으로 표시합니다. 드래그로 위치를 변경할 수 있고 이동 범위 제한은 없습니다. 캔버스는 화면 내 고정 뷰포트에 표시하며 브라우저나 캔버스 스크롤 없이 패닝·줌으로 넓은 그래프를 탐색합니다.
- `선택 퀘스트 동선`: 선택한 퀘스트에 등록된 맵 이동이 있을 때만 동선을 표시합니다. 장소는 읽기 전용입니다.
- `장소 편집`: 맵 장소를 추가·이동·수정합니다. 장소는 액터 지점, 원형 영역, 직사각형 영역을 지원합니다.

세 모드 모두 마우스 휠로 확대·축소할 수 있습니다. `questNodes`의 그래프 좌표와
`places`의 맵 좌표는 서로 다른 데이터입니다. 원본에 장소나 액터 좌표가 없는
항목은 퀘스트 문맥으로 추정한 임시 장소를 사용합니다.

## 인증

혼자 사용하는 도구라는 전제에서 Worker Secret `ADMIN_PASSWORD` 하나로
관리자 로그인을 처리합니다. 비밀번호는 저장소나 D1에 넣지 않습니다. 로그인
성공 시 12시간짜리 난수 세션을 발급하고 D1에는 원문 대신 SHA-256 해시만
저장합니다. 로그아웃하면 해당 D1 세션이 즉시 삭제되므로 이전 쿠키를 다시
사용할 수 없습니다.

- 최소 비밀번호 길이: 16자(24자 이상의 무작위 값 권장)
- 로그인 제한: 동일 IP에서 15분 내 5회 실패 시 15분 차단
- 운영 쿠키: `Secure`, `HttpOnly`, `SameSite=Strict`, `__Host-` prefix
- 비로그인: 공개 체험판만 조회
- 로그인: M01~M20, UE5 스냅샷, D1 작업공간 사용

최초 운영 설정은 인증 테이블을 먼저 만든 뒤 비밀번호를 대화형으로 입력한다.
실제 값은 명령 기록이나 문서에 적지 않는다.

```powershell
npm run db:migrate:remote
npx wrangler secret put ADMIN_PASSWORD
.\deploy.bat --worker-only
```

로컬 개발에서는 Git에서 제외된 `.dev.vars` 파일에 다음 형식으로 넣는다.

```dotenv
ADMIN_PASSWORD="로컬에서만-사용할-16자-이상-비밀번호"
```

비밀번호를 바꾼 뒤 기존 세션도 즉시 모두 끊어야 하면 D1의
`admin_sessions` 행을 삭제한다. 아무 비밀번호도 설정하지 않거나 16자보다
짧으면 인증은 fail-closed 상태로 유지된다.

## 테스트

```powershell
npm test
npm run test:typecheck
npm run build
```

Workers 런타임과 로컬 D1에 실제 마이그레이션을 적용해 로그인, 병렬 실패 제한,
쿠키 속성, 로그아웃 세션 폐기, 비공개 카탈로그 경계를 검증한다.
