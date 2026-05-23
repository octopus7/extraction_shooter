#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Rendering/RenderingCommon.h"
#include "Styling/SlateBrush.h"
#include "TunaSweeperVisionMaskWidget.generated.h"

class UTexture2D;

struct FTunaSweeperVisionMaskVertex
{
	FVector2D Position = FVector2D::ZeroVector;
	FColor Color = FColor::Transparent;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperVisionMaskWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskTexture(UTexture2D* InMaskTexture);

	void SetMaskMesh(TArray<FTunaSweeperVisionMaskVertex>&& InVertices, TArray<SlateIndex>&& InIndices);
	void ClearMaskMesh();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Vision")
	void SetMaskVisible(bool bVisible);

protected:
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MaskTexture;

	TArray<FTunaSweeperVisionMaskVertex> MaskVertices;
	TArray<SlateIndex> MaskIndices;
	mutable TArray<FSlateVertex> PaintVertices;
	mutable FSlateBrush MaskBrush;
};
