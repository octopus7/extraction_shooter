#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperLevelTravelPresentationDataAsset.generated.h"

class UMediaSource;
class UTunaSweeperLevelTransitionWidget;

/** Shared gameplay destinations selected by placed travel actors. */
UENUM(BlueprintType)
enum class ETunaSweeperLevelTravelDestination : uint8
{
	Bunker UMETA(DisplayName = "Bunker"),
	Raid UMETA(DisplayName = "Raid"),
	Basement UMETA(DisplayName = "Basement"),
	Forest UMETA(DisplayName = "Forest"),
	Village UMETA(DisplayName = "Village"),
	Plains UMETA(DisplayName = "Plains")
};

/** Shared transition presentation selected by a logical level-travel destination. */
USTRUCT(BlueprintType)
struct TUNASWEEPER_API FTunaSweeperLevelTravelPresentationDefinition
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel")
	ETunaSweeperLevelTravelDestination Destination = ETunaSweeperLevelTravelDestination::Raid;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video")
	TSoftObjectPtr<UMediaSource> TransitionMediaSource;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video")
	TSoftClassPtr<UTunaSweeperLevelTransitionWidget> TransitionWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video")
	FText TransitionMessage;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video")
	FName TransitionMessageStringKey = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeToBlackDuration = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel|Transition Video", meta = (ClampMin = "0.01", UIMin = "0.01"))
	float FadeFromBlackDuration = 0.2f;
};

/** Central transition-video configuration, owned by the GameInstance. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UTunaSweeperLevelTravelPresentationDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	bool TryGetPresentation(
		ETunaSweeperLevelTravelDestination Destination,
		FTunaSweeperLevelTravelPresentationDefinition& OutDefinition) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TunaSweeper|Level Travel")
	TArray<FTunaSweeperLevelTravelPresentationDefinition> Presentations;
};
