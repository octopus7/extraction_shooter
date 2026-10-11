# 전력 시설 BP

- BP: `/Game/Interaction/BP_PowerFacility`
- 부모: `ATunaSweeperPowerFacilityActor`
- 기본 메시: 기존 `SM_FacilityRooms_ElectricalCabinet`
- 고장 효과: `/Game/Interaction/PowerFacility/P_PowerFacility_Sparks`

고장 상태에서 스파크가 반복 재생된다. 수리에 성공하면 남아 있는 파티클까지 즉시 끄며, 시설 메시와 충돌은 유지한다. 퀘스트 수락 여부와 무관하게 고장 효과는 보인다. 수리 완료는 기존 월드 진행 저장 체계로 복원한다.

## 나중에 배치할 때 설정할 값

| 항목 | 설정 |
| --- | --- |
| `ProgressObjectId` | 배치별로 고유하고 저장 후 바뀌지 않는 ID. 기본값은 비워 두며, 미설정 시 수리를 막는다. |
| `bRequireActiveQuest` | 기본 true. 수락한 퀘스트의 미완료 목표에서만 수리 가능. |
| `RepairQuestId` | 해당 퀘스트의 양수 정수 ID. |
| `RepairObjectiveId` | 수리 목표 ID. |
| `RepairObjectiveEventId` | 해당 목표에서 사용하는 상호작용 이벤트 ID. 정상 플레이어 상호작용 성공 시 공용 상호작용 서브시스템이 보고한다. |
| `bRequiresItem` / `RequiredItemId` / `RequiredItemQuantity` | 기본적으로 재료 필요. 실제 아이템 ID와 수량을 지정한다. 재료 없는 수리는 `bRequiresItem`을 끈다. |
| `bConsumeRequiredItem` | true는 인벤토리 재료를 소모하고, false는 휴대 도구 보유만 확인한다. |
| `InteractionStringKey` | 기본 `ui.interaction.repair`. 기존 지역화 문자열 키를 사용한다. |

컴포넌트 트리의 `FacilityMesh`에서 메시를, `FaultSparks`에서 파티클 템플릿과 위치·회전·크기를, `Interactable`에서 상호작용 표시 위치를 조정한다. 기본 스파크는 반복형 Cascade 파티클 시스템이다. `bPreviewRepairedInEditor`로 에디터에서 수리 전후를 비교할 수 있다. 런타임에서는 저장 상태가 우선한다.

`Repair()`를 BP에서 직접 호출하면 시설 수리·저장만 수행한다. 퀘스트 이벤트 보고는 정상 플레이어 상호작용 경로가 담당한다. 기본 BP에는 퀘스트·아이템·배치 ID를 임의로 연결하지 않았다. 레이드 맵 배치와 퀘스트 데이터 수정은 수행하지 않았다.

## 기본 전력 시설 프리셋

`/Game/Interaction/BP_PowerFacility_Primary`는 공용 BP를 상속한 설정 프리셋이다. 활성 퀘스트의 수리 목표와 상호작용 이벤트를 연결했고 재료는 요구하지 않는다. 정상 플레이어 상호작용으로 수리해야 목표 이벤트까지 보고된다. 직접 `Repair()` 호출은 기존대로 시설 상태만 처리한다.

영속 ID는 `power.facility.primary`이며 같은 시설을 대표하는 단일 배치용이다. 독립 설비에 복제해 같은 ID를 공유하지 않는다. 원본 공용 BP의 빈 설정은 유지되며, 프리셋도 맵에는 배치하지 않았다. 이 프리셋은 조명 액터를 켜거나 다단계 전력을 분배하지 않는다.
