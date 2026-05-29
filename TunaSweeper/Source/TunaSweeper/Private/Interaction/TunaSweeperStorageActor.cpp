#include "Interaction/TunaSweeperStorageActor.h"

ATunaSweeperStorageActor::ATunaSweeperStorageActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::StorageOpen,
		FText::FromString(TEXT("\uCC3D\uACE0 \uC5F4\uAE30")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.storage_open")));
}
