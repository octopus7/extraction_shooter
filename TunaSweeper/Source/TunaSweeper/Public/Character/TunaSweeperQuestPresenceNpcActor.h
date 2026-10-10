#pragma once

#include "CoreMinimal.h"
#include "Character/TunaSweeperFacilityNpcActor.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "TunaSweeperQuestPresenceNpcActor.generated.h"

UENUM(BlueprintType)
enum class ETunaSweeperQuestNpcPresenceRole : uint8
{
	FieldOffer,
	BaseResident
};

// Place separate instances at each location. Presence is derived from quest progress.
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestPresenceNpcActor : public ATunaSweeperFacilityNpcActor
{
	GENERATED_BODY()
public:
	ATunaSweeperQuestPresenceNpcActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Presence", meta = (ClampMin = "1"))
	int32 PresenceQuestId = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Presence")
	ETunaSweeperQuestNpcPresenceRole PresenceRole = ETunaSweeperQuestNpcPresenceRole::FieldOffer;

	UFUNCTION(BlueprintCallable, Category = "Quest|Presence")
	void RefreshQuestPresence();

	static bool EvaluatePresence(ETunaSweeperQuestNpcPresenceRole Role, bool bKnownQuest,
		bool bCanAccept, ETunaSweeperQuestState State);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
};
