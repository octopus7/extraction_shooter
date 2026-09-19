#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperUIFont.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

TSharedRef<SWidget> UTunaSweeperVehicleDismountWidget::RebuildWidget()
{
	static const FSlateRoundedBoxBrush KeyBrush(FLinearColor::White, 5.0f, FLinearColor::Black, 1.0f);
	const auto Localized = [this](FName Key)
	{
		const auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
		return GI ? GI->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	};
	return SNew(SHorizontalBox)
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
		[
			SNew(SBorder).BorderImage(&KeyBrush).Padding(FMargin(7, 2))
			[
				SNew(STextBlock).Text_Lambda([Localized] { return Localized(TEXT("ui.key.x")); })
				.Font(TunaSweeperUIFont::MakeFont(nullptr, 18)).ColorAndOpacity(FLinearColor::Black)
			]
		]
		+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(8, 0, 0, 0)
		[
			SNew(STextBlock).Text_Lambda([Localized] { return Localized(TEXT("ui.vehicle.dismount")); })
			.Font(TunaSweeperUIFont::MakeFont(nullptr, 20)).ColorAndOpacity(FLinearColor::White)
			.ShadowOffset(FVector2D(1, 1)).ShadowColorAndOpacity(FLinearColor::Black)
		];
}
