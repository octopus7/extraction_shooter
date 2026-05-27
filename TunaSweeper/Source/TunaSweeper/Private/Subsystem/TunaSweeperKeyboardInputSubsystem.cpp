#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"

#include "Engine/Engine.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Math/UnrealMathUtility.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"

void UTunaSweeperKeyboardInputSubsystem::ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn* InstigatorPawn)
{
	const int32 ClampedSlotNumber = FMath::Clamp(SlotNumber, 1, 8);
	const int32 UsableQuickSlotIndex = ClampedSlotNumber - 3;

	FString Message = FString::Printf(TEXT("[QuickSlot] Slot %d is empty."), ClampedSlotNumber);
	if (ClampedSlotNumber >= 3)
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GetGameInstance()))
		{
			FTunaSweeperItemSlotReference QuickSlotReference;
			QuickSlotReference.Source = ETunaSweeperItemSlotSource::UsableQuickSlot;
			QuickSlotReference.SlotIndex = UsableQuickSlotIndex;
			FTunaSweeperItemInstance ItemInstance;
			if (TunaGameInstance->TryGetSlotItemInstance(QuickSlotReference, ItemInstance))
			{
				FText ItemName = FText::FromString(FString::Printf(TEXT("Item %d"), ItemInstance.ItemId));
				if (UTunaSweeperItemDataSubsystem* ItemDataSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperItemDataSubsystem>())
				{
					ItemDataSubsystem->TryGetItemNameText(ItemInstance.ItemId, TunaGameInstance->GetCurrentTextLanguage(), ItemName);
				}
				if (TunaGameInstance->TryUseItemInSlot(QuickSlotReference, InstigatorPawn))
				{
					Message = FString::Printf(
						TEXT("[QuickSlot] Slot %d used: %s"),
						ClampedSlotNumber,
						*ItemName.ToString());
				}
				else
				{
					Message = FString::Printf(
						TEXT("[QuickSlot] Slot %d cannot use: %s"),
						ClampedSlotNumber,
						*ItemName.ToString());
				}
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
