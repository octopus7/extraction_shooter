# 숲 호수와 미지와 조우

- 레벨: `/Game/MainRaid/RaidForest`.
- 호수 중심: `(-4580.562, 6200, 0)` cm. 사용자 지정 코끼리 위치 `(-5630.562, 6200, 0)`에 맞춰 세 요소를 X축으로 `-9980.562` cm 평행 이동했다. 기존 62개 액터의 위치는 유지했다.
- 숲맵의 호수는 `Water_06_ElephantLake`가 표시한다. 수면 아래 지형까지의 깊이가 색과 투과율을 결정하며 물가 메시가 흰 물결을 그린다. 기존 15×10m 이미지 데칼 `Forest_ElephantLake_Decal`은 이 레벨에서 숨겼다. 편집 방법은 [숲맵 수역](forest_water.md)에 정리했다.
- `Forest_LakeElephant`: `/Game/Environment/ElephantLake/BP_LakeElephant`. StaticMeshActor 기반 BP의 StaticMeshComponent에서 메시와 머터리얼을 교체할 수 있다. 달성 판정은 외형과 분리돼 있다.
- `Forest_ElephantLake_Achievement13`: 기존 `TunaSweeperAchievementLocationTrigger`. 중심 `(-4580.562, 6200, 200)`, 반크기 `(1300, 850, 300)` cm의 호숫가 영역. `LocationId=location.forest.elephant_lake`.

13번은 숲을 로드했다는 이유로 달성되지 않는다. 로컬 플레이어가 트리거에 겹치면 기존 서브시스템에 위치 도착을 보고하고, 독립 도전과제 저장 경로를 통해 기록한다. 이미 달성한 뒤 재진입해도 다시 알리지 않는다. 새 저장 필드나 초기화 치트는 추가하지 않았다.

## 코끼리 원본

`TunaSweeper/SourceArt/Characters/LakeElephant`에 blend, FBX, 팔레트 PNG, 미리보기, manifest 및 새 Blender 프로세스 검증 결과를 보관한다. 한 격자의 31×22×29셀, 셀 크기 10cm, 310×220×290cm, 2,128삼각형이다. 내부 면은 제거하고 같은 평면·색의 인접 면을 합쳤다. 머터리얼 한 개와 64×8 불투명 팔레트 한 개만 사용하며 UV0의 각 면은 색 칸 내부의 면적 있는 사각형이다. UE 팔레트는 nearest/no-mip 설정이다.

## 검증

- 새 Blender 프로세스에서 blend와 FBX의 격자, 크기, UV 면적, 유한한 노멀/탄젠트, 단일 머터리얼을 검사한다.
- 새 UE 에디터에서 호수 BP의 수면·물가·바닥 소품, 기존 데칼의 표시 상태, 트리거 ID와 크기, 기존 액터 위치를 검사하고 렌더 결과를 확인한다.
- `TunaSweeper.ElephantLake.PlayerEntry`는 실제 RaidForest PIE에서 시작/영역 밖/비플레이어 overlap 미달성, 플레이어 진입 달성을 검사한다. 반드시 매번 비어 있는 `ElephantLakeAutomation` 포함 `-UserDir`로 실행한다.
- `TunaSweeper.Achievement.ElephantLakeDefinition`은 실제 JSON을 읽어 숲 진입과 호수 진입을 구분한다. 기존 PersistenceRoundTrip/별도 프로세스 AchievementRestart.Read는 13번 해금과 위치 기록의 보존을 포함한다.
- Steam 서버 등록은 별도 플랫폼 매핑 설정이 필요하며 로컬 PIE 성공이 Steam 서버 달성 성공을 뜻하지 않는다.
