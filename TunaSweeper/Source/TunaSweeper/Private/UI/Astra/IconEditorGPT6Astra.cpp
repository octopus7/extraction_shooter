// Authored by gpt-6-astra; reasoning effort: medium.
#include "UI/Astra/IconEditorGPT6Astra.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/SpinBox.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "IconEditor/IconStorageSubsystem.h"
#include "InputCoreTypes.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UI/Astra/IconCanvasGPT6Astra.h"
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
    if (PaletteText)
        PaletteText->SetText(Text(TEXT("ui.icon_editor.palette")));
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
    SetColor(FColor(0, 0, 0));
}
void UIconEditorGPT6Astra::Pick01()
{
    SetColor(FColor(255, 255, 255));
}
void UIconEditorGPT6Astra::Pick02()
{
    SetColor(FColor(120, 132, 145));
}
void UIconEditorGPT6Astra::Pick03()
{
    SetColor(FColor(244, 67, 54));
}
void UIconEditorGPT6Astra::Pick04()
{
    SetColor(FColor(255, 153, 45));
}
void UIconEditorGPT6Astra::Pick05()
{
    SetColor(FColor(255, 221, 72));
}
void UIconEditorGPT6Astra::Pick06()
{
    SetColor(FColor(78, 197, 104));
}
void UIconEditorGPT6Astra::Pick07()
{
    SetColor(FColor(41, 208, 200));
}
void UIconEditorGPT6Astra::Pick08()
{
    SetColor(FColor(65, 140, 245));
}
void UIconEditorGPT6Astra::Pick09()
{
    SetColor(FColor(135, 93, 230));
}
void UIconEditorGPT6Astra::Pick10()
{
    SetColor(FColor(237, 105, 181));
}
void UIconEditorGPT6Astra::Pick11()
{
    SetColor(FColor(141, 93, 64));
}
