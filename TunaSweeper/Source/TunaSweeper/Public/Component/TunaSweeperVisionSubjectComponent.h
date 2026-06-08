#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "TunaSweeperVisionSubjectComponent.generated.h"

class UPrimitiveComponent;

struct FTunaSweeperVisionSubjectPrimitiveRenderState
{
	TWeakObjectPtr<UPrimitiveComponent> Component;
	bool bRenderInMainPass = true;
	bool bRenderInDepthPass = true;
};

UCLASS(ClassGroup = (TunaSweeper), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperVisionSubjectComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UTunaSweeperVisionSubjectComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vision")
	FVector GetVisionTestLocation() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vision")
	float GetVisibilityPaddingCm() const { return FMath::Max(0.0f, VisibilityPaddingCm); }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Vision")
	bool IsVisionVisibilityEnabled() const { return bEnableVisionVisibility; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetVisionVisibilityEnabled(bool bEnabled);

	void ApplyVisionVisible(bool bVisible);
	void ResetVisionVisibility();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	bool bEnableVisionVisibility = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision")
	FVector VisionTestLocationOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Vision", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float VisibilityPaddingCm = 0.0f;

private:
	void HideSubjectPrimitives();
	void CachePrimitiveRenderState(UPrimitiveComponent* PrimitiveComponent);
	FTunaSweeperVisionSubjectPrimitiveRenderState* FindCachedRenderState(UPrimitiveComponent* PrimitiveComponent);

	TArray<FTunaSweeperVisionSubjectPrimitiveRenderState> CachedPrimitiveRenderStates;
	bool bVisionHidden = false;
};
