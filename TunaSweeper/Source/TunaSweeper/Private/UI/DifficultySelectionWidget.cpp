#include "UI/DifficultySelectionWidget.h"

#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Settings/TunaSweeperLanguage.h"
#include "UI/TunaSweeperUIFont.h"

void UDifficultySelectionWidget::SetLabel(const TCHAR* ImageName, FName Key, const FString& Culture,
    bool bWhite, float MaxWidth, float MaxHeight)
{
    UImage* Image = Cast<UImage>(GetWidgetFromName(ImageName));
    const auto* Texture = LabelTextures.Find(FName(Culture + TEXT(".") + Key.ToString()));
    if (!Image || !Texture || !*Texture || !LabelMaterial) return;
    auto& Instance = LabelInstances.FindOrAdd(ImageName);
    if (!Instance) Instance = UMaterialInstanceDynamic::Create(LabelMaterial, this);
    Instance->SetTextureParameterValue(TEXT("Mask"), *Texture);
    Instance->SetVectorParameterValue(TEXT("Ink"), bWhite ? FLinearColor::White : FLinearColor(0.035f, 0.065f, 0.08f, 1.f));
    Image->SetBrushFromMaterial(Instance);
    if (auto* ImageSlot = Cast<UCanvasPanelSlot>(Image->Slot))
    {
        const FVector2D Size((*Texture)->GetSizeX(), (*Texture)->GetSizeY());
        const float Scale = FMath::Min(MaxWidth / Size.X, MaxHeight / Size.Y);
        ImageSlot->SetSize(Size * Scale);
    }
}

void UDifficultySelectionWidget::Present(int32 Stage, bool bBusy, bool bAdjustment)
{
    const auto* GI = Cast<UTunaSweeperGameInstance>(GetGameInstance());
    const FString Culture = GI ? TunaSweeperLanguage::ToLanguageCode(GI->GetCurrentTextLanguage()) : TEXT("ko");
    const TCHAR* Names[] = {TEXT("Story"), TEXT("Normal"), TEXT("Hard")};
    const TCHAR* Legacy[] = {TEXT("Farming"), TEXT("Normal"), TEXT("Hard")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        const bool Selected = Stage == Index + 1;
        SetLabel(*FString::Printf(TEXT("%sLabel"), Names[Index]),
            FName(*FString::Printf(TEXT("difficulty.stage.%d.title"), Index + 1)), Culture, false, 380.f, 80.f);
        if (auto* Card = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("Card%d"), Index))))
            if (auto* MID = Card->GetDynamicMaterial()) MID->SetScalarParameterValue(TEXT("Selected"), Selected ? 1.f : 0.f);
        if (auto* Check = GetWidgetFromName(*FString::Printf(TEXT("Check%d"), Index)))
            Check->SetVisibility(Selected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
        if (auto* Button = Cast<UButton>(GetWidgetFromName(*FString::Printf(TEXT("Difficulty%sButton"), Legacy[Index]))))
            Button->SetIsEnabled(!bBusy);
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(*FString::Printf(TEXT("Description%d"), Index))))
        {
            if (GI) Text->SetText(GI->ResolveLocalizedText(
                FName(*FString::Printf(TEXT("difficulty.stage.%d.description"), Index + 1)), FText::GetEmpty()));
            Text->SetAutoWrapText(false);
            Text->SetWrapTextAt(440.f);
            TunaSweeperUIFont::ApplyFont(Text, Culture == TEXT("ru") ? 22.f : 24.f);
        }
    }
    SetLabel(TEXT("TitleLabel"), TEXT("ui.difficulty.select_title"), Culture, false, 1320.f, 125.f);
    SetLabel(TEXT("BackLabel"), TEXT("ui.common.back"), Culture, false, 290.f, 62.f);
    SetLabel(TEXT("StartLabel"), TEXT("ui.difficulty.start"), Culture, true, 470.f, 62.f);
    // Existing-save adjustment must still say Apply/Cancel, not Start/Back.
    const TCHAR* Images[] = {TEXT("TitleLabel"), TEXT("BackLabel"), TEXT("StartLabel")};
    const TCHAR* Texts[] = {TEXT("AdjustmentTitle"), TEXT("AdjustmentBack"), TEXT("AdjustmentStart")};
    const FName Keys[] = {TEXT("ui.difficulty.adjust_title"), TEXT("ui.common.cancel"), TEXT("ui.common.apply")};
    for (int32 Index = 0; Index < 3; ++Index)
    {
        if (auto* Image = GetWidgetFromName(Images[Index])) Image->SetVisibility(bAdjustment ? ESlateVisibility::Hidden : ESlateVisibility::HitTestInvisible);
        if (auto* Text = Cast<UTextBlock>(GetWidgetFromName(Texts[Index])))
        {
            Text->SetVisibility(bAdjustment ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
            if (GI) Text->SetText(GI->ResolveLocalizedText(Keys[Index], FText::GetEmpty()));
        }
    }
    if (auto* Start = Cast<UButton>(GetWidgetFromName(TEXT("DifficultyStartButton")))) Start->SetIsEnabled(Stage >= 1 && Stage <= 3 && !bBusy);
    if (auto* Back = Cast<UButton>(GetWidgetFromName(TEXT("DifficultyBackButton")))) Back->SetIsEnabled(!bBusy);
}

void UDifficultySelectionWidget::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
    Super::NativeTick(Geometry, DeltaSeconds);
    DriftTime += DeltaSeconds;
    for (int32 Index = 0; Index < 3; ++Index)
        if (auto* Card = Cast<UImage>(GetWidgetFromName(*FString::Printf(TEXT("Card%d"), Index))))
            if (auto* MID = Card->GetDynamicMaterial()) MID->SetScalarParameterValue(TEXT("MotionTime"), DriftTime);
    for (int32 Index = 0; Index < 10; ++Index)
        if (UWidget* Fish = GetWidgetFromName(*FString::Printf(TEXT("DriftFish%d"), Index)))
        {
            const float Phase = Index * 1.79f;
            Fish->SetRenderTranslation(FVector2D(32.f * FMath::Sin(DriftTime * .09f + Phase), 20.f * FMath::Sin(DriftTime * .17f + Phase)));
            Fish->SetRenderTransformAngle(4.f * FMath::Sin(DriftTime * .12f + Phase));
        }
}
