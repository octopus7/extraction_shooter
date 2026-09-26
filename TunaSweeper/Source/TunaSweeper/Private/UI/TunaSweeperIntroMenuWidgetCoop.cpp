#include "TunaSweeperIntroMenuWidgetShared.h"
#include "UI/TunaSweeperOnlineCoopWidget.h"

void UTunaSweeperIntroMenuWidget::EnsureOnlineCoopEntry()
{
	if (!WidgetTree || bPauseSettingsMode || bDifficultyAdjustmentMode || bStartTravelPending) return;
	if (!OnlineCoopButton)
	{
		UVerticalBox* Menu = Cast<UVerticalBox>(FindIntroWidget(TEXT("MainMenuPanel")));
		UWidgetTree* MenuTree = Menu ? Menu->GetTypedOuter<UWidgetTree>() : nullptr;
		if (!MenuTree) return;
		USizeBox* Box = MenuTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("OnlineCoopButtonBox"));
		OnlineCoopButton = MenuTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OnlineCoopButton"));
		OnlineCoopButtonText = MenuTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OnlineCoopButtonText"));
		Box->SetWidthOverride(418.f);
		Box->SetHeightOverride(98.f);
		Box->SetContent(OnlineCoopButton);
		if (StartButton) OnlineCoopButton->SetStyle(StartButton->GetStyle());
		if (StartButtonText) OnlineCoopButtonText->SetFont(StartButtonText->GetFont());
		OnlineCoopButtonText->SetJustification(ETextJustify::Center);
		OnlineCoopButton->AddChild(OnlineCoopButtonText);
		OnlineCoopButtonText->SetText(ResolveUiText(TEXT("ui.coop.title"), FText::GetEmpty()));

		// Rebuild slots so the insertion updates both the UMG and live Slate trees.
		struct FChildLayout
		{
			UWidget* Child;
			FMargin Padding;
			FSlateChildSize Size;
			EHorizontalAlignment Horizontal;
			EVerticalAlignment Vertical;
		};
		TArray<FChildLayout> Children;
		bool bInserted = false;
		for (UWidget* Child : Menu->GetAllChildren())
		{
			if (Child->GetFName() == TEXT("SettingsButtonBox"))
			{
				Children.Add({Box, FMargin(12, 0, 0, -4), FSlateChildSize(ESlateSizeRule::Automatic), HAlign_Left, VAlign_Center});
				bInserted = true;
			}
			const UVerticalBoxSlot* ExistingSlot = CastChecked<UVerticalBoxSlot>(Child->Slot);
			Children.Add({Child, ExistingSlot->GetPadding(), ExistingSlot->GetSize(), ExistingSlot->GetHorizontalAlignment(), ExistingSlot->GetVerticalAlignment()});
		}
		if (!bInserted) Children.Add({Box, FMargin(12, 0, 0, -4), FSlateChildSize(ESlateSizeRule::Automatic), HAlign_Left, VAlign_Center});
		Menu->ClearChildren();
		for (const FChildLayout& Child : Children)
		{
			UVerticalBoxSlot* ChildSlot = Menu->AddChildToVerticalBox(Child.Child);
			ChildSlot->SetPadding(Child.Padding);
			ChildSlot->SetSize(Child.Size);
			ChildSlot->SetHorizontalAlignment(Child.Horizontal);
			ChildSlot->SetVerticalAlignment(Child.Vertical);
		}
	}
	OnlineCoopButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleOnlineCoopClicked);
}

void UTunaSweeperIntroMenuWidget::HandleOnlineCoopClicked()
{
	if (bStartTravelPending || bPauseSettingsMode || bDifficultyAdjustmentMode) return;
	if (OnlineCoopPanel && OnlineCoopPanel->IsInViewport()) return;
	UClass* Class = LoadClass<UTunaSweeperOnlineCoopWidget>(nullptr, TEXT("/Game/UI/WBP_OnlineCoop.WBP_OnlineCoop_C"));
	if (Class && GetOwningPlayer())
	{
		OnlineCoopPanel = CreateWidget<UTunaSweeperOnlineCoopWidget>(GetOwningPlayer(), Class);
		if (OnlineCoopPanel)
		{
			CastChecked<UTunaSweeperOnlineCoopWidget>(OnlineCoopPanel)->SetReturnFocusWidget(this);
			OnlineCoopPanel->AddToViewport(100);
		}
	}
}
