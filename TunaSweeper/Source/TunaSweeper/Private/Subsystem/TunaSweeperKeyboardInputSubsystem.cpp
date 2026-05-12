#include "Subsystem/TunaSweeperKeyboardInputSubsystem.h"

#include "Engine/Engine.h"
#include "Math/UnrealMathUtility.h"

void UTunaSweeperKeyboardInputSubsystem::ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn*)
{
	const int32 ClampedSlotNumber = FMath::Clamp(SlotNumber, 1, 8);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1,
			2.0f,
			FColor::Yellow,
			FString::Printf(
				TEXT("[QuickSlot] Slot %d is not implemented yet. It will be sent to quick slots."),
				ClampedSlotNumber));
	}
}
