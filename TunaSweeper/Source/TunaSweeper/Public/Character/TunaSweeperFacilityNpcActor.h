#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperFacilityNpcActor.generated.h"

class UCapsuleComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperQuestMarkerComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperFacilityNpcActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperFacilityNpcActor();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Facility NPC")
	FName ResolveQuestId() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Facility NPC")
	FName GetNpcId() const { return NpcId; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	void ConfigureFacilityNpcDefaults(
		FName InNpcId,
		FName InQuestProviderId,
		FName InQuestFallbackId,
		FName InQuestInteractionEventId,
		FVector InBodyScale,
		FVector InHeadScale,
		FVector InHeadRelativeLocation);

	void RefreshQuestNoticeVisibility();
	bool ShouldShowQuestNotice() const;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UCapsuleComponent> BodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UTunaSweeperInteractableComponent> QuestInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest Marker")
	TObjectPtr<UTunaSweeperQuestMarkerComponent> QuestMarkerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Facility NPC")
	FName NpcId = TEXT("npc.facility");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestProviderId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestFallbackId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestInteractionEventId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InteractionMarkerWidgetClass;

private:
	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultBodyMesh;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMesh> DefaultHeadMesh;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperSignalBotActor : public ATunaSweeperFacilityNpcActor
{
	GENERATED_BODY()

public:
	ATunaSweeperSignalBotActor();
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperRicePotBotActor : public ATunaSweeperFacilityNpcActor
{
	GENERATED_BODY()

public:
	ATunaSweeperRicePotBotActor();
};
