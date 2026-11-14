# 식량 창고

과거 데모 식사 엔딩과 작별 UI·삽화·세이브 삭제 처리는 제거되었다. 식량 창고는 독립적인 퀘스트 상호작용으로 남아 있다.

- BP: `/Game/Interaction/DemoEnding/BP_FoodWarehouse` (`ATunaSweeperFoodWarehouseActor`). 기존 에셋 참조 호환을 위해 폴더 경로는 유지한다.
- 외형: 같은 폴더의 `SM_FoodWarehouse_Placeholder`, `M_FoodWarehouse`. 메시를 바꿔도 상호작용 로직은 유지한다.
- `bRequireQuest`가 켜져 있으면 양수 `RequiredQuestId`에 해당하는 퀘스트를 수락한 동안만 식량을 획득할 수 있다. 폐기된 마지막 데모 퀘스트에는 대체 ID를 부여하지 않아 기본값은 0이며 상호작용은 비활성이다.
- `FoodItemId`/`FoodQuantity`로 지급할 아이템을 설정한다. 한 레이드에서는 중복 획득을 차단하며 새 레이드에서 다시 사용할 수 있다.
- 자동 검사: `TunaSweeper.FoodWarehouse.AssetsAndInteraction`.
