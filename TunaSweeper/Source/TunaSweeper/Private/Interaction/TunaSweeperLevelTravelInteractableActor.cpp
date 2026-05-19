#include "Interaction/TunaSweeperLevelTravelInteractableActor.h"

#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMesh.h"
#include "Subsystem/TunaSweeperLevelTransitionSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

ATunaSweeperLevelTravelInteractableActor::ATunaSweeperLevelTravelInteractableActor()
{
	TargetLevelName = NAME_None;
	LevelTravelVisualScale = FVector(0.75f, 0.75f, 0.75f);
	LevelTravelVisualRelativeLocation = FVector::ZeroVector;
	FadeToBlackDuration = 0.2f;
	FadeFromBlackDuration = 0.2f;
	TransitionMessage = FText::GetEmpty();
	TransitionWidgetClass = TSoftClassPtr<UTunaSweeperLevelTransitionWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_LevelTransitionVideo.WBP_LevelTransitionVideo_C")));

	if (InteractableComponent)
	{
		InteractableComponent->SetInteractionTypeAndDisplayName(
			ETunaSweeperInteractionType::LevelTravel,
			FText::FromString(TEXT("Travel")));
	}
}

void ATunaSweeperLevelTravelInteractableActor::ConfigureLevelTravelDefaults(
	FName InTargetLevelName,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	TSoftObjectPtr<UMediaSource> InTransitionMediaSource,
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> InTransitionWidgetClass,
	const FText& InTransitionMessage)
{
	Modify();
	TargetLevelName = InTargetLevelName;
	TransitionMediaSource = InTransitionMediaSource;
	TransitionMessage = InTransitionMessage;
	FadeToBlackDuration = 0.2f;
	FadeFromBlackDuration = 0.2f;
	if (!InTransitionWidgetClass.IsNull())
	{
		TransitionWidgetClass = InTransitionWidgetClass;
	}
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::LevelTravel, InInteractionDisplayName, InMarkerWidgetClass);
}

void ATunaSweeperLevelTravelInteractableActor::ConfigureLevelTravelVisualDefaults(
	TSoftObjectPtr<UStaticMesh> InVisualMesh,
	FVector InVisualMeshScale,
	FVector InVisualMeshRelativeLocation)
{
	Modify();
	VisualMeshOverride = InVisualMesh;
	LevelTravelVisualScale = InVisualMeshScale;
	LevelTravelVisualRelativeLocation = InVisualMeshRelativeLocation;
	RefreshLevelTravelVisual();
}

void ATunaSweeperLevelTravelInteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshLevelTravelVisual();
}

void ATunaSweeperLevelTravelInteractableActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshLevelTravelVisual();
}

void ATunaSweeperLevelTravelInteractableActor::RefreshLevelTravelVisual()
{
	if (!VisualMesh)
	{
		return;
	}

	UStaticMesh* MeshToUse = VisualMeshOverride.IsNull()
		? LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"))
		: VisualMeshOverride.LoadSynchronous();

	if (MeshToUse)
	{
		VisualMesh->SetStaticMesh(MeshToUse);
	}

	VisualMesh->SetRelativeScale3D(LevelTravelVisualScale);
	VisualMesh->SetRelativeLocation(LevelTravelVisualRelativeLocation);
}

bool ATunaSweeperLevelTravelInteractableActor::TravelToTargetLevel(APawn* InstigatorPawn)
{
	if (TargetLevelName.IsNone())
	{
		return false;
	}

	UObject* WorldContextObject = InstigatorPawn ? Cast<UObject>(InstigatorPawn) : Cast<UObject>(this);
	const FName SourceLevelName = GetWorld() ? FName(*GetWorld()->GetMapName()) : NAME_None;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UTunaSweeperGameInstance* TunaGameInstance = Cast<UTunaSweeperGameInstance>(GameInstance))
		{
			TunaGameInstance->HandleLevelTravelPersistence(SourceLevelName, TargetLevelName);
		}

		if (UTunaSweeperQuestSubsystem* QuestSubsystem = GameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->NotifyLevelTravelRequested(SourceLevelName, TargetLevelName);
		}
	}

	if (!TransitionMediaSource.IsNull() && !TransitionWidgetClass.IsNull())
	{
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (UTunaSweeperLevelTransitionSubsystem* TransitionSubsystem = GameInstance->GetSubsystem<UTunaSweeperLevelTransitionSubsystem>())
			{
				if (TransitionSubsystem->StartTransition(
					WorldContextObject,
					TargetLevelName,
					TransitionMediaSource,
					TransitionWidgetClass,
					FadeToBlackDuration,
					FadeFromBlackDuration,
					TransitionMessage))
				{
					return true;
				}
			}
		}
	}

	UGameplayStatics::OpenLevel(WorldContextObject, TargetLevelName);
	return true;
}
