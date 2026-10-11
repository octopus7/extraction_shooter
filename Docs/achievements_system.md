# 도전과제 시스템 v0.5

기준: 사용자가 제공한 2026-10-11 기획 목록, UE 5.7. 내부 ID는 문자열 `01`–`20`이며 플랫폼 식별자와 분리한다. `14`는 명칭·번역·조건 없이 예약한다. 확정된 한국어·영어·일본어 명칭 19개는 `Content/Data/UITextStrings.csv`의 `achievement.NN.name` 키에 원문 그대로 보관한다.

## 기존 구조 분석과 재사용

| 기존 기능 | 확인 내용 / 사용 방법 |
|---|---|
| AchievementSubsystem / Model | 기존 JSON 판정기, 독립 저장, 중복 방지와 플랫폼 큐 확장. 기존 정의 JSON은 비어 있었음 |
| EnemyCharacter::HandleDeath | 사망 중복 방지와 플레이어 귀속 판정 재사용. 도전과제에는 로컬 플레이어 처치만 보고 |
| QuestSubsystem | 진행 알림과 보상 수령 보고 존재. 기존 호환 기능은 보존하되 v0.5는 보상 수령 조건을 사용하지 않음 |
| ExtractionPointActor | 구역 내 홀드 완료 시 체력·시간·처치 수를 캡처하고 목적지 도착 시 확정 |
| GameInstance 경험치 | 레이드 경험치 시작/정산은 이동 요청 때 실행되므로 도전과제의 진입/탈출 판정으로 사용하지 않음 |
| GameInstance 인벤토리 | OnInventoryStateChanged 구독. 휴대·장비·보조 가방·퀵슬롯·기지 창고에서 UID 중복 제거 후 현재 수량 합산. 루팅 상자·상점 제외 |
| 월드 진행 | OnWorldProgressChanged 구독. 승인된 전체 시설 ObjectId 목록이 현재 슬롯에서 모두 Completed인지 확인 |
| AchievementLocationTrigger | 로컬 플레이어 실제 overlap 재사용. 지역 경계와 호수에 배치 가능 |
| TextSubsystem / ToastSubsystem | 기존 문자열 조회와 토스트 재사용. 알림 템플릿도 문자열 테이블에 추가 |
| SafeSave | CRC, candidate/previous 검증·교체·복원 재사용. v1 읽기와 v2 갱신 지원 |
| 플랫폼 | 기존 IOnlineAchievements publisher와 전송 대기/확인 기록 재사용. STOVE GameSupport 업적 어댑터는 아직 없음 |

BabyBoar 블루프린트, 리프트 시퀀스/맵, LoopRail 열차·역, MainRaid 맵과 보스 테스트 애셋의 존재를 확인했다. 애셋 이름만으로 합류 확정, 실제 사용, 지역 번호 또는 보스 정체성을 추측하지 않는다. LoopRail은 물리 열차 이동이며 빠른 이동 완료 이벤트는 확인되지 않았다. ATV 피해 수신 코드는 있지만 충돌로 적에게 피해를 주는 경로는 확인되지 않았다. 대전차 RPG 대상/발사기와 전체 엔딩 확정 이벤트도 연결 대기다. 데모 엔딩을 전체 엔딩의 대체 이벤트로 쓰지 않는다.

## 도전과제별 상태

아래 완료는 코드와 현재 게임 경로의 연결을 뜻한다. PIE·패키지 플레이 검증은 별도로 구분한다.

