#include "UI/TunaSweeperWardrobePanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateNoResource.h"
#include "Character/TunaSweeperOutfitCatalog.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

namespace
{
	FText WardrobeText(UTunaSweeperGameInstance* Game, FName Key)
	{
		return Game ? Game->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	}

	UTextBlock* MakeLabel(UWidgetTree* Tree, const TCHAR* Name, float FontSize)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		TunaSweeperUIStyle::ApplyLabel(Label);
		TunaSweeperUIFont::ApplyFont(Label, FontSize);
		return Label;
	}

	USizeBox* MakePortrait(UWidgetTree* Tree, UImage*& OutImage, const TCHAR* Name, float Width, float Height)
	{
		USizeBox* Size = Tree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(Width);
		Size->SetHeightOverride(Height);
		UScaleBox* Fit = Tree->ConstructWidget<UScaleBox>();
		Fit->SetStretch(EStretch::ScaleToFit);
		OutImage = Tree->ConstructWidget<UImage>(UImage::StaticClass(), Name);
		// Reserve the catalog's 2:3 portrait ratio even while editor textures compile.
		OutImage->SetDesiredSizeOverride(FVector2D(1024.0f, 1536.0f));
		OutImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		Fit->SetContent(OutImage);
		Size->SetContent(Fit);
		return Size;
	}
}

void UTunaSweeperOutfitCardWidget::Configure(FName InOutfitId, UTexture2D* Thumbnail,
	const FText& Name, const FText& Status, bool bSelected, bool bUnlocked,
	FTunaSweeperOutfitCardSelected InSelected)
{
	OutfitId = InOutfitId;
	PortraitTexture = Thumbnail;
	OutfitName = Name;
	OutfitStatus = Status;
	bIsSelected = bSelected;
	bIsUnlocked = bUnlocked;
	SelectedDelegate = MoveTemp(InSelected);
	RefreshCard();
}

TSharedRef<SWidget> UTunaSweeperOutfitCardWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	BuildCard();
	return Super::RebuildWidget();
}

void UTunaSweeperOutfitCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	BuildCard();
	RefreshCard();
}

void UTunaSweeperOutfitCardWidget::BuildCard()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	CardButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OutfitCardButton"));
	WidgetTree->RootWidget = CardButton;
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	CardButton->SetContent(Stack);
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(Stack->Slot))
	{
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Fill);
	}
	UImage* Portrait = nullptr;
	USizeBox* PortraitBox = MakePortrait(WidgetTree, Portrait, TEXT("OutfitCardPortrait"), 150.0f, 225.0f);
	PortraitImage = Portrait;
	Stack->AddChildToVerticalBox(PortraitBox)->SetHorizontalAlignment(HAlign_Center);
	NameText = MakeLabel(WidgetTree, TEXT("OutfitCardName"), 18.0f);
	NameText->SetJustification(ETextJustify::Center);
	NameText->SetAutoWrapText(true);
	Stack->AddChildToVerticalBox(NameText)->SetPadding(FMargin(0.0f, 3.0f));
	StatusText = MakeLabel(WidgetTree, TEXT("OutfitCardStatus"), 14.0f);
	StatusText->SetJustification(ETextJustify::Center);
	Stack->AddChildToVerticalBox(StatusText);
	CardButton->OnClicked.AddDynamic(this, &UTunaSweeperOutfitCardWidget::HandleClicked);
}

