# 택배 상자 획득

`ATunaSweeperParcelContainerActor`는 기존 루팅 상자를 상속한다. 열기 상호작용으로 내용물을 인벤토리로 이동하며, 일반 상자는 기존 수동 루팅을 유지한다.

## 배치용 프리셋

| Blueprint | Container Definition ID | Contents ID | 지급 아이템 |
| --- | --- | --- | --- |
| `/Game/Interaction/Parcels/BP_Parcel_ToadJar` | 7004 | 8014 | 두꺼비 유리병(6011) 1개 |
| `/Game/Interaction/Parcels/BP_Parcel_ModelBoat` | 7004 | 8015 | 모형배(6017) 1개 |

외형은 기존 종이박스 정의(7004)와 재질을 사용한다. 내용물은 `Content/Data/LootContainerContents.json`에서 지정한다.

퀘스트 조건부 배치에는 기존 Loot Container 앵커와 `LootContainerSpawns.json`의 `required_active_quest_id`를 사용한다. `loot_container_class`에 위 프리셋의 생성 클래스를 지정하고, `container_definition_id`와 `contents_id`도 표와 맞춘다. 스폰 데이터가 Blueprint 기본값을 덮어쓴다. 실제 레벨·앵커 ID는 배치 작업에서 지정하며 [앵커 번호 규칙](raid_placement_id_numbering.md)을 따른다.

## 획득 동작

- 기존 루팅 슬롯 이동 경로로 지급하므로 획득 기록과 퀘스트 아이템 획득 알림이 유지된다.
- 공간이 부족하면 가져가지 못한 아이템을 상자에 남기고 기존 루팅 화면을 연다. 플레이어의 아이템을 상자로 교환하지 않는다.
- 같은 상자를 다시 열어도 가져간 내용물이 다시 생성되지 않는다. 기존 상자의 월드 내 내용물 보존 방식을 사용한다.
- 기지 귀환 시 제거, 레이드 레벨 간 아이템 유지, 목적지 전달 완료는 별도 수명·퀘스트 처리 범위다. 이 프리셋 추가는 해당 동작이나 실제 맵 배치를 구현하지 않는다.
