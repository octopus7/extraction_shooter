#include "UI/TunaSweeperDemoFarewellWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Styling/CoreStyle.h"
#include "UI/TunaSweeperUiText.h"
#include "UI/TunaSweeperUIFont.h"

UTunaSweeperDemoFarewellWidget::UTunaSweeperDemoFarewellWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    SetIsFocusable(true);
}

TSharedRef<SWidget> UTunaSweeperDemoFarewellWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"));
    if (!WidgetTree->RootWidget)
    {
        auto* Canvas = WidgetTree->ConstructWidget<UCanvasPanel>();
        WidgetTree->RootWidget = Canvas;
        auto Place = [&](UWidget* Widget, FAnchors Anchors)
        {
            auto* Slot = Canvas->AddChildToCanvas(Widget);
            Slot->SetAnchors(Anchors); Slot->SetOffsets(FMargin(0));
        };
        auto* Background = WidgetTree->ConstructWidget<UBorder>();
        Background->SetBrushColor(FLinearColor::White);
        Background->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Background, FAnchors(0,0,1,1));
        auto* Scale = WidgetTree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("IllustrationFrame"));
        Scale->SetStretch(EStretch::ScaleToFit);
        Scale->SetVisibility(ESlateVisibility::HitTestInvisible);
        Place(Scale, FAnchors(0.02f,0.03f,0.98f,0.03f + 2.f/3.f));
        auto* Image = WidgetTree->ConstructWidget<UImage>();
        Image->SetBrushFromTexture(Illustration, true);
        Scale->AddChild(Image);
        auto Text = [&](FText Value, int32 Size, FAnchors Anchors)
        {
            auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
            Label->SetText(Value); Label->SetJustification(ETextJustify::Center);
            Label->SetColorAndOpacity(FLinearColor(.12f,.10f,.09f));
            Label->SetFont(TunaSweeperUIFont::MakeFont(Label, Size));
            Label->SetAutoWrapText(true);
            Label->SetVisibility(ESlateVisibility::HitTestInvisible);
            Place(Label, Anchors);
        };
        const UTunaSweeperGameInstance* GameInstance = GetGameInstance<UTunaSweeperGameInstance>();
        Text(TunaSweeperUiText::ResolveUiText(GameInstance, TEXT("ui.demo_ending.farewell_title"), TEXT("")), 44, FAnchors(.05f,.75f,.95f,.85f));
        Text(TunaSweeperUiText::ResolveUiText(GameInstance, TEXT("ui.demo_ending.return_to_title"), TEXT("")), 20, FAnchors(.05f,.89f,.95f,.98f));
    }
    return Super::RebuildWidget();
}
void UTunaSweeperDemoFarewellWidget::NativeConstruct()
{
    Super::NativeConstruct();
    AcceptInputAfter = FPlatformTime::Seconds() + .5;
}
void UTunaSweeperDemoFarewellWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    if (!bLeaving && FPlatformTime::Seconds() >= AcceptInputAfter)
    {
        EnsureInputFocus();
    }
}
void UTunaSweeperDemoFarewellWidget::EnsureInputFocus()
{
    if (APlayerController* OwningPlayer = GetOwningPlayer(); OwningPlayer && !HasUserFocus(OwningPlayer))
    {
        SetUserFocus(OwningPlayer);
        SetKeyboardFocus();
    }
}
FReply UTunaSweeperDemoFarewellWidget::Continue()
{
    if (!bLeaving && FPlatformTime::Seconds() >= AcceptInputAfter)
    {
        bLeaving = true;
        OnContinue.ExecuteIfBound();
    }
    return FReply::Handled();
}
FReply UTunaSweeperDemoFarewellWidget::NativeOnKeyDown(const FGeometry&, const FKeyEvent& Event)
{
    return Event.IsRepeat() ? FReply::Handled() : Continue();
}
FReply UTunaSweeperDemoFarewellWidget::NativeOnMouseButtonDown(const FGeometry&, const FPointerEvent&) { return Continue(); }
FReply UTunaSweeperDemoFarewellWidget::NativeOnMouseWheel(const FGeometry&, const FPointerEvent&) { return Continue(); }
