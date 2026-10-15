#pragma once

#include "CoreMinimal.h"
#include "Interaction/TunaSweeperShopActor.h"
#include "TunaSweeperQuestVendingActor.generated.h"

class UTunaSweeperQuestMarkerComponent;

UENUM(BlueprintType)
enum class ETunaSweeperQuestVendingRole : uint8
{
	FieldOffer,
	BaseResident
};

// Separate field/base instances derive their presence from the same saved quest progress.
UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperQuestVendingActor : public ATunaSweeperShopActor
{
	GENERATED_BODY()

public:
	ATunaSweeperQuestVendingActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Vending", meta = (ClampMin = "1"))
	int32 FirstMeetingQuestId = 23;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Vending", meta = (ClampMin = "1"))
	int32 RelocationQuestId = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Vending")
	FName QuestProviderId = TEXT("provider.vending_machine");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Vending")
	ETunaSweeperQuestVendingRole PresenceRole = ETunaSweeperQuestVendingRole::FieldOffer;

	UFUNCTION(BlueprintPure, Category = "Quest|Vending")
	int32 ResolveQuestId() const;

	UFUNCTION(BlueprintPure, Category = "Quest|Vending")
	FName GetQuestProviderId() const { return QuestProviderId; }

	UFUNCTION(BlueprintPure, Category = "Quest|Vending")
	bool IsShopAvailable() const;

	UFUNCTION(BlueprintPure, Category = "Quest|Vending")
	UTunaSweeperInteractableComponent* GetQuestInteractableComponent() const { return QuestInteractableComponent; }

	UFUNCTION(BlueprintPure, Category = "Quest|Vending")
	UStaticMeshComponent* GetVendingMeshComponent() const { return VisualMesh; }

	UFUNCTION(BlueprintCallable, Category = "Quest|Vending")
	void RefreshQuestPresence();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UTunaSweeperInteractableComponent> QuestInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest Marker")
	TObjectPtr<UTunaSweeperQuestMarkerComponent> QuestMarkerComponent;

private:
	bool IsQuestPresent() const;
};
