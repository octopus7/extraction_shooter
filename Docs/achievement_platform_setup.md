# Steam / STOVE 도전과제 설정 가이드

운영자가 수행할 설정 절차다. 이번 작업은 스토어 등록·수정·게시를 하지 않았고 플랫폼 ID도 확정하지 않았다. 내부 `01`–`20`과 스토어 API ID는 별개다. 확정 명칭은 `Content/Data/UITextStrings.csv`의 19개 `achievement.NN.name`을 사용한다. 14는 등록하지 않는다. 아이콘·공개/숨김 정책·다국어 조건 설명은 별도 승인이 필요하다.

## Steam

Steamworks 해당 앱의 Stats & Achievements에서 승인된 항목을 등록한다. API Name, 이름/설명, 표시 정책과 아이콘을 확인하고 변경을 게시해야 API가 항목을 찾을 수 있다. [공식 안내](https://partner.steamgames.com/doc/features/achievements), [ISteamUserStats API](https://partner.steamgames.com/doc/api/ISteamUserStats).

1. 실제 등록한 API Name을 JSON의 해당 항목 `platform_ids`에 `Steam` 키로 넣는다. 임의의 예시 ID를 확정 값으로 쓰지 않는다.
2. 배포 대상 `[OnlineSubsystemSteam]`의 `Achievement_0_Id`부터 연속 인덱스로 같은 ID 집합을 등록한다. 비활성/예약 항목은 발행 목록에서 제외한다.
3. 기존 publisher가 IOnlineAchievements의 QueryAchievements/WriteAchievements를 호출한다. Steam 호출을 중복 추가하지 않는다. JSON과 설정 ID 집합이 다르면 발행을 중단한다.
4. 테스트 계정으로 최초 달성, 재실행, 중복 이벤트, 오프라인 달성 후 온라인 복귀를 검증한다. 실패한 전송은 로컬 해금과 플랫폼 확인의 차이로 대기 상태가 유지된다. 계정/배포 대상도 확인한다.

로컬 자동화 성공은 Steam 서버 검증을 뜻하지 않는다.

## STOVE

STOVE Studio의 게임 통계/업적 설정에서 승인된 통계·업적 ID와 목표/갱신 방식을 지정하고 명칭·설명·아이콘을 입력한다. 통계와 업적 설정을 함께 확인한다. [Studio 업적 안내](https://studiodocs.onstove.com/1772c8ef-e31b-44da-b9c0-87adefb92691), [PC SDK v3 GameSupport 안내](https://developers-beta.onstove.com/ko/docs/Store/PCSDK3/PCSDK_v3_gamesupport).

현재 프로젝트는 BaseSDK/OwnershipSDK를 사용한다. PC SDK 3.4.2의 GameSupportSDK.h는 로컬에 있지만 업적용 초기화·조회·통계 갱신 어댑터와 빌드/런타임 모듈 연결은 아직 없다. JSON에 STOVE ID만 넣어도 작동하는 상태는 아니다.

후속 어댑터는 기존 `ITunaSweeperAchievementPublisher`를 구현한다. GameSupport 초기화/콜백 수명을 STOVE 런타임에 맞추고, 실제 SDK의 업적 조회 및 승인된 통계 갱신으로 해금을 확정한다. 콜백은 Game Thread로 전달하고 서버 성공을 확인한 뒤 Completion(true)을 호출한다. 없는 ID나 실패를 성공으로 취급하지 않는다. 게임플레이 판정은 공통 시스템을 그대로 사용한다.

어댑터가 준비되면 실제 등록된 ID를 어댑터가 반환하는 플랫폼 이름과 정확히 같은 `platform_ids` 키에 넣는다. 키나 업적 ID를 미리 가정하지 않는다. 테스트 계정에서 최초 달성·서버 확인·오프라인 재시도·재실행·계정 경계·중복 알림을 검증한다.

## 공통

- 내부 저장 ID는 명칭 변경과 무관하게 유지한다.
- 매핑 전에 조건의 enabled와 실제 대상 연결을 검증한다. 미정 시설/호수 조건을 플랫폼 등록으로 대신 해결하지 않는다.
- 11·12·17은 게임이 확정한 조건을 발행한다. 플랫폼 누적 통계로 순간 체력·동시 보유량·동일 레이드 조건을 추측하지 않는다.
- 게임의 토스트와 플랫폼 자체 오버레이는 별도 표시 계층이다. 프로젝트 이벤트 중복 방지가 플랫폼 오버레이 억제를 뜻하지는 않는다.
- 자격 증명·비밀 키는 문서나 추적 파일에 기록하지 않는다.
