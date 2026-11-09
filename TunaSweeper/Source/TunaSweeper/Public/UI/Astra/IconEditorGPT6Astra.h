#pragma once
// Authored by gpt-6-astra; reasoning effort: medium.
#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "IconEditorGPT6Astra.generated.h"
class UIconCanvasGPT6Astra;
class UTextBlock;
class UBorder;
class USpinBox;
class UButton;
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIconEditorCloseRequested);
UCLASS()
class TUNASWEEPER_API UIconEditorGPT6Astra : public UUserWidget
{
    GENERATED_BODY()
#if WITH_DEV_AUTOMATION_TESTS
    friend struct FIconEditorAutomationAccess;
#endif
  public:
    UIconEditorGPT6Astra(const FObjectInitializer &ObjectInitializer);
    bool InitializeEditor();
    UPROPERTY(BlueprintAssignable, Category = "Icon") FIconEditorCloseRequested OnCloseRequested;

  protected:
    virtual FReply NativeOnPreviewKeyDown(const FGeometry &, const FKeyEvent &) override;
    virtual void NativeDestruct() override;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UIconCanvasGPT6Astra> PixelCanvas;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UIconCanvasGPT6Astra> PreviewCanvas;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SubtitleText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DescriptionText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PreviewText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ColorText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PaletteText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> RedText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> GreenText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BlueText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> BrushText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> EyedropperText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> FillText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> HelpText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> StatusText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CancelText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ConfirmText;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UBorder> SelectedColor;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> RedInput;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> GreenInput;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<USpinBox> BlueInput;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BrushButton;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> EyedropperButton;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> FillButton;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CancelButton;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ConfirmButton;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette00;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette01;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette02;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette03;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette04;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette05;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette06;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette07;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette08;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette09;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette10;
    UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Palette11;

  private:
    TArray<uint8> Draft;
    FColor Color = FColor::White;
    bool bUpdatingColor = false;
    void PixelInput(int32 X, int32 Y, bool bSample);
    void RefreshPixels();
    void SetColor(FColor Value);
    void SetStatus(FName Key);
    FText Text(FName Key) const;
    UFUNCTION() void Confirm();
    UFUNCTION() void Cancel();
    UFUNCTION() void Fill();
    UFUNCTION() void Brush();
    UFUNCTION() void Eyedropper();
    UFUNCTION() void RedChanged(float Value);
    UFUNCTION() void GreenChanged(float Value);
    UFUNCTION() void BlueChanged(float Value);
    UFUNCTION() void Pick00();
    UFUNCTION() void Pick01();
    UFUNCTION() void Pick02();
    UFUNCTION() void Pick03();
    UFUNCTION() void Pick04();
    UFUNCTION() void Pick05();
    UFUNCTION() void Pick06();
    UFUNCTION() void Pick07();
    UFUNCTION() void Pick08();
    UFUNCTION() void Pick09();
    UFUNCTION() void Pick10();
    UFUNCTION() void Pick11();
};