void UTunaSweeperOutfitCardWidget::RefreshCard()
{
	if (!CardButton) return;
	TunaSweeperUIStyle::ApplyButton(CardButton, TunaSweeperUIStyle::EButtonRole::Secondary, bIsSelected);
	FButtonStyle Style = CardButton->GetStyle();
	Style.SetNormal(FSlateNoResource());
	Style.SetHovered(FSlateNoResource());
	Style.SetPressed(FSlateNoResource());
	Style.SetDisabled(FSlateNoResource());
	Style.SetNormalPadding(FMargin(8.0f));
	Style.SetPressedPadding(FMargin(8.0f));
	CardButton->SetStyle(Style);
	PortraitImage->SetBrushFromTexture(PortraitTexture, true);
	PortraitImage->SetColorAndOpacity(bIsUnlocked ? FLinearColor::White : FLinearColor(0.4f, 0.4f, 0.4f, 0.65f));
	NameText->SetText(OutfitName);
	NameText->SetColorAndOpacity(bIsSelected ? FLinearColor(0.38f, 0.94f, 0.8f) : FLinearColor::White);
	StatusText->SetText(OutfitStatus);
	StatusText->SetColorAndOpacity(bIsUnlocked ? FLinearColor(0.38f, 0.94f, 0.8f) : FLinearColor(0.7f, 0.72f, 0.75f));
	CardButton->SetToolTipText(OutfitName);
}

void UTunaSweeperOutfitCardWidget::HandleClicked()
{
	// Locked outfits remain previewable; the transaction validates unlocks again.
	SelectedDelegate.ExecuteIfBound(OutfitId);
}

TSharedRef<SWidget> UTunaSweeperWardrobePanelWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	BuildPanel();
	return Super::RebuildWidget();
}

void UTunaSweeperWardrobePanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	BuildPanel();
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnOutfitChanged.RemoveAll(this);
		Game->OnOutfitChanged.AddUObject(this, &ThisClass::RefreshWardrobe);
		Game->OnOutfitUnlocksChanged.RemoveAll(this);
		Game->OnOutfitUnlocksChanged.AddUObject(this, &ThisClass::RefreshWardrobe);
		Game->OnLanguageChanged.RemoveAll(this);
		Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshWardrobe);
	}
	RefreshWardrobe();
}

void UTunaSweeperWardrobePanelWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnOutfitChanged.RemoveAll(this);
		Game->OnOutfitUnlocksChanged.RemoveAll(this);
		Game->OnLanguageChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UTunaSweeperWardrobePanelWidget::BuildPanel()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WardrobeScreen"));
	// A viewport-wide scrim keeps the game visible without a framed modal window.
	Root->SetBrush(FSlateColorBrush(FLinearColor(0.015f, 0.029f, 0.04f, 0.88f)));
	Root->SetPadding(FMargin(32.0f));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Stack);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Stack->AddChildToVerticalBox(Header)->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
	UVerticalBox* Heading = WidgetTree->ConstructWidget<UVerticalBox>();
	Header->AddChildToHorizontalBox(Heading)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	TitleText = MakeLabel(WidgetTree, TEXT("WardrobeTitle"), 30.0f);
	TunaSweeperUIFont::ApplyFont(TitleText, 30.0f, ETunaSweeperUIFontWeight::Bold);
	Heading->AddChildToVerticalBox(TitleText);
	SubtitleText = MakeLabel(WidgetTree, TEXT("WardrobeSubtitle"), 16.0f);
	SubtitleText->SetColorAndOpacity(FLinearColor(0.59f, 0.72f, 0.75f));
	Heading->AddChildToVerticalBox(SubtitleText);
	CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WardrobeCloseButton"));
	CloseText = MakeLabel(WidgetTree, TEXT("WardrobeCloseText"), 18.0f);
	CloseButton->SetContent(CloseText);
	TunaSweeperUIStyle::ApplyButton(CloseButton, TunaSweeperUIStyle::EButtonRole::Secondary);
	Header->AddChildToHorizontalBox(CloseButton)->SetVerticalAlignment(VAlign_Center);
	CloseButton->OnClicked.AddDynamic(this, &ThisClass::HandleCloseClicked);

	UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>();
	Stack->AddChildToVerticalBox(Body)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UScrollBox* ListScroll = WidgetTree->ConstructWidget<UScrollBox>();
	ListScroll->SetOrientation(Orient_Vertical);
	ListScroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	ListScroll->SetNavigationDestination(EDescendantScrollDestination::IntoView);
	UHorizontalBoxSlot* ListSlot = Body->AddChildToHorizontalBox(ListScroll);
	FSlateChildSize ListFill(ESlateSizeRule::Fill);
	ListFill.Value = 0.58f;
	ListSlot->SetSize(ListFill);
	ListSlot->SetPadding(FMargin(0.0f, 0.0f, 20.0f, 0.0f));
	OutfitGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(UUniformGridPanel::StaticClass(), TEXT("WardrobeOutfitGrid"));
	OutfitGrid->SetSlotPadding(FMargin(5.0f));
	OutfitGrid->SetMinDesiredSlotWidth(166.0f);
	OutfitGrid->SetMinDesiredSlotHeight(296.0f);
	ListScroll->AddChild(OutfitGrid);

	UVerticalBox* Detail = WidgetTree->ConstructWidget<UVerticalBox>();
	FSlateChildSize PreviewFill(ESlateSizeRule::Fill);
	PreviewFill.Value = 0.42f;
	Body->AddChildToHorizontalBox(Detail)->SetSize(PreviewFill);
	PreviewTitleText = MakeLabel(WidgetTree, TEXT("WardrobePreviewTitle"), 14.0f);
	PreviewTitleText->SetColorAndOpacity(FLinearColor(0.59f, 0.72f, 0.75f));
	Detail->AddChildToVerticalBox(PreviewTitleText);
	UScaleBox* PortraitFit = WidgetTree->ConstructWidget<UScaleBox>();
	PortraitFit->SetStretch(EStretch::ScaleToFit);
	PreviewImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("WardrobePreviewImage"));
	PreviewImage->SetDesiredSizeOverride(FVector2D(1024.0f, 1536.0f));
	PreviewImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	PortraitFit->SetContent(PreviewImage);
	UVerticalBoxSlot* PortraitSlot = Detail->AddChildToVerticalBox(PortraitFit);
	PortraitSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	PortraitSlot->SetHorizontalAlignment(HAlign_Fill);
	PortraitSlot->SetVerticalAlignment(VAlign_Fill);
	SelectedNameText = MakeLabel(WidgetTree, TEXT("WardrobeSelectedName"), 24.0f);
	SelectedNameText->SetJustification(ETextJustify::Center);
	Detail->AddChildToVerticalBox(SelectedNameText)->SetPadding(FMargin(0.0f, 2.0f));
	StatusText = MakeLabel(WidgetTree, TEXT("WardrobeStatus"), 15.0f);
	StatusText->SetJustification(ETextJustify::Center);
	StatusText->SetAutoWrapText(true);
	Detail->AddChildToVerticalBox(StatusText)->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 10.0f));
	EquipButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("WardrobeEquipButton"));
	EquipText = MakeLabel(WidgetTree, TEXT("WardrobeEquipText"), 20.0f);
	EquipText->SetJustification(ETextJustify::Center);
	EquipButton->SetContent(EquipText);
	TunaSweeperUIStyle::ApplyButton(EquipButton);
	Detail->AddChildToVerticalBox(EquipButton);
	EquipButton->OnClicked.AddDynamic(this, &ThisClass::HandleEquipClicked);
}

void UTunaSweeperWardrobePanelWidget::OpenWardrobe()
{
	bApplyFailed = false;
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	PreviewOutfitId = Game ? Game->GetSelectedOutfitId() : NAME_None;
	RefreshWardrobe();
}

