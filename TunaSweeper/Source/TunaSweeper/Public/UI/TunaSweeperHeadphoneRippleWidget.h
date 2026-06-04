#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "TunaSweeperHeadphoneRippleWidget.generated.h"

class AActor;
class UCanvasPanel;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHeadphoneRippleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void SetListenerActor(AActor* InListenerActor);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Noise")
	void AddNoiseRipple(AActor* InListenerActor, const FVector& InDirectionFromListener, float InStrength);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "16.0", UIMin = "16.0"))
	float RingRadiusAt1080p = 118.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "0.5", UIMin = "0.5"))
	float RingThicknessPixels = 13.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "0.25", UIMin = "0.25"))
	float ParticlePixelSize = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "1.0", ClampMax = "90.0", UIMin = "1.0", UIMax = "90.0"))
	float SectorHalfAngleDegrees = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "0.05", UIMin = "0.05"))
	float RippleLifetimeSeconds = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "0", UIMin = "0"))
	int32 BaseRingParticleCount = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "0", UIMin = "0"))
	int32 MinSectorParticleCount = 18;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space", meta = (ClampMin = "1", UIMin = "1"))
	int32 MaxSectorParticleCount = 104;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|Noise|Screen Space")
	FLinearColor RippleColor = FLinearColor(0.78f, 0.68f, 0.42f, 1.0f);

private:
	struct FScreenRipple
	{
		FVector DirectionFromListener = FVector::ForwardVector;
		float Strength = 1.0f;
		float ElapsedSeconds = 0.0f;
		int32 Seed = 1;
	};

	void EnsureNativeRoot();
	bool TryGetListenerScreenCenter(const FGeometry& AllottedGeometry, FVector2D& OutCenter) const;
	FVector2D ResolveScreenDirection(const FGeometry& AllottedGeometry, const FScreenRipple& Ripple, const FVector2D& Center) const;
	float GetLocalPixelSize() const;
	void DrawParticle(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FVector2D& Center,
		float Size,
		const FLinearColor& Color) const;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> NativeRootCanvas;

	TWeakObjectPtr<AActor> ListenerActor;
	TArray<FScreenRipple> ActiveRipples;
	int32 NextSeed = 1;
};
