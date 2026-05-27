#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"

#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Math/UnrealMathUtility.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

void UTunaSweeperKeyboardInputSubsystem::ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn*)
{
	const int32 ClampedSlotNumber = FMath::Clamp(SlotNumber, 1, 8);
	const int32 UsableQuickSlotIndex = ClampedSlotNumber - 3;

	FString Message = FString::Printf(TEXT("[QuickSlot] Slot %d is empty."), ClampedSlotNumber);
	if (ClampedSlotNumber >= 3)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			const TArray<FTunaSweeperInventorySlot>& UsableQuickSlots = TunaGameInstance->GetUsableQuickSlots();
			FTunaSweeperItemInstance ItemInstance;
			if (UsableQuickSlots.IsValidIndex(UsableQuickSlotIndex) &&
				TunaGameInstance->TryGetItemInstance(UsableQuickSlots[UsableQuickSlotIndex].ItemUid, ItemInstance))
			{
				FText ItemName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
				if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
				{
					ItemDataSubsystem->TryGetItemNameText(ItemInstance.ItemId, ETunaSweeperItemTextLanguage::Korean, ItemName);
				}
				Message = FString::Printf(
					TEXT("[QuickSlot] Slot %d: %s"),
					ClampedSlotNumber,
					*ItemName.ToString());
			}
		}
	}

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			Message);
	}
}
