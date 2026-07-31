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

## 인증

현재 배포는 `ACCESS_TEAM_DOMAIN`과 `ACCESS_AUD`가 비어 있어 인증 기능이
fail-closed 상태입니다. 따라서 비로그인 체험판만 공개되고, M01~M20과 개인
작업공간 API는 차단됩니다. Cloudflare Access 애플리케이션을 만든 뒤 두 값을
설정하고 다시 배포하면 Access JWT 검증을 통해 인증 전용 카탈로그와 D1 저장을
사용할 수 있습니다.
