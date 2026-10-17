# 본편 지역 이동

2026-10-11 기준. 본편의 플레이 지역은 `RaidForest`, `RaidVillage`, `RaidPlains`다. `RaidMap`은 개발용으로 보존하고 실제 게임의 이동 대상과 기본 쿠킹 맵 목록에서 제외했다. 이전의 논리 목적지 `RaidMap`은 본편에서 숲으로 해석한다. Demo의 이동 맵 선택은 기존 설정을 유지한다.

## 연결

| 출발 | 도착 | 조건 |
| --- | --- | --- |
| 기지 | 숲 | 기존 출구에서 F 상호작용 |
| 숲 | 기지 | `Forest_BaseEntrance`에서 F 상호작용 |
| 숲 | 마을 | 기존 통로 완료 상태를 확인한 뒤 F 상호작용. 장애물 구현 제거로 새 완료 수단은 미연결 |
| 숲 | 평원 | 승강기에서 고장력 강선 케이블(6010) 1개와 고출력 모터(6034) 1개를 함께 소모해 수리한 뒤 F 이동 상호작용 |
| 숲·마을·평원 | 기지 | 익스트랙션 포인트에서 4초 대기 완료 |

마을→숲, 평원→숲의 별도 통로는 현재 요구에 포함되지 않았다. 숲의 F 상호작용 귀환과 익스트랙션 귀환은 별개다. F 상호작용 귀환은 익스트랙션 완료로 계산하지 않는다.

## 배치와 BP

새 액터는 각 맵의 `RegionTravel` 폴더에 있다. 아래 좌표는 2026-10-12 맵 축소 후 cm 단위이며 이동·회전은 에디터에서 변경 가능하다. 맵 크기와 원래 좌표 복원 자료는 [main_raid_layout.md](main_raid_layout.md)를 따른다.

| 맵 | 액터 라벨 | 위치 X, Y, Z |
| --- | --- | --- |
| RaidForest | Forest_BaseEntrance | -9080.16, -2334.13, 335.87 |
| RaidForest | Forest_VillagePassage | 7142.86, 1388.89, -0.31 |
| RaidForest | Forest_PlainsLift | 5158.73, -4761.90, 351.25 |
| RaidForest | RaidForest_Extraction_1 | -3968.25, 1984.13, 58.40 |
| RaidVillage | RaidVillage_Extraction_1 | -1845, -485, 163.06 |
| RaidVillage | RaidVillage_Extraction_2 | 1080, 1260, 152.49 |
| RaidPlains | RaidPlains_Extraction_1 | 992.06, 1785.71, 51.63 |

`/Game/MainRaid/RegionTravel`에 `BP_ForestReturn`, `BP_ForestToVillage`, `BP_ForestToPlainsLift`가 있다. 메시·재료는 같은 폴더의 복셀 형태 임시 애셋이다. 익스트랙션은 기존 `/Game/Interaction/BP_ExtractionPoint`를 사용하고 각 포인트 위치에 복셀 표시대를 별도 배치했다. 위치를 바꿀 때에는 같은 번호의 `ExtractionVoxel` 액터도 함께 옮긴다. 모든 익스트랙션 목적지는 `BunkerMap`이다.

통로 BP의 `Destination`, `Repair Parts`를 편집할 수 있다. 마을 통로는 기존 저장 ID `region.forest.village.obstacle`을 계속 읽는다. 2026-10-15 기존 폭파 장애물 대신 `/Game/Interaction/Rockfall/BP_ForestRockfallBarrier`를 연결했다. 터널 남쪽 400cm인 (6742.86, 1388.89, 0)에 `Forest_TunnelRockfall`로 배치했으며, 기존 저장 상태와 통로의 해금 조건은 유지한다. 승강기는 `region.forest.plains.lift`를 사용한다. 복제해 독립 통로로 쓸 때에는 새 저장 ID를 부여한다.

## 상태 유지

- 통로·수리 완료 상태는 기존 `WorldProgressStates`에 저장하여 다시 입장해도 유지한다.
- 승강기는 두 재료가 모두 있을 때만 한 번에 소모한다. 일부만 있을 때 소모하거나 이동하지 않는다.
- 지역 사이 이동은 레이드의 연속이다. 체력·허기·수분 비율과 레이드 경험치, 처치 수, 플레이 시간을 유지한다. 로딩·일시정지 시간을 업적용 플레이 시간에 더하지 않는다.
- 기지 귀환의 저장·경험치 정산과 익스트랙션 도착 확정 판정은 기존 경로를 사용한다.
- 개발용 RaidMap에 있는 기존 전투·루팅·퀘스트 배치를 새 지역에 복제하지 않았다.

