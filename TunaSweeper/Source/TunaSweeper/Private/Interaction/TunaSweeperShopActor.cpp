#include "Interaction/TunaSweeperShopActor.h"

#include "Components/StaticMeshComponent.h"

ATunaSweeperShopActor::ATunaSweeperShopActor()
{
	ConfigureInteractionDefaults(
		ETunaSweeperInteractionType::ShopOpen,
		FText::FromString(TEXT("\uC0C1\uC810")),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
			FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		FName(TEXT("ui.interaction.shop_open")));

	if (VisualMesh)
	{
		VisualMesh->SetRelativeScale3D(FVector(0.62f, 0.44f, 1.25f));
		VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 62.0f));
	}
}

void ATunaSweeperShopActor::SetShopId(int32 InShopId)
{
	ShopId = FMath::Max(1, InShopId);
}

void ATunaSweeperShopActor::ConfigureShopDefaults(int32 InShopId)
{
	SetShopId(InShopId);
}
