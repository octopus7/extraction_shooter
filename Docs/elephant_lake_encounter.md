# 숲 호수와 미지와 조우

- 레벨: `/Game/MainRaid/RaidForest`.
- 확장 전 호수 중심: `(-4580.562, 6200, 0)` cm. 사용자 지정 코끼리 위치 `(-5630.562, 6200, 0)`에 맞춰 세 요소를 X축으로 `-9980.562` cm 평행 이동했다. 이후 코끼리 쪽 기슭을 기준으로 호수의 X 길이를 2배, Y 길이를 4.5배 넓혔다. 코끼리와 도전과제 트리거는 원래 위치에 있다.
- 숲맵의 호수는 `Water_06_ElephantLake`가 표시한다. 수면 아래 지형까지의 깊이가 색과 투과율을 결정하며 물가 메시가 흰 물결을 그린다. 기존 15×10m 이미지 데칼 `Forest_ElephantLake_Decal`은 이 레벨에서 숨겼다. 편집 방법은 [숲맵 수역](forest_water.md)에 정리했다.
- `Forest_LakeElephant`: `SkeletalMeshActor`로 배치한 StarPondElephant. `SkeletalMeshComponent`가 `/Game/Astra/Characters/StarPond/Guardian/SK_SP_UnicornElephant`와 8초 반복 대기 애니메이션 `A_SP_UnicornElephant_Idle`을 사용한다. 인스턴스의 NearbyPlayerFacingComponent가 수평거리 4m에서 플레이어를 따라 돌기 시작한다. 5m를 벗어나면 2초 기다렸다가 배치 방향으로 돌아간다. 이 스켈레톤의 유일한 애니메이션인 8초 대기 루프는 회전 중에도 계속 재생된다. 달성 판정은 외형과 분리돼 있다.
- `Forest_ElephantLake_Achievement13`: 기존 `TunaSweeperAchievementLocationTrigger`. 중심 `(-4580.562, 6200, 200)`, 반크기 `(1300, 850, 300)` cm의 호숫가 영역. `LocationId=location.forest.elephant_lake`.

13번은 숲을 로드했다는 이유로 달성되지 않는다. 로컬 플레이어가 트리거에 겹치면 기존 서브시스템에 위치 도착을 보고하고, 독립 도전과제 저장 경로를 통해 기록한다. 이미 달성한 뒤 재진입해도 다시 알리지 않는다. 새 저장 필드나 초기화 치트는 추가하지 않았다.

## 코끼리 원본

별도 UE 5.7 프로젝트 AstraLevelTest에서 만든 StarPondElephant를 가져왔다. 스켈레탈 메시, 16개 본의 스켈레톤, 대기 애니메이션 및 머터리얼 7개를 원래 패키지 경로에 함께 배치한다.

## 검증

- 저장된 숲맵을 다시 열어 스켈레탈 메시와 대기 애니메이션 할당을 확인하고, 호숫가 플레이 화면에서 모델과 반복 재생을 확인한다.
- 새 UE 에디터에서 호수 BP의 수면·물가·바닥 소품, 기존 데칼의 표시 상태, 트리거 ID와 크기, 기존 액터 위치를 검사하고 렌더 결과를 확인한다.
- `TunaSweeper.ElephantLake.PlayerEntry`는 실제 RaidForest PIE에서 시작/영역 밖/비플레이어 overlap 미달성, 플레이어 진입 달성을 검사한다. 반드시 매번 비어 있는 `ElephantLakeAutomation` 포함 `-UserDir`로 실행한다.
- `TunaSweeper.Achievement.ElephantLakeDefinition`은 실제 JSON을 읽어 숲 진입과 호수 진입을 구분한다. 기존 PersistenceRoundTrip/별도 프로세스 AchievementRestart.Read는 13번 해금과 위치 기록의 보존을 포함한다.
- Steam 서버 등록은 별도 플랫폼 매핑 설정이 필요하며 로컬 PIE 성공이 Steam 서버 달성 성공을 뜻하지 않는다.
