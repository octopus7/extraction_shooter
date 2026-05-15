#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperQuestNpcActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestNpcActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperQuestNpcActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName GetQuestId() const { return QuestId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FText GetNpcDisplayName() const { return NpcDisplayName; }

	void ConfigureQuestNpcDefaults(
		FName InQuestId,
		const FText& InNpcDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText NpcDisplayName;
};