검증은 에디터 빌드, 이동 경로·수리·폭파·생존 수치·업적·퀘스트 자동화 및 저장된 맵의 목적지·개수·장애물 충돌 확인으로 수행한다. 세 지역을 직접 순회하는 전체 플레이 검증과 배포 패키징은 별도다.

## 통로 개방 퀘스트 목표

`world_progress_completed` 목표는 `world_progress_id`에 지정한 월드 오브젝트의 `Completed` 상태를 인정한다. `required_count`는 생략하거나 1로 지정한다. 빈 ID, 숫자 ID, 1 이외의 수량은 로더가 거부한다. 목표 문구는 기존 `text_string_key`로 연결한다.

폭파 장애물의 `ProgressObjectId`, 승강기의 `RequiredProgressId`와 목표의 `world_progress_id`를 맞추면 기존 액터와 연결된다. 액터에 퀘스트 번호를 넣거나 별도 이벤트를 호출할 필요가 없다. 개방 전에 수락한 퀘스트는 개방 즉시, 먼저 개방한 경우는 나중에 수락하는 시점에 인정한다. 저장 복원은 현재 맵에 액터가 없어도 저장된 상태로 판정한다.

완료는 보상 대기 상태까지만 진행하며 기존 선행 조건·수락 경로·보상 수령 절차를 유지한다. 미수락 퀘스트를 자동 수락하거나 다른 목표를 생략하지 않는다. 반복 알림이나 재입장은 진행값과 보상을 중복 지급하지 않는다.

## 시작점 주변 시설 배치 (2026-10-12)

숲의 PlayerStart는 (-22000, -5000, 60)이다. `QuestFacilities/Forest_PrimaryPowerFacility`에 기존 `/Game/Interaction/BP_PowerFacility_Primary`를 (-21650, -4550, -67.54)에 배치했다. 시작점과의 거리는 약 5.84m이며, 기지 입구는 약 5.07m다. 입구의 뒤집힌 회전을 바로잡고 두 메시의 바닥을 지형에 맞췄다. 저장한 맵을 다시 열어 거리·배치 유일성·지면 접촉·시작점과 귀환 트리거 분리를 확인했다. 실제 수리 및 귀환 플레이 검증은 별도다.

위 문단은 최초 배치 기록이다. 맵 축소 후 시작점은 (-8730.16, -1984.13, 108.07), 전력 시설은 (-8380.16, -1534.13, -8.23)이다. 시작점에 대한 두 시설의 XY 간격은 유지하고 각 지점의 새 지형 높이에 맞췄다.

2026-10-12 수정: 일반 이동 액터는 F 상호작용으로만 이동한다. 거리 검사·WalkArea 중첩 자동 이동 코드와 설정을 제거했으므로 기존 BP/맵에 저장된 자동 통과 값도 동작하지 않는다. 익스트랙션 포인트의 4초 대기 귀환은 유지한다.

## 숲 보급 상자 (2026-10-12)

북쪽(+X) 빈 지형에 개방형 임시 창고를 배치했다. 중심은 (6000, 1400, 3.92)이며 `SupplyCaches/NorthWarehouse` 폴더의 기본 블록 메시로 구성한다. 기존 나무는 유지한다.

- `Forest_SupplyCache_North`: (6120, 1400, 5.92), ContentsId 8016, 캔참치(3004) 1개 확정.
- `Forest_ToolCache_Power`: (-8130.16, -1534.13, -14.26), ContentsId 8017, 망치(6020) 1개 확정. 현재 전력 시설에서 약 2.5m.

두 상자는 기존 BP_LootContainer를 직접 배치하며 PlacementId를 새로 할당하지 않는다. 상자 내용은 기존 수동 루팅과 런타임 슬롯 유지 규칙을 따른다. 확률은 프로젝트 10000분율 기준 10000이다.

## 맵 진입 판정 (2026-10-12)

퀘스트의 레벨 비교는 실제 이동 경로와 목표 양쪽에서 패키지 경로와 PIE 접두어를 제거한 맵 이름을 비교한다. 예를 들어 `RaidForest`, `/Game/MainRaid/RaidForest`, `/Game/MainRaid/UEDPIE_0_RaidForest`는 같은 맵이다. 임의의 접미어 일치는 허용하지 않으므로 `Other_RaidForest`는 해당하지 않는다. 기존 목표의 논리 이름 `RaidMap`에 대한 레이드 지역 호환 처리는 유지하지만, 명시적인 `RaidForest` 목표가 개발용 `RaidMap`에서 완료되지는 않는다.

