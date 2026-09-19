#include "UI/TunaSweeperVehicleDismountWidget.h"
#include "Game/TunaSweeperGameInstance.h"
#include "UI/TunaSweeperUIFont.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Brushes/SlateColorBrush.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Notifications/SProgressBar.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

void UTunaSweeperVehicleDismountWidget::SetVehicle(ATunaSweeperATVActor* InVehicle)
{
	Vehicle = InVehicle;
	// UE 5.7 resets anchors to (0,0) when setting the desired size.
	// Apply the final anchor after sizing so the HUD remains inside the screen.
	SetDesiredSizeInViewport(FVector2D(180, 58));
	SetAnchorsInViewport(FAnchors(0.5f, 0.82f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
}

TSharedRef<SWidget> UTunaSweeperVehicleDismountWidget::RebuildWidget()
{
	static const FSlateRoundedBoxBrush KeyBrush(FLinearColor::White, 5.0f, FLinearColor::Black, 1.0f);
	static const FProgressBarStyle DurabilityStyle = FProgressBarStyle()
		.SetBackgroundImage(FSlateColorBrush(FLinearColor(0.035f, 0.035f, 0.035f, 1.0f)))
		.SetFillImage(FSlateColorBrush(FLinearColor(0.12f, 0.82f, 0.38f, 1.0f)));
	const auto Localized = [this](FName Key)
	{
		const auto* GI = GetGameInstance<UTunaSweeperGameInstance>();
		return GI ? GI->ResolveLocalizedText(Key, FText::GetEmpty()) : FText::GetEmpty();
	};
	return SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBox).HeightOverride(12.0f)
			.Visibility_Lambda([this] { return Vehicle.IsValid() ? EVisibility::Visible : EVisibility::Hidden; })
			[
				SNew(SProgressBar).Style(&DurabilityStyle)
				.BarFillType(EProgressBarFillType::LeftToRight)
				.FillColorAndOpacity(FLinearColor::White).BorderPadding(FVector2D::ZeroVector)
				.Percent_Lambda([this]() -> TOptional<float>
				{
					const auto* ATV = Vehicle.Get();
					return ATV ? ATV->GetDurabilityRatio() : 0.0f;
				})
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
		[
		SNew(SHorizontalBox)
		// Keep the bar in place when the delayed dismount hint is hidden.
		.Visibility_Lambda([this] { return bDismountHintVisible ? EVisibility::Visible : EVisibility::Hidden; })
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
		]
		];
}
