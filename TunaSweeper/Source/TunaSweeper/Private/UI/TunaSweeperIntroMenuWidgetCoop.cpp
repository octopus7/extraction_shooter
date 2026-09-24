#include "TunaSweeperIntroMenuWidgetShared.h"
#include "UI/TunaSweeperOnlineCoopWidget.h"

void UTunaSweeperIntroMenuWidget::EnsureOnlineCoopEntry()
{
	if (!WidgetTree || bPauseSettingsMode) return;
	if (!OnlineCoopButton)
	{
		UVerticalBox* Menu = nullptr;
		for (UPanelWidget* Parent = StartButton ? StartButton->GetParent() : nullptr; Parent; Parent = Parent->GetParent())
		{
			if ((Menu = Cast<UVerticalBox>(Parent))) break;
		}
		if (!Menu) return;
		OnlineCoopButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("OnlineCoopButton"));
		OnlineCoopButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("OnlineCoopButtonText"));
		if (StartButton) OnlineCoopButton->SetStyle(StartButton->GetStyle());
		if (StartButtonText) OnlineCoopButtonText->SetFont(StartButtonText->GetFont());
		OnlineCoopButtonText->SetJustification(ETextJustify::Center);
		OnlineCoopButton->AddChild(OnlineCoopButtonText);
		UVerticalBoxSlot* MenuSlot = Menu->AddChildToVerticalBox(OnlineCoopButton);
		MenuSlot->SetPadding(FMargin(0.f, 6.f));
		MenuSlot->SetHorizontalAlignment(HAlign_Fill);
		OnlineCoopButtonText->SetText(ResolveUiText(TEXT("ui.coop.title"), FText::GetEmpty()));
	}
	OnlineCoopButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandleOnlineCoopClicked);
}

void UTunaSweeperIntroMenuWidget::HandleOnlineCoopClicked()
{
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
