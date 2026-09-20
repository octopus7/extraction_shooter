#include "UI/TunaSweeperTutorialPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Button.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"

FReply UTunaSweeperTutorialPopupWidget::NativeOnPreviewKeyDown(const FGeometry& Geometry, const FKeyEvent& Event)
{
	const FKey Key = Event.GetKey();
	if (Key == EKeys::F)
	{
		// The interaction key which opened the popup must be released before it can close it.
		if (!Event.IsRepeat()) bCloseKeyPressed = true;
		return FReply::Handled();
	}
	if (Key == EKeys::A || Key == EKeys::D)
	{
		auto* Button = Cast<UButton>(GetWidgetFromName(Key == EKeys::A ? TEXT("PreviousPageButton") : TEXT("NextPageButton")));
		if (Button && Button->IsVisible() && Button->GetIsEnabled() && Button->OnClicked.IsBound())
		{
			if (!Event.IsRepeat()) Button->OnClicked.Broadcast();
			return FReply::Handled();
		}
	}
	return Super::NativeOnPreviewKeyDown(Geometry, Event);
}

FReply UTunaSweeperTutorialPopupWidget::NativeOnKeyUp(const FGeometry& Geometry, const FKeyEvent& Event)
{
	if (Event.GetKey() == EKeys::F)
	{
		const bool bShouldClose = bCloseKeyPressed;
		bCloseKeyPressed = false;
		if (bShouldClose)
		{
			auto* Button = Cast<UButton>(GetWidgetFromName(TEXT("ContinueButton")));
			if (Button && Button->IsVisible() && Button->GetIsEnabled()) Button->OnClicked.Broadcast();
		}
		return FReply::Handled();
	}
	return Super::NativeOnKeyUp(Geometry, Event);
}

void UTunaSweeperTutorialPopupWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	RefreshLocalizedText();
}

void UTunaSweeperTutorialPopupWidget::RefreshLocalizedText()
{
	if (!WidgetTree) return;
	const UTunaSweeperGameInstance* Instance = IsDesignTime() ? nullptr : GetGameInstance<UTunaSweeperGameInstance>();
	const auto Language = Instance ? Instance->GetCurrentTextLanguage() : PreviewLanguage;
	// The designer has no game instance. Read the same CSV through the existing resolver.
	TStrongObjectPtr<UGameInstance> PreviewOwner;
	TStrongObjectPtr<UTunaSweeperTextSubsystem> PreviewStrings;
	const UTunaSweeperTextSubsystem* Strings = Instance ? Instance->GetSubsystem<UTunaSweeperTextSubsystem>() : nullptr;
	if (!Strings)
	{
		PreviewOwner.Reset(NewObject<UGameInstance>());
		PreviewStrings.Reset(NewObject<UTunaSweeperTextSubsystem>(PreviewOwner.Get()));
		Strings = PreviewStrings.Get();
	}
	for (const auto& Pair : LocalizedTextKeys)
	{
		FText Text;
		Strings->TryGetTextByKey(Pair.Value, Language, Text);
		if (UTextBlock* Label = Cast<UTextBlock>(WidgetTree->FindWidget(Pair.Key)))
		{
			Label->SetText(Text);
		}
		else if (URichTextBlock* RichLabel = Cast<URichTextBlock>(WidgetTree->FindWidget(Pair.Key)))
		{
			RichLabel->SetText(Text);
		}
	}
}
