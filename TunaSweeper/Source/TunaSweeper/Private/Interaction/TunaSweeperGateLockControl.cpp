#include "Interaction/TunaSweeperGateLockControl.h"

#include "Components/StaticMeshComponent.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperChainLinkGate.h"
#include "Subsystem/TunaSweeperSpeechBubbleSubsystem.h"

ATunaSweeperGateLockControl::ATunaSweeperGateLockControl()
{
	VisualMesh->SetRelativeScale3D(FVector::OneVector);
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::GateLockControl, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(
			TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		TEXT("ui.gate.control_open"));
}

bool ATunaSweeperGateLockControl::CanOpenGate() const
{
	return IsValid(LinkedGate) && LinkedGate->GetWorld() == GetWorld() && !LinkedGate->IsUnlocked();
}

bool ATunaSweeperGateLockControl::OpenGate(APawn* InstigatorPawn)
{
	if (!CanOpenGate() || !IsWithinInteractionDistance(InstigatorPawn))
	{
		return false;
	}
	if (LinkedGate->Unlock())
	{
		return true;
	}
	if (auto* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (auto* Speech = Game->GetSubsystem<UTunaSweeperSpeechBubbleSubsystem>())
		{
			Speech->ShowForActor(Game->ResolveLocalizedText(TEXT("ui.gate.open_failed"), FText::GetEmpty()),
				this, FVector(0, 0, 145), ETunaSweeperSpeechBubbleTailDirection::Down, 2.0f, -28.0f);
		}
	}
	return false;
}
