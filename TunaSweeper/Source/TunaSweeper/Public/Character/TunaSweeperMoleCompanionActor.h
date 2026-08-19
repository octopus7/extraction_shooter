#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperMoleCompanionActor.generated.h"

class UMaterialInterface;
class UCapsuleComponent;
class USceneComponent;
class UStaticMesh;
class UStaticMeshComponent;
class UTunaSweeperInteractableComponent;
class UTunaSweeperInteractionMarkerWidget;
class UWidgetComponent;

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
		TSoftObjectPtr<UStaticMesh> InBodyMesh,
		TSoftObjectPtr<UStaticMesh> InHeadMesh,
		TSoftObjectPtr<UStaticMesh> InSnoutMesh,
		TSoftObjectPtr<UMaterialInterface> InVisualMaterial);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Mole Companion")
	FName GetCompanionId() const { return CompanionId; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Quest")
	FName ResolveQuestId() const;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void RefreshCompanionVisuals();
	void RefreshQuestNoticeVisibility();
	bool ShouldShowQuestNotice() const;
	void UpdatePlayerLookAt(float DeltaSeconds);
	bool TryGetPlayerLookYaw(float& OutYaw, float& OutDistance2D) const;
	float ResolveOrganicYawOffset(float DeltaSeconds);

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> HeadMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> SnoutMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Collision", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UCapsuleComponent> BodyCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperInteractableComponent> DialogueInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperInteractableComponent> QuestInteractableComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mole Companion|Interaction", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UWidgetComponent> QuestNoticeWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion", meta = (AllowPrivateAccess = "true"))
	FName CompanionId = TEXT("BunkerMole");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> BodyMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> HeadMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UStaticMesh> SnoutMeshOverride;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	TSoftObjectPtr<UMaterialInterface> VisualMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector BodyRelativeLocation = FVector(0.0f, 0.0f, 72.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector BodyScale = FVector(0.72f, 0.92f, 0.58f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector HeadRelativeLocation = FVector(54.0f, 0.0f, 86.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector HeadScale = FVector(0.48f, 0.52f, 0.43f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector SnoutRelativeLocation = FVector(91.0f, 0.0f, 78.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Visual", meta = (AllowPrivateAccess = "true"))
	FVector SnoutScale = FVector(0.17f, 0.21f, 0.15f);

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
	float LookAtStartDistance = 650.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtStopDistance = 850.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtInterpolationSpeed = 2.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mole Companion|Look At", meta = (AllowPrivateAccess = "true", ClampMin = "0.0"))
	float LookAtReturnInterpolationSpeed = 1.8f;

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
	float LookAtYawOffset = 0.0f;
	float LookAtYawOffsetTarget = 0.0f;
	float LookAtYawOffsetRefreshElapsed = 0.0f;
	bool bIsLookingAtPlayer = false;
	bool bLookAtReactionPending = false;
};
