#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMoleCompanionActor.generated.h"

class UMaterialInterface;
class UCapsuleComponent;
class USceneComponent;
class USkeletalMeshComponent;
class USkeletalMesh;
class UBlendSpace;
class UTunaSweeperMoleAnimInstance;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;
class UTunaSweeperQuestMarkerComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperMoleCompanionActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperMoleCompanionActor();

	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Mole Companion")
	void ConfigureCompanionDefaults(
		FName InCompanionId,
		TSoftObjectPtr<USkeletalMesh> InSkeletalMesh,
		TSoftObjectPtr<UMaterialInterface> InVisualMaterial);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Mole Companion")
	FName GetCompanionId() const { return CompanionId; }

	/** Zero means breathing idle; negative/positive values select left/right footwork. */
	static float ResolveTurnAnimationAmount(float PreviousYaw, float CurrentYaw, float DeltaSeconds);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Idle Variations")
	bool bEnableIdleVariations = true;

	/** Rest after a gesture finishes, in seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Idle Variations", meta = (ClampMin = "0.1", Units = "s"))
	float IdleVariationMinDelay = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Idle Variations", meta = (ClampMin = "0.1", Units = "s"))
	float IdleVariationMaxDelay = 15.0f;

	FVector2D GetIdleVariationDelayRange() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName ResolveQuestId() const;
	FName GetQuestProviderId() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RefreshCompanionVisuals();
	void RefreshQuestNoticeVisibility();
	bool ShouldShowQuestNotice() const;
	void UpdatePlayerLookAt(float DeltaSeconds);
	void UpdateCompanionAnimation(float PreviousYaw, float DeltaSeconds);
	bool TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const;
	float ResolveOrganicYawOffset(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> BodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperInteractableComponent> DialogueInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperInteractableComponent> QuestInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Quest Marker", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperQuestMarkerComponent> QuestMarkerComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion", meta = (AllowPrivateAccess = "true"))
	FName CompanionId = TEXT("BunkerMole");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<USkeletalMesh> SkeletalMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBlendSpace> IdleTurnBlendSpace;

	UPROPERTY(EditDefaultsOnly, Category = "Mole Companion|Visual")
	TSubclassOf<UTunaSweeperMoleAnimInstance> CompanionAnimClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Collision", meta = (AllowPrivateAccess = "true"))
	FVector BodyCollisionRelativeLocation = FVector(0.0f, 0.0f, 72.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Collision", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float BodyCollisionRadius = 66.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Collision", meta = (AllowPrivateAccess = "true", ClampMin = "1.0"))
	float BodyCollisionHalfHeight = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Quest", meta = (AllowPrivateAccess = "true"))
	FName QuestFallbackId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Quest", meta = (AllowPrivateAccess = "true"))
	FName QuestProviderId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InteractionMarkerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true"))
	bool bLookAtNearbyPlayer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtStartDistance = 400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtStopDistance = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtInterpolationSpeed = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtReturnInterpolationSpeed = 1.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtMaxTurnSpeed = 90.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtReturnDelay = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtMinReactionDelay = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtMaxReactionDelay = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtTargetRefreshInterval = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtYawOffsetDegrees = 3.0f;

	FRotator IdleActorRotation = FRotator::ZeroRotator;
	float PendingLookAtYaw = 0.0f;
	float LookAtReactionElapsed = 0.0f;
	float LookAtReactionDelay = 0.0f;
	float LookAtRefreshElapsed = 0.0f;
	float LookAtReturnDelayRemaining = 0.0f;
	float LookAtYawOffset = 0.0f;
	float LookAtYawOffsetTarget = 0.0f;
	float LookAtYawOffsetRefreshElapsed = 0.0f;
	bool bIsLookingAtPlayer = false;
	bool bLookAtReactionPending = false;
};
