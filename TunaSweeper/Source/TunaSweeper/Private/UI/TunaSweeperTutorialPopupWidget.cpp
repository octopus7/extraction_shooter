#include "UI/TunaSweeperTutorialPopupWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Subsystem/TunaSweeperTextSubsystem.h"
#include "UObject/StrongObjectPtr.h"

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
		if (UTextBlock* Label = Cast<UTextBlock>(WidgetTree->FindWidget(Pair.Key)))
		{
			FText Text;
			Strings->TryGetTextByKey(Pair.Value, Language, Text);
			Label->SetText(Text);
		}
	}
}
