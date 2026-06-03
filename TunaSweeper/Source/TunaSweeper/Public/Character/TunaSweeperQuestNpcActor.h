#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperInteractableActor.h"
#include "TunaSweeperQuestNpcActor.generated.h"

class UTunaSweeperInteractionMarkerWidget;
class UWidgetComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestNpcActor : public ATunaSweeperInteractableActor
{
	GENERATED_BODY()

public:
	ATunaSweeperQuestNpcActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName GetQuestId() const { return QuestId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName GetProviderId() const { return ProviderId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName ResolveQuestId() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FText GetNpcDisplayName() const { return NpcDisplayName; }

	void ConfigureQuestNpcDefaults(
		FName InQuestId,
		const FText& InNpcDisplayName,
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
		FName InProviderId = NAME_None,
		const FText& InInteractionDisplayName = FText::GetEmpty(),
		FName InInteractionDisplayNameStringKey = NAME_None);

protected:
	void RefreshQuestNoticeVisibility();
	bool ShouldShowQuestNotice() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName ProviderId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText NpcDisplayName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> QuestNoticeWidgetComponent;
};
