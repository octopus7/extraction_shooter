#pragma once
// Authored by gpt-6-astra; reasoning effort: medium.
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "IconCanvasGPT6Astra.generated.h"
DECLARE_DELEGATE_ThreeParams(FIconPixelInput, int32, int32, bool);
UCLASS()
class TUNASWEEPER_API UIconCanvasGPT6Astra : public UUserWidget
{
    GENERATED_BODY()
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FIconEditorAutomationAccess;
#endif
  public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon") bool bInteractive = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Icon") bool bShowGrid = true;
    bool bEyedropper = false;
    FIconPixelInput OnPixelInput;
    void SetPixels(const TArray<uint8> &InPixels);

  protected:
    virtual int32 NativePaint(const FPaintArgs &, const FGeometry &, const FSlateRect &, FSlateWindowElementList &,
                              int32, const FWidgetStyle &, bool) const override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry &, const FPointerEvent &) override;
    virtual FReply NativeOnMouseMove(const FGeometry &, const FPointerEvent &) override;
    virtual FReply NativeOnMouseButtonUp(const FGeometry &, const FPointerEvent &) override;
    virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent &) override;

  private:
    TArray<uint8> Pixels;
    FIntPoint LastPixel = FIntPoint(-1, -1);
    bool bDrawing = false;
    bool bSamplingStroke = false;
    bool GetPixel(const FGeometry &, const FPointerEvent &, FIntPoint &) const;
    void StrokeTo(FIntPoint);
};
