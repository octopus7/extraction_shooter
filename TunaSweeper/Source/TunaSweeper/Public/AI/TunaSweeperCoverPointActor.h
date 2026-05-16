#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaSweeperCoverPointActor.generated.h"

class AActor;
class UArrowComponent;
class UBillboardComponent;
class UBoxComponent;
class USceneComponent;
class USphereComponent;

UENUM(BlueprintType)
enum class ETunaSweeperCoverPeekSide : uint8
{
	Left UMETA(DisplayName = "Left"),
	Right UMETA(DisplayName = "Right")
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperCoverPointActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaSweeperCoverPointActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	FVector GetCoverLocation() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	FVector GetCoverFacingDirection() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	FVector GetPeekLocation(ETunaSweeperCoverPeekSide PeekSide) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	bool IsPeekSideEnabled(ETunaSweeperCoverPeekSide PeekSide) const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	bool IsReserved() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Cover")
	AActor* GetReservedBy() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Cover")
	bool TryReserve(AActor* ReservingActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Cover")
	void ReleaseReservation(AActor* ReservingActor);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> RootScene;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> CoverAreaComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> ReservationRadiusComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> FacingArrowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> LeftPeekArrowComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> RightPeekArrowComponent;

#if WITH_EDITORONLY_DATA
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBillboardComponent> EditorIconComponent;
#endif

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Visualization", meta = (ClampMin = "1.0", UIMin = "1.0"))
	FVector CoverAreaExtent = FVector(120.0f, 80.0f, 90.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Visualization", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ReservationRadius = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Peeking", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PeekForwardOffset = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Peeking", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PeekSideOffset = 85.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Peeking")
	bool bAllowLeftPeek = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Peeking")
	bool bAllowRightPeek = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Evaluation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MinUsefulTargetDistance = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cover|Evaluation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float MaxUsefulTargetDistance = 1200.0f;

private:
	void UpdateVisualizerComponents();

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ReservedBy;
};
