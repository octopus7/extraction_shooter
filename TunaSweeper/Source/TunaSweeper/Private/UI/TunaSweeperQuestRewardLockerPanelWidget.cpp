#include "UI/TunaSweeperQuestRewardLockerPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperToastSubsystem.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUIStyle.h"

namespace
{
	FText LockerText(const UTunaSweeperGameInstance* Game, FName Key)
	{
		return Game ? Game->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	}
	UTextBlock* LockerLabel(UWidgetTree* Tree, float Size)
	{
		UTextBlock* Label = Tree->ConstructWidget<UTextBlock>();
		TunaSweeperUIStyle::ApplyLabel(Label);
		TunaSweeperUIFont::ApplyFont(Label, Size);
		Label->SetAutoWrapText(true);
		return Label;
	}
}

void UTunaSweeperQuestRewardCardWidget::Configure(int32 InIndex, int32 InItemId, int32 InQuantity,
	UTexture2D* InIcon, const FText& InName, FTunaSweeperReceiveQuestReward InReceive)
{
	Index = InIndex;
	ItemId = InItemId;
	Quantity = InQuantity;
	Icon = InIcon;
	ItemName = InName;
	Receive = MoveTemp(InReceive);
}

TSharedRef<SWidget> UTunaSweeperQuestRewardCardWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	if (!WidgetTree->RootWidget)
	{
		UBorder* Border = WidgetTree->ConstructWidget<UBorder>();
		Border->SetBrush(FSlateColorBrush(FLinearColor(0.04f, 0.07f, 0.085f, 1.0f)));
		Border->SetPadding(FMargin(16));
		WidgetTree->RootWidget = Border;
		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
		Border->SetContent(Stack);
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetWidthOverride(190);
		Size->SetHeightOverride(120);
		UScaleBox* Fit = WidgetTree->ConstructWidget<UScaleBox>();
		Fit->SetStretch(EStretch::ScaleToFit);
		UImage* Image = WidgetTree->ConstructWidget<UImage>();
		Image->SetBrushFromTexture(Icon, true);
		Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		Fit->SetContent(Image);
		Size->SetContent(Fit);
		Stack->AddChildToVerticalBox(Size)->SetHorizontalAlignment(HAlign_Center);
		UTextBlock* Name = LockerLabel(WidgetTree, 18);
		Name->SetText(ItemName);
		Name->SetJustification(ETextJustify::Center);
		Stack->AddChildToVerticalBox(Name)->SetPadding(FMargin(0, 8));
		UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
		UTextBlock* Count = LockerLabel(WidgetTree, 16);
		Count->SetText(FText::Format(LockerText(Game, TEXT("ui.quest_reward_locker.quantity")), FText::AsNumber(Quantity)));
		Count->SetJustification(ETextJustify::Center);
		Stack->AddChildToVerticalBox(Count)->SetPadding(FMargin(0, 0, 0, 10));
		UButton* Button = WidgetTree->ConstructWidget<UButton>();
		TunaSweeperUIStyle::ApplyButton(Button);
		UTextBlock* ReceiveText = LockerLabel(WidgetTree, 18);
		ReceiveText->SetText(LockerText(Game, TEXT("ui.quest_reward_locker.receive")));
		ReceiveText->SetJustification(ETextJustify::Center);
		Button->SetContent(ReceiveText);
		Stack->AddChildToVerticalBox(Button);
		Button->OnClicked.AddDynamic(this, &ThisClass::HandleReceive);
	}
	return Super::RebuildWidget();
}

void UTunaSweeperQuestRewardCardWidget::HandleReceive()
{
	Receive.ExecuteIfBound(Index, ItemId, Quantity);
}

TSharedRef<SWidget> UTunaSweeperQuestRewardLockerPanelWidget::RebuildWidget()
{
	if (!WidgetTree) WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	BuildPanel();
	return Super::RebuildWidget();
}

void UTunaSweeperQuestRewardLockerPanelWidget::BuildPanel()
{
	if (!WidgetTree || WidgetTree->RootWidget) return;
	UBorder* Root = WidgetTree->ConstructWidget<UBorder>();
	Root->SetBrush(FSlateColorBrush(FLinearColor(0.015f, 0.029f, 0.04f, 0.96f)));
	Root->SetPadding(FMargin(48));
	WidgetTree->RootWidget = Root;
	UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>();
	Root->SetContent(Stack);
	UHorizontalBox* Header = WidgetTree->ConstructWidget<UHorizontalBox>();
	Stack->AddChildToVerticalBox(Header)->SetPadding(FMargin(0, 0, 0, 16));
	TitleText = LockerLabel(WidgetTree, 30);
	Header->AddChildToHorizontalBox(TitleText)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	UButton* Close = WidgetTree->ConstructWidget<UButton>();
	TunaSweeperUIStyle::ApplyButton(Close, TunaSweeperUIStyle::EButtonRole::Secondary);
	CloseText = LockerLabel(WidgetTree, 18);
	Close->SetContent(CloseText);
	Header->AddChildToHorizontalBox(Close);
	Close->OnClicked.AddDynamic(this, &ThisClass::HandleClose);
	DescriptionText = LockerLabel(WidgetTree, 18);
	Stack->AddChildToVerticalBox(DescriptionText)->SetPadding(FMargin(0, 0, 0, 24));
	EmptyText = LockerLabel(WidgetTree, 22);
	Stack->AddChildToVerticalBox(EmptyText)->SetPadding(FMargin(0, 16));
	UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>();
	Scroll->SetScrollWhenFocusChanges(EScrollWhenFocusChanges::InstantScroll);
	Stack->AddChildToVerticalBox(Scroll)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	RewardGrid = WidgetTree->ConstructWidget<UUniformGridPanel>();
	RewardGrid->SetSlotPadding(FMargin(8));
	RewardGrid->SetMinDesiredSlotWidth(230);
	RewardGrid->SetMinDesiredSlotHeight(245);
	Scroll->AddChild(RewardGrid);
}

void UTunaSweeperQuestRewardLockerPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.RemoveAll(this);
		Game->OnInventoryStateChanged.RemoveAll(this);
		Game->OnLanguageChanged.AddUObject(this, &ThisClass::RefreshRewards);
		Game->OnInventoryStateChanged.AddUObject(this, &ThisClass::RefreshRewards);
	}
	RefreshRewards();
}

void UTunaSweeperQuestRewardLockerPanelWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>())
	{
		Game->OnLanguageChanged.RemoveAll(this);
		Game->OnInventoryStateChanged.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UTunaSweeperQuestRewardLockerPanelWidget::OpenQuestRewardLocker()
{
	RefreshRewards();
}

void UTunaSweeperQuestRewardLockerPanelWidget::RefreshRewards()
{
	BuildPanel();
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	if (!RewardGrid || !Game) return;
	TitleText->SetText(LockerText(Game, TEXT("ui.quest_reward_locker.title")));
	DescriptionText->SetText(LockerText(Game, TEXT("ui.quest_reward_locker.description")));
	EmptyText->SetText(LockerText(Game, TEXT("ui.quest_reward_locker.empty")));
	CloseText->SetText(LockerText(Game, TEXT("ui.quest_reward_locker.close")));
	UTunaSweeperItemDataSubsystem* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
	RewardGrid->ClearChildren();
	const TArray<FTunaSweeperItemStack> Rewards = Game->GetPendingQuestRewards();
	EmptyText->SetVisibility(Rewards.IsEmpty() ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	for (int32 Index = 0; Index < Rewards.Num(); ++Index)
	{
		const FTunaSweeperItemStack& Reward = Rewards[Index];
		FTunaSweeperItemDefinition Definition;
		FText Name = LockerText(Game, TEXT("ui.quest_reward_locker.unnamed_item"));
		UTexture2D* Icon = nullptr;
		if (Items && Items->TryGetItemDefinition(Reward.ItemId, Definition))
		{
			Items->TryGetItemNameTextByKey(Definition.NameStringKey, Game->GetCurrentTextLanguage(), Name);
			const FString Path = Items->BuildItemIconObjectPath(Definition);
			if (!Path.IsEmpty()) Icon = LoadObject<UTexture2D>(nullptr, *Path);
		}
		UTunaSweeperQuestRewardCardWidget* Card = CreateWidget<UTunaSweeperQuestRewardCardWidget>(GetOwningPlayer());
		if (!Card) continue;
		Card->Configure(Index, Reward.ItemId, Reward.Quantity, Icon, Name,
			FTunaSweeperReceiveQuestReward::CreateUObject(this, &ThisClass::ReceiveReward));
		RewardGrid->AddChildToUniformGrid(Card, Index / 4, Index % 4);
	}
}

void UTunaSweeperQuestRewardLockerPanelWidget::ReceiveReward(int32 Index, int32 ItemId, int32 Quantity)
{
	ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer());
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	if (!Controller || !Game) return;
	// An old card must never receive a different stack after the list changed.
	const TArray<FTunaSweeperItemStack>& Rewards = Game->GetPendingQuestRewards();
	if (!Rewards.IsValidIndex(Index) || Rewards[Index].ItemId != ItemId || Rewards[Index].Quantity != Quantity)
	{
		RefreshRewards();
		return;
	}
	if (!Controller->IsQuestRewardLockerInteractionValid())
	{
		Controller->CloseQuestRewardLockerPanel();
		return;
	}
	if (!Controller->TryReceiveQuestReward(Index))
	{
		if (UTunaSweeperToastSubsystem* Toast = Game->GetSubsystem<UTunaSweeperToastSubsystem>())
		{
			Toast->ShowLocalizedToast(TEXT("ui.quest_reward_locker.no_space"), FText::GetEmpty());
		}
	}
	RefreshRewards();
}

void UTunaSweeperQuestRewardLockerPanelWidget::HandleClose()
{
	if (ATunaSweeperPlayerController* Controller = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
	{
		Controller->CloseQuestRewardLockerPanel();
	}
}