| ID | 명칭 | 조건 | 상태 / 남은 작업 |
|---|---|---|---|
| 01 | 첫 외출 | 1지역 최초 진입 | 연결 대기: 경계 LocationId 또는 정확한 map_paths |
| 02 | 붉은 신고식 | 최초 적 처치 | 완료: 실제 플레이어 귀속 사망 |
| 03 | 새로운 가족 | 새끼 멧돼지 합류 | 연결 대기: CharacterJoined 대상과 확정 이벤트 |
| 04 | 발사 | 실제 리프트 최초 사용 | 연결 대기: FacilityUsed 대상과 사용 이벤트 |
| 05 | 아늑한 집 | 1지역 모든 시설 복구 | 연결 대기: required_facility_ids 전체 목록 승인. 빈 목록은 달성 불가 |
| 06 | 골목대장 | 1지역 보스 처치 | 연결 대기: 실제 EnemyId |
| 07 | 마을 | 2지역 최초 진입 | 연결 대기: 경계/맵 확정 |
| 08 | 또 다른 가족? | AI 자판기 기지 이전 | 연결 대기: FacilityRelocated 대상과 이전 확정 |
| 09 | 문명의 맛 | 순환철도 빠른 이동 실제 이용 | 연결 대기: FastTravelUsed 완료 이벤트 |
| 10 | 토끼사냥 | 토끼대장 처치 | 연결 대기: 실제 EnemyId |
| 11 | 구사일생 | 탈출 성공 때 HP / MaxHP ≤ 0.10 | 완료: 살아 있는 플레이어의 홀드 완료 샘플과 도착 확정 |
| 12 | 퇴근합니다 | 동일 레이드 5명 이상, 295–305초 탈출 | 완료: 양끝 포함, 실제 진입 후 게임 시간 |
| 13 | 미지와 조우 | 호수 지정 구역 최초 진입 | 연결 대기: 범위와 LocationId, 트리거 배치 |
| 14 | 예약 | 없음 | 미구현(의도적 예약): 로컬·저장 복원·플랫폼 병합/전송 모두 차단 |
| 15 | 요술봉 | 대전차 RPG 실제 발사 | 연결 대기: 발사 성공 훅 구현, RPG ItemId와 전용 발사기 확정 필요 |
| 16 | 안전운전 | 차량 충돌로 아무 적 처치 | 연결 대기: 사망 원인 판정 구현, 차량 충돌 피해 발행자 필요 |
| 17 | 부자 | 참치 100개 동시 보유 | 완료: 사용자 확인한 참치 통조림 3004, 휴대 + 기지 창고 |
| 18 | 동네최강 | 적 100명 누적 처치 | 완료: 독립 계정 기록에 레이드 간 누적 |
| 19 | 황무지 | 3지역 최초 진입 | 연결 대기: 경계/맵 확정 |
| 20 | 결말 | 실제 전체 엔딩 도달 | 연결 대기: EndingReached 대상과 확정 이벤트 |

연결 대기 항목의 평가기와 API는 구현되어 있다. 16은 명시적인 차량 충돌 피해 타입만 인정하고 현재 발행자가 없어 달성되지 않는다. 나머지 미확정 대상은 `enabled:false`이며 플랫폼 매핑은 모두 비어 있다.

## 수동 연결

- 지역/호수: 기존 `ATunaSweeperAchievementLocationTrigger.LocationId`와 정의의 `target_id`를 승인된 값으로 맞춘다. 독립 맵이면 `map_paths`에 정확한 `/Game/...` 패키지를 지정할 수도 있다. 스트리밍 지역은 실제 경계 트리거를 사용한다. 처음부터 볼륨 안에 스폰하는 배치도 PIE로 점검한다.
- 시설: `required_facility_ids`에 시설 종류/표시 이름이 아닌 `WorldProgress`의 ObjectId를 넣는다. 한 슬롯의 현재 상태를 검사하므로 여러 슬롯의 부분 복구를 합치지 않는다.
- 합류·이전·리프트·빠른 이동·엔딩: 소유 시스템이 행동/상태를 확정한 코드 또는 BP에서 `ReportGameplayEvent(EventType, TargetId)`를 보고한다. 요청·수락·복구·보상 단계에서 호출하지 않는다.
- RPG: 승인된 양의 아이템 ID의 10진 문자열을 `target_id`로 지정한다. 일반 FireWeapon은 FireWithAimIntent 성공 후 `ReportWeaponFired(ItemId)`를 호출한다. 별도 RPG 발사기는 실제 발사 성공 후 같은 API로 연결한다.
- 로드킬: 차량 충돌 피해에만 `UTunaSweeperVehicleCollisionDamageType`과 실제 운전자 Controller를 전달한다. 차량 무기/폭발에는 이 타입을 쓰지 않는다. 이번 작업으로 차량 피해량이나 충돌 규칙은 추가하지 않았다.
- 새 탈출 경로: 실제 탈출 완료 시 `CaptureCompletedExtraction(Player, Destination)`을 호출한다. 일반 이동은 `DiscardPendingExtraction`으로 이전 탈출 증거를 폐기한다.

레이드 시작은 기존 `IsRaidGameplayLevelName` 정책을 재사용한다. 현재 레이드 월드 안의 스트리밍 지역 이동은 재시작하지 않는다. 새 독립 레이드 월드를 만들면 프로젝트 전체 레이드 식별 정책을 확장해야 한다. 지역 업적의 map_paths만 추가한다고 레이드가 정의되지는 않는다.

## 판정·저장·알림

