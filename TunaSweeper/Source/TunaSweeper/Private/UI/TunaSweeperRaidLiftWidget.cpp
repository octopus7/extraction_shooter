#include "UI/TunaSweeperRaidLiftWidget.h"
#include "Interaction/TunaSweeperRaidLiftActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

TSharedRef<SWidget> UTunaSweeperRaidLiftWidget::RebuildWidget()
{
    if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
    if (!WidgetTree->RootWidget)
    {
        auto* Backdrop = WidgetTree->ConstructWidget<UBorder>();
        Backdrop->SetBrushColor(FLinearColor(0.015f, 0.025f, 0.03f, 0.97f));
        Backdrop->SetHorizontalAlignment(HAlign_Center);
        Backdrop->SetVerticalAlignment(VAlign_Center);
        WidgetTree->RootWidget = Backdrop;
        auto* Column = WidgetTree->ConstructWidget<UVerticalBox>();
        Backdrop->SetContent(Column);
        const auto Text = [this](const TCHAR* Key, int32 Size)
        {
            auto* Label = WidgetTree->ConstructWidget<UTextBlock>();
            auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
            Label->SetText(Game ? Game->ResolveLocalizedText(FName(Key), FText::GetEmpty()) : FText::GetEmpty());
            Label->SetJustification(ETextJustify::Center);
            TunaSweeperUIFont::ApplyFont(Label, Size);
            return Label;
        };
        Column->AddChildToVerticalBox(Text(TEXT("ui.lift.title"), 30))->SetPadding(FMargin(12, 12, 12, 28));
        auto* Row = WidgetTree->ConstructWidget<UHorizontalBox>();
        Column->AddChildToVerticalBox(Row);
        const TCHAR* Keys[] = {TEXT("ui.lift.forest"), TEXT("ui.lift.village_east"), TEXT("ui.lift.village_west"), TEXT("ui.lift.plains")};
        for (int32 Index = 0; Index < 4; ++Index)
        {
            auto* Button = WidgetTree->ConstructWidget<UButton>();
            TunaSweeperUIStyle::ApplyButton(Button, TunaSweeperUIStyle::EButtonRole::Secondary);
            Row->AddChildToHorizontalBox(Button)->SetPadding(FMargin(8));
            auto* Card = WidgetTree->ConstructWidget<UVerticalBox>();
            Button->SetContent(Card);
            auto* Frame = WidgetTree->ConstructWidget<USizeBox>();
            Frame->SetWidthOverride(230);
            Frame->SetHeightOverride(150);
            auto* Picture = WidgetTree->ConstructWidget<UImage>();
            Picture->SetBrushFromTexture(Lift.IsValid() ? Lift->GetDestinationImage(Index) : nullptr);
            Frame->SetContent(Picture);
            Card->AddChildToVerticalBox(Frame);
            Card->AddChildToVerticalBox(Text(Keys[Index], 22))->SetPadding(FMargin(8, 18));
            switch (Index)
            {
            case 0: Button->OnClicked.AddDynamic(this, &ThisClass::Forest); break;
            case 1: Button->OnClicked.AddDynamic(this, &ThisClass::VillageEast); break;
            case 2: Button->OnClicked.AddDynamic(this, &ThisClass::VillageWest); break;
            case 3: Button->OnClicked.AddDynamic(this, &ThisClass::Plains); break;
            }
        }
        auto* Close = WidgetTree->ConstructWidget<UButton>();
        Close->SetContent(Text(TEXT("ui.lift.cancel"), 20));
        TunaSweeperUIStyle::ApplyButton(Close, TunaSweeperUIStyle::EButtonRole::Secondary);
        auto* CloseSlot = Column->AddChildToVerticalBox(Close);
        CloseSlot->SetPadding(FMargin(8, 24));
        CloseSlot->SetHorizontalAlignment(HAlign_Center);
        Close->OnClicked.AddDynamic(this, &ThisClass::Cancel);
    }
    return Super::RebuildWidget();
}
void UTunaSweeperRaidLiftWidget::NativeConstruct()
{
    Super::NativeConstruct();
    SetIsFocusable(true);
    TunaSweeperUIFont::ApplyFontToWidgetTree(this);
}
void UTunaSweeperRaidLiftWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    if (Lift.IsValid() && !Lift->IsPawnOnPlatform(GetOwningPlayerPawn())) Cancel();
}
FReply UTunaSweeperRaidLiftWidget::NativeOnKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
    if (Event.GetKey() == EKeys::Escape || Event.GetKey() == EKeys::Gamepad_FaceButton_Right)
    {
        Cancel();
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(Geometry, Event);
}
void UTunaSweeperRaidLiftWidget::Forest() { if (Lift.IsValid()) Lift->SelectArrival(ETunaSweeperRaidArrivalGroup::Forest); }
void UTunaSweeperRaidLiftWidget::VillageEast() { if (Lift.IsValid()) Lift->SelectArrival(ETunaSweeperRaidArrivalGroup::VillageEast); }
void UTunaSweeperRaidLiftWidget::VillageWest() { if (Lift.IsValid()) Lift->SelectArrival(ETunaSweeperRaidArrivalGroup::VillageWest); }
void UTunaSweeperRaidLiftWidget::Plains() { if (Lift.IsValid()) Lift->SelectArrival(ETunaSweeperRaidArrivalGroup::Plains); }
void UTunaSweeperRaidLiftWidget::Cancel() { if (Lift.IsValid()) Lift->ClosePicker(); }