일반 이동 퀘스트의 판정 주체는 `UTunaSweeperQuestSubsystem`이며 기존 이동 요청 알림 시점을 유지한다. 지역 진입 도전과제는 `UTunaSweeperAchievementSubsystem`이 실제 게임 월드 준비 완료를 받은 뒤 PIE 접두어를 제외한 전체 패키지 경로로 판정한다.

| 도전과제 ID | 도착 맵 | 저장 지역 ID |
| --- | --- | --- |
| 01 | `/Game/MainRaid/RaidForest` | `region.forest` |
| 07 | `/Game/MainRaid/RaidVillage` | `region.village` |
| 19 | `/Game/MainRaid/RaidPlains` | `region.plains` |

개발용 `RaidMap`은 위 세 도전과제에 포함하지 않는다. 플랫폼 도전과제 ID 연결은 기존 상태를 유지한다.

## 터널 낙석과 고농축 폭발물 (2026-10-15)

- 좌우 절벽은 영구 유지하고 중앙 낙석과 차단 충돌만 제거한다. BP의 메시·위치·폭발 반경과 피해는 에디터에서 조정할 수 있다.
- 고농축 폭발물(6031) 1개를 소지하고 상호작용하면 소모·장전 상태를 함께 저장한다. 5초 뒤 폭발하며, 완료 저장에 성공해야 통로 개방·이펙트·피해를 적용한다.
- 지면에는 처음부터 반경 400cm의 흐린 붉은 최대 영역이 표시되고, 내부가 1초 동안 차오른다. 전용 줄무늬 텍스처의 UV 이동과 경계 맥동은 폭발까지 유지한다. 피해 기본값은 중심 60·외곽 15이며 설치자도 대상이다.
- 장전 도중 종료하면 다음 로드에서 추가 소모 없이 5초를 다시 센다. 이미 개방된 저장에서는 폭발이나 피해를 반복하지 않는다.
- 작업대 1에서 분홍 뿡뿡(6028) 1개 + 탁상시계(6032) 1개로 고농축 폭발물 1개를 제작한다. 레시피는 기본 공개한다.
- 아이콘 원본은 `TunaSweeper/SourceArt/UI/Items/Rockfall`, UE 텍스처는 `/Game/UI/Icons/T_UIIcon_ConcentratedExplosive`와 `T_UIIcon_DeskClock`이다. 탁상시계는 흰 앞 테두리·푸른 도색·철색 금속으로 표현한다.


## 수리 승강기 메시 교체와 입구 (2026-10-18)

`/Game/MainRaid/RegionTravel/BP_ForestToPlainsLift`의 `VisualMesh`는 문을 제외한 고정 본체, `LeftDoorMesh`와 `RightDoorMesh`는 좌우 문이다. 세 컴포넌트의 Static Mesh와 로컬 위치·크기를 각각 편집할 수 있다. 임시 본체는 `/Game/MainRaid/RegionTravel/SM_RepairableLift_Body`이며 원본 OBJ는 `SourceArt/Props/RepairableLift`에 있다. 바닥과 벽을 포함하고 내부가 비어 있어야 하며, 교체 본체도 출입구를 막지 않는 충돌을 설정한다.

입구는 액터 로컬 +X 방향이다. 문 메시의 정렬을 유지한 채 부모 `LeftDoorSlide`와 `RightDoorSlide`가 `LeftDoorOpenOffset`/`RightDoorOpenOffset`만큼 이동한다. `DoorOpenSeconds`로 개폐 시간을, `PreviewDoorsOpen`으로 에디터 표시를 조정한다. `TravelInterior` 박스는 실내 판정 범위, `RepairInteractionPoint`와 `InteriorInteractionPoint`는 각각 수리·이동 상호작용의 위치다.

수리 전에는 입구에서 기존 재료 수리 상호작용을 제공한다. 수리 완료 알림을 받으면 문이 자동으로 좌우로 열리고, 완전히 열린 후 실내에 들어간 플레이어에게만 평원행 F 상호작용을 제공한다. 실내 진입 자체로 이동하지 않는다. 저장된 수리 완료 상태에서는 처음부터 문을 열어 두고, 월드 진행 초기화 시 다시 닫는다. 일반 지역 통로는 `UseSlidingDoors=false`를 유지한다.