시간은 `GetTimeSeconds()` 게임 시간 기준으로 일시정지와 시간 배율을 따른다. 로딩·복귀 연출 시간은 완료 샘플에 더하지 않는다. 홀드 완료 후 의도한 목적지에 도착해야 11·12를 확정한다. 사망 구조, 다른 목적지, 여행 실패, 일반 이동 요청은 성공이 아니다. TravelFailure와 일반 이동은 보류 증거를 폐기한다.

레이드 처치 수/시각·보류 탈출·현재 보유량·시설 스냅샷은 저장하지 않는다. 영구 누적 처치·게임플레이 사실·해금·플랫폼 확인은 기존 독립 저장 컨테이너에 보존하며 새 게임/슬롯 변경으로 초기화하지 않는다. 계정·스토어·Demo/Main 경계는 기존 정책을 유지한다. v1을 읽고 다음 저장에서 v2로 갱신하며 파일 교체 검증도 v1을 허용한다. 잘못 저장된 14번 해금은 복원 시 버린다.

새 해금은 저장 성공 후 한 번만 이벤트와 토스트로 알린다. 저장 실패 중 플랫폼 전송도 보류하며 다음 이벤트·맵 로드·종료에서 재시도한다. 종료 저장은 UI를 생성하지 않는다. 플랫폼이 없어도 로컬 판정·저장은 동작한다.

## 변경 파일

`Source/TunaSweeper`의 AchievementTypes, AchievementModel, AchievementSubsystem을 확장하고 `Private/Subsystem/TunaSweeperAchievementGameplay.cpp`를 추가했다. 실제 연결은 EnemyCharacter, TopDownCharacterWeapon/Vitals, PlayerController, ExtractionPointActor, LevelTravelInteractableActor에 최소한으로 추가했다. 차량 분류는 `Public/Vehicle/TunaSweeperVehicleCollisionDamageType.h`다. 테스트는 기존 AchievementTests와 새 `Private/Tests/TunaSweeperAchievementV05Tests.cpp`다.

데이터는 `Content/Data/AchievementDefinitions.json`, `Content/Data/UITextStrings.csv`. 퀘스트 데이터·선행 관계·맵 배치·게임 규칙은 변경하지 않았다.

## 검증 기록

2026-10-11 검증:

- UE 5.7 `TunaSweeperEditor Win64 Development` 빌드 성공(종료 0). 로컬 UBA/캐시 환경 문제를 피하도록 `-NoUBTMakefiles -NoUBA -NoUbaLocal` 사용. 로그: `Saved/Logs/AchievementsBuild.log`.
- UnrealEditor-Cmd의 `TunaSweeper.Achievement.` 자동화 10/10 성공(종료 0): Collection, DefinitionValidation, PublishingState, ReservedSlot, CatalogAndLocalization, CurrentStateAndFacts, ExtractionBoundaries, ExtractionLifecycle, PersistenceRoundTrip, VersionOneUpgrade. 로그: `Saved/Logs/AchievementsTests.log`.
- 별도 새 프로세스에서 `TunaSweeper.AchievementRestart.Read` 1/1 성공(종료 0). 앞 프로세스가 저장한 SafeSave 파일을 읽어 해금 상태·누적 100처치 복원과 14번 제외 검증. 로그: `Saved/Logs/AchievementsRestart.log`.
- 최초/중복 해금, 실패·사망·다른 목적지와 성공 탈출 구분, 레이드 통계 초기화, 체력 10%와 295/305초 양끝·범위 밖, 현재 보유량과 누적량 구분, 비활성·예약 차단, 19개 × 3언어 실데이터 로딩, v1 실제 파일의 v2 교체를 검증했다.
- 코드 리뷰에서 발견된 v1 교체 검증 거부와 이동 실패 후 탈출 증거 잔류를 수정하고 회귀 테스트를 추가했다. `git diff --check` 통과.

자동화는 NullRHI의 모델/서브시스템·파일 저장 검증이다. 실제 플레이어 이동·전투·인벤토리 UI를 조작하는 PIE는 실행하지 않았다. 패키징, 렌더링 토스트 육안 확인, Steam/STOVE 서버 검증도 미실행이다. 자동화 저장은 `Saved/Automation` 전용이며 실제 플레이 저장을 변경하지 않는다.

후속 순서: 시설 목록/지역/보스 ID 승인 → 실제 행동·상태 확정 이벤트 연결 → 호수/지역 경계 배치와 PIE → 플랫폼 등록·STOVE 어댑터 → 테스트 계정 검증. 플랫폼 설정은 [설정 가이드](achievement_platform_setup.md)를 따른다.
