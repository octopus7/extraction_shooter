#include "Interaction/TunaSweeperWardrobeActor.h"

ATunaSweeperWardrobeActor::ATunaSweeperWardrobeActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::WardrobeOpen,
		FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.wardrobe_open")));
}
