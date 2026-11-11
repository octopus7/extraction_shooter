// Authored by gpt-6-astra; reasoning effort: medium.
#include "UI/Astra/IconEditorGPT6Astra.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "IconEditor/IconStorageSubsystem.h"
#include "InputCoreTypes.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/Astra/IconCanvasGPT6Astra.h"

namespace
{
// RGB8 presets, in visible swatch order. Names are authoring comments only.
const FColor PresetColors[6][12] = {
    {FColor(0, 0, 0), FColor(255, 255, 255), FColor(120, 132, 145), FColor(244, 67, 54), FColor(255, 153, 45),
     FColor(255, 221, 72), FColor(78, 197, 104), FColor(41, 208, 200), FColor(65, 140, 245), FColor(135, 93, 230),
     FColor(237, 105, 181), FColor(141, 93, 64)}, // Original
    {FColor(255, 245, 230), FColor(255, 214, 220), FColor(255, 191, 163), FColor(255, 229, 163), FColor(232, 239, 176),
     FColor(197, 237, 204), FColor(188, 235, 230), FColor(184, 221, 246), FColor(199, 204, 245), FColor(222, 200, 243),
     FColor(242, 200, 228), FColor(216, 206, 196)}, // Pastel
    {FColor(0, 0, 0), FColor(23, 23, 23), FColor(46, 46, 46), FColor(69, 69, 69), FColor(93, 93, 93),
     FColor(116, 116, 116), FColor(139, 139, 139), FColor(162, 162, 162), FColor(186, 186, 186), FColor(209, 209, 209),
     FColor(232, 232, 232), FColor(255, 255, 255)}, // Grayscale
    {FColor(59, 40, 38), FColor(101, 67, 56), FColor(141, 92, 66), FColor(183, 121, 75), FColor(217, 155, 99),
     FColor(242, 192, 120), FColor(245, 222, 163), FColor(217, 183, 136), FColor(163, 151, 101), FColor(114, 125, 75),
     FColor(198, 85, 68), FColor(237, 136, 100)}, // Warm earth and sunset
    {FColor(21, 50, 61), FColor(29, 83, 99), FColor(40, 122, 141), FColor(67, 165, 181), FColor(131, 207, 209),
     FColor(188, 230, 223), FColor(25, 63, 53), FColor(40, 96, 73), FColor(67, 135, 97), FColor(120, 173, 130),
     FColor(179, 201, 157), FColor(220, 231, 196)}, // Cool ocean and forest
    {FColor(80, 27, 100), FColor(135, 38, 163), FColor(198, 56, 176), FColor(238, 67, 138), FColor(219, 40, 74),
     FColor(242, 100, 50), FColor(249, 187, 48), FColor(180, 220, 50), FColor(32, 179, 106), FColor(25, 184, 187),
     FColor(36, 102, 213), FColor(87, 59, 209)}, // Vivid jewels
};
} // namespace
UIconEditorGPT6Astra::UIconEditorGPT6Astra(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}
FText UIconEditorGPT6Astra::Text(FName Key) const
{
    auto *Game = GetGameInstance<UTunaSweeperGameInstance>();
    FText Value;
    if (Game)
        if (auto *Strings = Game->GetSubsystem<UTunaSweeperTextSubsystem>())
            Strings->TryGetTextByKey(Key, Game->GetCurrentTextLanguage(), Value);
    return Value;
}
bool UIconEditorGPT6Astra::InitializeEditor()
{
    if (!PixelCanvas || !PreviewCanvas || !GetGameInstance())
        return false;
    SetIsFocusable(true);
    auto *Storage = GetGameInstance()->GetSubsystem<UIconStorageSubsystem>();
    if (!Storage)
        return false;
    Draft.Init(0, UIconStorageSubsystem::ByteCount);
    const EIconLoadResult Result = Storage->LoadRgbIcon(Draft);
    if (Draft.Num() != UIconStorageSubsystem::ByteCount)
        Draft.Init(0, UIconStorageSubsystem::ByteCount);
    if (TitleText)
        TitleText->SetText(Text(TEXT("ui.icon_editor.title")));
    if (SubtitleText)
        SubtitleText->SetText(Text(TEXT("ui.icon_editor.subtitle")));
    if (DescriptionText)
        DescriptionText->SetText(Text(TEXT("ui.icon_editor.description")));
    if (PreviewText)
        PreviewText->SetText(Text(TEXT("ui.icon_editor.preview")));
    if (ColorText)
        ColorText->SetText(Text(TEXT("ui.icon_editor.color")));
    if (RedText)
        RedText->SetText(Text(TEXT("ui.icon_editor.red")));
    if (GreenText)
        GreenText->SetText(Text(TEXT("ui.icon_editor.green")));
    if (BlueText)
        BlueText->SetText(Text(TEXT("ui.icon_editor.blue")));
    if (BrushText)
        BrushText->SetText(Text(TEXT("ui.icon_editor.brush")));
    if (EyedropperText)
        EyedropperText->SetText(Text(TEXT("ui.icon_editor.eyedropper")));
    if (FillText)
        FillText->SetText(Text(TEXT("ui.icon_editor.fill")));
    if (HelpText)
        HelpText->SetText(Text(TEXT("ui.icon_editor.help")));
    if (StatusText)
        StatusText->SetText(Text(TEXT("ui.icon_editor.ready")));
    if (CancelText)
        CancelText->SetText(Text(TEXT("ui.icon_editor.cancel")));
    if (ConfirmText)
        ConfirmText->SetText(Text(TEXT("ui.icon_editor.confirm")));
    if (RedDecrease)
    {
        RedDecrease->OnClicked.AddUniqueDynamic(this, &ThisClass::RedDecreaseClicked);
        RedDecrease->SetToolTipText(Text(TEXT("ui.icon_editor.decrease")));
    }
    if (RedIncrease)
    {
        RedIncrease->OnClicked.AddUniqueDynamic(this, &ThisClass::RedIncreaseClicked);
        RedIncrease->SetToolTipText(Text(TEXT("ui.icon_editor.increase")));
    }
    if (GreenDecrease)
    {
        GreenDecrease->OnClicked.AddUniqueDynamic(this, &ThisClass::GreenDecreaseClicked);
        GreenDecrease->SetToolTipText(Text(TEXT("ui.icon_editor.decrease")));
    }
    if (GreenIncrease)
    {
        GreenIncrease->OnClicked.AddUniqueDynamic(this, &ThisClass::GreenIncreaseClicked);
        GreenIncrease->SetToolTipText(Text(TEXT("ui.icon_editor.increase")));
    }
    if (BlueDecrease)
    {
        BlueDecrease->OnClicked.AddUniqueDynamic(this, &ThisClass::BlueDecreaseClicked);
        BlueDecrease->SetToolTipText(Text(TEXT("ui.icon_editor.decrease")));
    }
    if (BlueIncrease)
    {
        BlueIncrease->OnClicked.AddUniqueDynamic(this, &ThisClass::BlueIncreaseClicked);
        BlueIncrease->SetToolTipText(Text(TEXT("ui.icon_editor.increase")));
    }
    if (BrushButton)
        BrushButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Brush);
    if (EyedropperButton)
        EyedropperButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Eyedropper);
    if (FillButton)
        FillButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Fill);
    if (CancelButton)
        CancelButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Cancel);
    if (ConfirmButton)
        ConfirmButton->OnClicked.AddUniqueDynamic(this, &ThisClass::Confirm);
    if (Palette00)
        Palette00->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick00);
    if (Palette01)
        Palette01->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick01);
    if (Palette02)
        Palette02->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick02);
    if (Palette03)
        Palette03->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick03);
    if (Palette04)
        Palette04->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick04);
    if (Palette05)
        Palette05->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick05);
    if (Palette06)
        Palette06->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick06);
    if (Palette07)
        Palette07->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick07);
    if (Palette08)
        Palette08->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick08);
    if (Palette09)
        Palette09->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick09);
    if (Palette10)
        Palette10->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick10);
    if (Palette11)
        Palette11->OnClicked.AddUniqueDynamic(this, &ThisClass::Pick11);
    if (RedInput)
        RedInput->OnValueChanged.AddUniqueDynamic(this, &ThisClass::RedChanged);
    if (GreenInput)
        GreenInput->OnValueChanged.AddUniqueDynamic(this, &ThisClass::GreenChanged);
    if (BlueInput)
        BlueInput->OnValueChanged.AddUniqueDynamic(this, &ThisClass::BlueChanged);
    PixelCanvas->OnPixelInput.BindUObject(this, &ThisClass::PixelInput);
    PreviewCanvas->bInteractive = false;
    PreviewCanvas->bShowGrid = false;
    PreviewCanvas->SetVisibility(ESlateVisibility::HitTestInvisible);
    Preset1->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset1Clicked);
    Preset2->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset2Clicked);
    Preset3->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset3Clicked);
    Preset4->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset4Clicked);
    Preset5->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset5Clicked);
    Preset6->OnClicked.AddUniqueDynamic(this, &ThisClass::Preset6Clicked);
    SelectPreset(0);
    SetColor(Color);
    Brush();
    RefreshPixels();
    SetStatus(Result == EIconLoadResult::Invalid      ? TEXT("ui.icon_editor.invalid")
              : Result == EIconLoadResult::ReadFailed ? TEXT("ui.icon_editor.read_failed")
                                                      : TEXT("ui.icon_editor.ready"));
    SetKeyboardFocus();
    return true;
}
void UIconEditorGPT6Astra::NativeDestruct()
{
    if (PixelCanvas)
        PixelCanvas->OnPixelInput.Unbind();
    Draft.Empty();
    Super::NativeDestruct();
}
FReply UIconEditorGPT6Astra::NativeOnPreviewKeyDown(const FGeometry &G, const FKeyEvent &E)
{
    if (E.GetKey() == EKeys::Escape)
    {
        Cancel();
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(G, E);
}
void UIconEditorGPT6Astra::SetStatus(FName Key)
{
    if (StatusText)
        StatusText->SetText(Text(Key));
}
void UIconEditorGPT6Astra::RefreshPixels()
{
    PixelCanvas->SetPixels(Draft);
    PreviewCanvas->SetPixels(Draft);
}
void UIconEditorGPT6Astra::PixelInput(int32 X, int32 Y, bool bSample)
{
    const int32 I = (Y * UIconStorageSubsystem::Width + X) * 3;
    if (!Draft.IsValidIndex(I + 2))
        return;
    if (bSample)
    {
        SetColor(FColor(Draft[I], Draft[I + 1], Draft[I + 2]));
        return;
    }
    Draft[I] = Color.R;
    Draft[I + 1] = Color.G;
    Draft[I + 2] = Color.B;
    RefreshPixels();
}
void UIconEditorGPT6Astra::SetColor(FColor V)
{
    Color = V;
    bUpdatingColor = true;
    if (RedInput)
        RedInput->SetValue(V.R);
    if (GreenInput)
        GreenInput->SetValue(V.G);
    if (BlueInput)
        BlueInput->SetValue(V.B);
    if (SelectedColor)
        SelectedColor->SetBrushColor(FLinearColor(V));
    bUpdatingColor = false;
}
void UIconEditorGPT6Astra::RedChanged(float V)
{
    if (!bUpdatingColor)
        SetColor(FColor(FMath::Clamp(FMath::RoundToInt(V), 0, 255), Color.G, Color.B));
}
void UIconEditorGPT6Astra::GreenChanged(float V)
{
    if (!bUpdatingColor)
        SetColor(FColor(Color.R, FMath::Clamp(FMath::RoundToInt(V), 0, 255), Color.B));
}
void UIconEditorGPT6Astra::BlueChanged(float V)
{
    if (!bUpdatingColor)
        SetColor(FColor(Color.R, Color.G, FMath::Clamp(FMath::RoundToInt(V), 0, 255)));
}
void UIconEditorGPT6Astra::Confirm()
{
    auto *S = GetGameInstance() ? GetGameInstance()->GetSubsystem<UIconStorageSubsystem>() : nullptr;
    if (S && S->TrySaveIcon(Draft))
        OnCloseRequested.Broadcast();
    else
        SetStatus(TEXT("ui.icon_editor.save_failed"));
}
void UIconEditorGPT6Astra::Cancel()
{
    OnCloseRequested.Broadcast();
}
void UIconEditorGPT6Astra::Fill()
{
    for (int32 I = 0; I + 2 < Draft.Num(); I += 3)
    {
        Draft[I] = Color.R;
        Draft[I + 1] = Color.G;
        Draft[I + 2] = Color.B;
    }
    RefreshPixels();
}
void UIconEditorGPT6Astra::Brush()
{
    PixelCanvas->bEyedropper = false;
    if (BrushButton)
        BrushButton->SetBackgroundColor(FLinearColor(0.08f, 0.65f, 0.75f));
    if (EyedropperButton)
        EyedropperButton->SetBackgroundColor(FLinearColor(0.2f, 0.25f, 0.3f));
}
void UIconEditorGPT6Astra::Eyedropper()
{
    PixelCanvas->bEyedropper = true;
    if (BrushButton)
        BrushButton->SetBackgroundColor(FLinearColor(0.2f, 0.25f, 0.3f));
    if (EyedropperButton)
        EyedropperButton->SetBackgroundColor(FLinearColor(0.08f, 0.65f, 0.75f));
}
void UIconEditorGPT6Astra::Pick00()
{
    PickSwatch(0);
}
void UIconEditorGPT6Astra::Pick01()
{
    PickSwatch(1);
}
void UIconEditorGPT6Astra::Pick02()
{
    PickSwatch(2);
}
void UIconEditorGPT6Astra::Pick03()
{
    PickSwatch(3);
}
void UIconEditorGPT6Astra::Pick04()
{
    PickSwatch(4);
}
void UIconEditorGPT6Astra::Pick05()
{
    PickSwatch(5);
}
void UIconEditorGPT6Astra::Pick06()
{
    PickSwatch(6);
}
void UIconEditorGPT6Astra::Pick07()
{
    PickSwatch(7);
}
void UIconEditorGPT6Astra::Pick08()
{
    PickSwatch(8);
}
void UIconEditorGPT6Astra::Pick09()
{
    PickSwatch(9);
}
void UIconEditorGPT6Astra::Pick10()
{
    PickSwatch(10);
}
void UIconEditorGPT6Astra::Pick11()
{
    PickSwatch(11);
}

void UIconEditorGPT6Astra::RedDecreaseClicked()
{
    RedChanged(FMath::Clamp(static_cast<int32>(Color.R) - 1, 0, 255));
}
void UIconEditorGPT6Astra::RedIncreaseClicked()
{
    RedChanged(FMath::Clamp(static_cast<int32>(Color.R) + 1, 0, 255));
}
void UIconEditorGPT6Astra::GreenDecreaseClicked()
{
    GreenChanged(FMath::Clamp(static_cast<int32>(Color.G) - 1, 0, 255));
}
void UIconEditorGPT6Astra::GreenIncreaseClicked()
{
    GreenChanged(FMath::Clamp(static_cast<int32>(Color.G) + 1, 0, 255));
}
void UIconEditorGPT6Astra::BlueDecreaseClicked()
{
    BlueChanged(FMath::Clamp(static_cast<int32>(Color.B) - 1, 0, 255));
}
void UIconEditorGPT6Astra::BlueIncreaseClicked()
{
    BlueChanged(FMath::Clamp(static_cast<int32>(Color.B) + 1, 0, 255));
}

void UIconEditorGPT6Astra::PickSwatch(int32 Index)
{
    if (Index >= 0 && Index < 12)
        SetColor(PresetColors[ActivePreset][Index]);
}
void UIconEditorGPT6Astra::SelectPreset(int32 Index)
{
    if (Index < 0 || Index >= 6)
        return;
    ActivePreset = Index;
    UButton *Tabs[] = {Preset1, Preset2, Preset3, Preset4, Preset5, Preset6};
    const FLinearColor Bright(0.88f, 0.93f, 0.96f);
    // The authored disabled brush retains the containing panel color for both tab states.
    const FLinearColor Dark = Preset1->GetStyle().Disabled.TintColor.GetSpecifiedColor();
    for (int32 I = 0; I < 6; ++I)
    {
        const bool bSelected = I == ActivePreset;
        FButtonStyle Style = Tabs[I]->GetStyle();
        Style.SetNormal(FSlateRoundedBoxBrush(bSelected ? Bright : Dark, 6.0f));
        Style.SetHovered(FSlateRoundedBoxBrush(bSelected ? Bright : FLinearColor(0.18f, 0.23f, 0.28f), 6.0f));
        Style.SetPressed(FSlateRoundedBoxBrush(bSelected ? Bright : FLinearColor(0.25f, 0.30f, 0.35f), 6.0f));
        Tabs[I]->SetStyle(Style);
        if (auto *Label = Cast<UTextBlock>(Tabs[I]->GetContent()))
        {
            Label->SetText(Text(FName(*FString::Printf(TEXT("ui.icon_editor.preset%d"), I + 1))));
            Label->SetColorAndOpacity(bSelected ? Dark : Bright);
        }
    }
    UButton *Swatches[] = {Palette00, Palette01, Palette02, Palette03, Palette04, Palette05,
                           Palette06, Palette07, Palette08, Palette09, Palette10, Palette11};
    for (int32 I = 0; I < 12; ++I)
    {
        FButtonStyle Style = Swatches[I]->GetStyle();
        const FLinearColor Tint(PresetColors[ActivePreset][I]);
        Style.Normal.TintColor = Tint;
        Style.Hovered.TintColor = Tint;
        Style.Pressed.TintColor = Tint;
        Swatches[I]->SetStyle(Style);
    }
}
void UIconEditorGPT6Astra::Preset1Clicked()
{
    SelectPreset(0);
}
void UIconEditorGPT6Astra::Preset2Clicked()
{
    SelectPreset(1);
}
void UIconEditorGPT6Astra::Preset3Clicked()
{
    SelectPreset(2);
}
void UIconEditorGPT6Astra::Preset4Clicked()
{
    SelectPreset(3);
}
void UIconEditorGPT6Astra::Preset5Clicked()
{
    SelectPreset(4);
}
void UIconEditorGPT6Astra::Preset6Clicked()
{
    SelectPreset(5);
}
