# 본편 지역 이동

2026-10-11 기준. 본편의 플레이 지역은 `RaidForest`, `RaidVillage`, `RaidPlains`다. `RaidMap`은 개발용으로 보존하고 실제 게임의 이동 대상과 기본 쿠킹 맵 목록에서 제외했다. 이전의 논리 목적지 `RaidMap`은 본편에서 숲으로 해석한다. Demo의 이동 맵 선택은 기존 설정을 유지한다.

## 연결

| 출발 | 도착 | 조건 |
| --- | --- | --- |
| 기지 | 숲 | 기존 출구에 걸어 들어가거나 상호작용 |
| 숲 | 기지 | `Forest_BaseEntrance`에 걸어 들어가거나 상호작용 |
| 숲 | 마을 | 앞의 폭파 장애물을 수류탄 또는 다이너마이트 피해로 파괴한 뒤 통로 진입 |
| 숲 | 평원 | 승강기에서 고장력 강선 케이블(6010) 1개와 임시 재료 뿡뿡(6006) 1개를 함께 소모해 수리한 뒤 이동 상호작용 |
| 숲·마을·평원 | 기지 | 익스트랙션 포인트에서 4초 대기 완료 |

마을→숲, 평원→숲의 별도 통로는 현재 요구에 포함되지 않았다. 숲의 보행 귀환과 익스트랙션 귀환은 별개다. 보행 귀환은 익스트랙션 완료로 계산하지 않는다.

## 배치와 BP

새 액터는 각 맵의 `RegionTravel` 폴더에 있다. 아래 좌표는 cm이며 이동·회전은 에디터에서 변경 가능하다.

| 맵 | 액터 라벨 | 위치 X, Y, Z |
| --- | --- | --- |
| RaidForest | Forest_BaseEntrance | -22900, -5000, 104.68 |
| RaidForest | Forest_VillagePassage | 18000, 3500, -0.78 |
| RaidForest | Forest_VillageBombObstacle | 17600, 3500, -17.97 |
| RaidForest | Forest_PlainsLift | 13000, -12000, 885.15 |
| RaidForest | RaidForest_Extraction_1 | -10000, 5000, 95.16 |
| RaidVillage | RaidVillage_Extraction_1 | -20500, -11500, 856.87 |
| RaidVillage | RaidVillage_Extraction_2 | 12000, 14000, 253.75 |
| RaidPlains | RaidPlains_Extraction_1 | 2500, 4500, 68.59 |

`/Game/MainRaid/RegionTravel`에 `BP_ForestReturn`, `BP_ForestToVillage`, `BP_ForestBombObstacle`, `BP_ForestToPlainsLift`가 있다. 메시·재료는 같은 폴더의 복셀 형태 임시 애셋이다. 익스트랙션은 기존 `/Game/Interaction/BP_ExtractionPoint`를 사용하고 각 포인트 위치에 복셀 표시대를 별도 배치했다. 위치를 바꿀 때에는 같은 번호의 `ExtractionVoxel` 액터도 함께 옮긴다. 모든 익스트랙션 목적지는 `BunkerMap`이다.

통로 BP의 `Destination`, `Walk Through`, `Walk Area`, `Repair Parts`를 편집할 수 있다. 장애물과 마을 통로의 저장 ID `region.forest.village.obstacle`은 반드시 일치해야 한다. 승강기는 `region.forest.plains.lift`를 사용한다. 복제해 독립 통로로 쓸 때에는 새 저장 ID를 부여한다.

## 상태 유지

- 폭파·수리 완료는 기존 `WorldProgressStates`에 저장하여 다시 입장해도 유지한다.
- 승강기는 두 재료가 모두 있을 때만 한 번에 소모한다. 일부만 있을 때 소모하거나 이동하지 않는다.
- 지역 사이 이동은 레이드의 연속이다. 체력·허기·수분 비율과 레이드 경험치, 처치 수, 플레이 시간을 유지한다. 로딩·일시정지 시간을 업적용 플레이 시간에 더하지 않는다.
- 기지 귀환의 저장·경험치 정산과 익스트랙션 도착 확정 판정은 기존 경로를 사용한다.
- 개발용 RaidMap에 있는 기존 전투·루팅·퀘스트 배치를 새 지역에 복제하지 않았다.

검증은 에디터 빌드, 이동 경로·수리·폭파·생존 수치·업적·퀘스트 자동화 및 저장된 맵의 목적지·개수·장애물 충돌 확인으로 수행한다. 세 지역을 직접 순회하는 전체 플레이 검증과 배포 패키징은 별도다.

## 통로 개방 퀘스트 목표

`world_progress_completed` 목표는 `world_progress_id`에 지정한 월드 오브젝트의 `Completed` 상태를 인정한다. `required_count`는 생략하거나 1로 지정한다. 빈 ID, 숫자 ID, 1 이외의 수량은 로더가 거부한다. 목표 문구는 기존 `text_string_key`로 연결한다.

폭파 장애물의 `ProgressObjectId`, 승강기의 `RequiredProgressId`와 목표의 `world_progress_id`를 맞추면 기존 액터와 연결된다. 액터에 퀘스트 번호를 넣거나 별도 이벤트를 호출할 필요가 없다. 개방 전에 수락한 퀘스트는 개방 즉시, 먼저 개방한 경우는 나중에 수락하는 시점에 인정한다. 저장 복원은 현재 맵에 액터가 없어도 저장된 상태로 판정한다.

완료는 보상 대기 상태까지만 진행하며 기존 선행 조건·수락 경로·보상 수령 절차를 유지한다. 미수락 퀘스트를 자동 수락하거나 다른 목표를 생략하지 않는다. 반복 알림이나 재입장은 진행값과 보상을 중복 지급하지 않는다.
