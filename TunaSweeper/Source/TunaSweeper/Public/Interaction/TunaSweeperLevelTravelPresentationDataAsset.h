#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaSweeperLevelTravelPresentationDataAsset.generated.h"

class UMediaSource;
class UTunaSweeperLevelTransitionWidget;

/** The two gameplay destinations available in the current demo. */
UENUM(BlueprintType)
enum class ETunaSweeperLevelTravelDestination : uint8
{
	Bunker UMETA(DisplayName = "Bunker"),
	Raid UMETA(DisplayName = "Raid")
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

/** Central demo transition-video configuration, owned by the GameInstance. */
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
