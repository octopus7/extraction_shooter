#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaThoughtIndicatorActor.generated.h"

class USceneComponent;
class UTexture2D;
class UWidgetComponent;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaThoughtIndicatorActor : public AActor
{
	GENERATED_BODY()

public:
	ATunaThoughtIndicatorActor();

	virtual void Tick(float DeltaSeconds) override;
	virtual void OnConstruction(const FTransform& Transform) override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	float CalculateBobOffsetPixels(float TimeSeconds) const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Tuna Thought Indicator")
	void RefreshIndicator();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	UWidgetComponent* GetIndicatorWidgetComponent() const { return IndicatorWidgetComponent; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	UTexture2D* GetIndicatorTexture() const { return IndicatorTexture; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	FVector2D GetIndicatorImageSizePixels() const { return IndicatorImageSizePixels; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	float GetBobAmplitudePixels() const { return BobAmplitudePixels; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Tuna Thought Indicator")
	float GetBobCyclesPerSecond() const { return BobCyclesPerSecond; }

protected:
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual bool ShouldTickIfViewportsOnly() const override;
#endif

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> IndicatorWidgetComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Appearance")
	TObjectPtr<UTexture2D> IndicatorTexture;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Appearance", meta = (ClampMin = "16.0", UIMin = "16.0"))
	FVector2D IndicatorImageSizePixels = FVector2D(176.0f, 176.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Placement", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WorldHeightOffset = 135.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Animation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BobAmplitudePixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Animation", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float BobCyclesPerSecond = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator|Animation", meta = (ClampMin = "-360.0", ClampMax = "360.0"))
	float InitialPhaseDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Tuna Thought Indicator")
	bool bIndicatorVisible = true;

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "TunaSweeper|Tuna Thought Indicator|Animation")
	bool bPreviewAnimationInEditor = true;
#endif

private:
	FVector2D CalculateCanvasSizePixels() const;
	void ApplyComponentSettings();
	void UpdateWidgetPresentation();

	float AnimationTimeSeconds = 0.0f;
};