void UTunaSweeperWardrobePanelWidget::RefreshWardrobe()
{
	BuildPanel();
	if (!OutfitGrid) return;
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperOutfitCatalog* Catalog = Game ? Game->GetOutfitCatalog() : nullptr;
	const FName EquippedId = Game ? Game->GetSelectedOutfitId() : NAME_None;
	if (Catalog && !Catalog->FindOutfit(PreviewOutfitId)) PreviewOutfitId = EquippedId;
	TitleText->SetText(WardrobeText(Game, TEXT("ui.wardrobe.title")));
	SubtitleText->SetText(WardrobeText(Game, TEXT("ui.wardrobe.subtitle")));
	PreviewTitleText->SetText(WardrobeText(Game, TEXT("ui.wardrobe.preview")));
	CloseText->SetText(WardrobeText(Game, TEXT("ui.wardrobe.close")));
	TArray<FName> CurrentIds;
	if (Catalog)
	{
		for (const FTunaSweeperOutfitDefinition& Outfit : Catalog->Outfits) CurrentIds.Add(Outfit.OutfitId);
	}
	if (CardOutfitIds != CurrentIds)
	{
		OutfitGrid->ClearChildren();
		OutfitCards.Reset();
		CardOutfitIds = CurrentIds;
	}
	int32 Index = 0;
	if (Catalog)
	{
		for (const FTunaSweeperOutfitDefinition& Outfit : Catalog->Outfits)
		{
			UTunaSweeperOutfitCardWidget* Card = OutfitCards.IsValidIndex(Index) ? OutfitCards[Index].Get() : nullptr;
			if (!Card)
			{
				Card = CreateWidget<UTunaSweeperOutfitCardWidget>(GetOwningPlayer());
				if (!Card) continue;
				OutfitCards.Add(Card);
				OutfitGrid->AddChildToUniformGrid(Card, Index / 3, Index % 3);
			}
			const bool bUnlocked = Game->IsOutfitUnlocked(Outfit.OutfitId);
			const FText Status = !bUnlocked ? WardrobeText(Game, TEXT("ui.wardrobe.locked"))
				: Outfit.OutfitId == EquippedId ? WardrobeText(Game, TEXT("ui.wardrobe.equipped")) : FText::GetEmpty();
			Card->Configure(Outfit.OutfitId, Outfit.Thumbnail.LoadSynchronous(), WardrobeText(Game, Outfit.DisplayNameStringKey),
				Status, Outfit.OutfitId == PreviewOutfitId, bUnlocked,
				FTunaSweeperOutfitCardSelected::CreateUObject(this, &ThisClass::SelectOutfit));
			++Index;
		}
	}
	const FTunaSweeperOutfitDefinition* Preview = Catalog ? Catalog->FindOutfit(PreviewOutfitId) : nullptr;
	PreviewImage->SetBrushFromTexture(Preview ? Preview->Thumbnail.LoadSynchronous() : nullptr, true);
	SelectedNameText->SetText(Preview ? WardrobeText(Game, Preview->DisplayNameStringKey) : FText::GetEmpty());
	const bool bUnlocked = Preview && Game->IsOutfitUnlocked(PreviewOutfitId);
	const bool bEquipped = Preview && PreviewOutfitId == EquippedId;
	const FName StatusKey = bApplyFailed ? FName(TEXT("ui.wardrobe.apply_failed"))
		: !Preview ? FName(TEXT("ui.wardrobe.unavailable"))
		: !bUnlocked ? FName(TEXT("ui.wardrobe.locked_hint"))
		: bEquipped ? FName(TEXT("ui.wardrobe.equipped")) : NAME_None;
	StatusText->SetText(StatusKey.IsNone() ? FText::GetEmpty() : WardrobeText(Game, StatusKey));
	StatusText->SetColorAndOpacity(bApplyFailed ? FLinearColor(1.0f, 0.4f, 0.32f) : FLinearColor(0.59f, 0.82f, 0.8f));
	EquipText->SetText(WardrobeText(Game, bEquipped ? TEXT("ui.wardrobe.equipped") : TEXT("ui.wardrobe.equip")));
	EquipButton->SetIsEnabled(bUnlocked && !bEquipped);
}

void UTunaSweeperWardrobePanelWidget::SelectOutfit(FName OutfitId)
{
	PreviewOutfitId = OutfitId;
	bApplyFailed = false;
	RefreshWardrobe();
}

void UTunaSweeperWardrobePanelWidget::HandleEquipClicked()
{
	ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	bApplyFailed = !Controller || !Controller->TryEquipWardrobeOutfit(PreviewOutfitId);
	RefreshWardrobe();
}

void UTunaSweeperWardrobePanelWidget::HandleCloseClicked()
{
	bApplyFailed = false;
	if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseWardrobePanel();
	}
}
