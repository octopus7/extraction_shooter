# 구현 검증 기록

- 승인된 설계·계획에 따라 Worker, UI, Unreal 전송부를 병렬 구현.
- 이벤트 수신 테스트: stub 501에서 6개 실패 확인 후 구현하여 통과.
- 통계·인증 테스트: 미구현 404에서 8개 실패 확인 후 구현하여 통과.
- Node/SQLite 테스트와 문자열·구문 검증 통과. 실제 Cloudflare 배포는 하지 않음.
- UI 브라우저에서 인증·필터·페이지·누락 상세·모바일 배치 검증.
- Unreal Editor 빌드 및 트래커 계약·엔딩 저장 정책·버전 정책 자동화 통과.
- 별도 코드 리뷰에서 완료를 차단할 결함 없음. 장기 대규모 데이터의 조회·보관 확장은 README에 기록.

## 구현 결정

- 테스트에는 Node 내장 SQLite 사용: 로컬 Wrangler 실행 없이 실제 SQL의 결과를 검증한다.
- 순수 JavaScript/Web API 구현으로 Node 호환 플래그나 서버 런타임 패키지를 요구하지 않는다.
- 관리자 조회는 Web Crypto HMAC 검증으로 토큰을 비교한다.
- 진행 상세 경로는 /api/admin/run 및 복합 식별자 쿼리로 정해 서로 다른 빌드·진행이 섞이지 않게 한다.
- 추가 로컬 Miniflare 검증은 설치된 workerd의 최대 호환 날짜가 2026-08-06이고 이후 시도에서 응답 대기가 끝나지 않아 완료하지 못했다. 배포용 2026-09-30 설정은 유지하며 실제 Cloudflare 런타임 통과를 주장하지 않는다.

## 최종 확인

- Worker Node 테스트 19/19, npm run check 통과.
- 폴더만 임시 경로에 복사한 상태에서 동일 테스트와 검사 통과 후 임시 복사본 정리.
- 데스크톱·390×844 모바일에서 인증, 빌드 필터, 진행 상세, 페이지 이동 확인. 0건 데이터는 API 검증 통과; 후속 브라우저 연결이 없어 빈 화면 추가 시각 확인은 미실시.
- UE5.7 Editor 빌드 성공, TunaSweeper.ProgressTracker.Contract / TunaSweeper.Save.DemoEndingRetirementGuards / TunaSweeper.Save.VersionPolicy 3개 성공, TEST COMPLETE EXIT CODE 0.
- 원래 TunaSweeper 프로젝트의 기존 GUI Editor 실행을 확인해 유지. 변경된 별도 워크트리는 UnrealEditor-Cmd로 검증.
