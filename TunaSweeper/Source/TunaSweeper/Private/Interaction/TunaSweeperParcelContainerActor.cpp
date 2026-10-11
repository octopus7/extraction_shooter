#include "Interaction/TunaSweeperParcelContainerActor.h"

#include "Game/TunaSweeperGameInstance.h"
#include "Templates/UnrealTemplate.h"

ATunaSweeperParcelContainerActor::ATunaSweeperParcelContainerActor()
{
	ContainerDefinitionId = 7004;
	ContentsId = 8014;
}

bool ATunaSweeperParcelContainerActor::CollectContentsOnOpen(UTunaSweeperGameInstance* Game)
{
	if (!Game || bCollecting || Game->GetActiveLootContainerOwner() != this)
	{
		return false;
	}
	TGuardValue<bool> Guard(bCollecting, true);
	const int32 SlotCount = Game->GetActiveLootContainerSlots().Num();
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		FTunaSweeperItemSlotReference Source;
		Source.Source = ETunaSweeperItemSlotSource::LootContainer;
		Source.SlotIndex = Index;
		while (Game->GetActiveLootContainerOwner() == this &&
			Game->GetActiveLootContainerSlots().IsValidIndex(Index) &&
			!Game->GetActiveLootContainerSlots()[Index].IsEmpty())
		{
			FTunaSweeperItemSlotReference Target;
			if (!Game->TryFindFirstStackTargetSlot(Source, ETunaSweeperItemSlotSource::Inventory, Target))
			{
				Target.Source = ETunaSweeperItemSlotSource::Inventory;
				Target.SlotIndex = Game->GetInventorySlots().IndexOfByPredicate(
					[](const FTunaSweeperInventorySlot& Slot) { return Slot.IsEmpty(); });
			}
			// Never swap a player's item into the parcel when the inventory is full.
			if (!Target.IsValid() || !Game->MoveItemBetweenSlots(Source, Target)) break;
		}
	}
	if (Game->GetActiveLootContainerOwner() != this) return false;
	return !Game->GetActiveLootContainerSlots().ContainsByPredicate(
		[](const FTunaSweeperInventorySlot& Slot) { return !Slot.IsEmpty(); });
}
