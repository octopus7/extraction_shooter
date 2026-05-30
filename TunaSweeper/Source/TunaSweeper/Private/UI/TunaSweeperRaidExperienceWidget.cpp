#include "UI/TunaSweeperRaidExperienceWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace
{
	constexpr float RootPadding = 72.0f;
	constexpr float PanelPadding = 38.0f;

	void ConfigureTextBlock(UTextBlock* TextBlock, float FontSize, const FLinearColor& Color)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo FontInfo = TextBlock->GetFont();
		FontInfo.Size = FMath::RoundToInt(FontSize);
		TextBlock->SetFont(FontInfo);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetMinDesiredWidth(520.0f);
	}

	void ConfigureVerticalSlot(UVerticalBoxSlot* Slot, float TopPadding)
	{
		if (!Slot)
		{
			return;
		}

		Slot->SetHorizontalAlignment(HAlign_Center);
		Slot->SetVerticalAlignment(VAlign_Center);
		Slot->SetPadding(FMargin(0.0f, TopPadding, 0.0f, 0.0f));
	}
}

namespace TunaSweeperRaidExperience
{
	using TunaSweeperUiText::ResolveUiText;
}

TSharedRef<SWidget> UTunaSweeperRaidExperienceWidget::RebuildWidget()
{
	BuildWidgetTree();
	return Super::RebuildWidget();
}

void UTunaSweeperRaidExperienceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperRaidExperienceWidget::HandleLanguageChanged);
	}
	RefreshStaticText();
	RefreshDisplayedExperience(AnimationState.StartExperiencePoints);
	RefreshStatusText();
}

void UTunaSweeperRaidExperienceWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperRaidExperienceWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bAnimationFinished)
	{
		return;
	}

	AnimationElapsedSeconds += FMath::Max(0.0f, InDeltaTime);
	const float DurationSeconds = FMath::Max(0.01f, AnimationState.AnimationDurationSeconds);
	const float Alpha = FMath::Clamp(AnimationElapsedSeconds / DurationSeconds, 0.0f, 1.0f);
	const float SmoothAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, Alpha, 2.0f);
	const int64 DisplayExperience = FMath::RoundToInt64(FMath::Lerp(
		static_cast<double>(AnimationState.StartExperiencePoints),
		static_cast<double>(AnimationState.TargetExperiencePoints),
		static_cast<double>(SmoothAlpha)));
	RefreshDisplayedExperience(DisplayExperience);

	if (Alpha >= 1.0f)
	{
		bAnimationFinished = true;
		RefreshDisplayedExperience(AnimationState.TargetExperiencePoints);
		RefreshStatusText();
		OnAnimationFinished.Broadcast();
	}
}

FReply UTunaSweeperRaidExperienceWidget::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	return TryRequestContinue()
		? FReply::Handled()
		: Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UTunaSweeperRaidExperienceWidget::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	return TryRequestContinue()
		? FReply::Handled()
		: Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UTunaSweeperRaidExperienceWidget::NativeOnTouchStarted(
	const FGeometry& InGeometry,
	const FPointerEvent& InGestureEvent)
{
	return TryRequestContinue()
		? FReply::Handled()
		: Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
}

void UTunaSweeperRaidExperienceWidget::StartExperiencePresentation(
	const FTunaSweeperExperienceAnimationState& InAnimationState)
{
	AnimationState = InAnimationState;
	AnimationState.AnimationDurationSeconds = FMath::Max(0.01f, AnimationState.AnimationDurationSeconds);
	AnimationElapsedSeconds = 0.0f;
	bAnimationFinished = false;
	bContinueReady = false;
	bContinueRequested = false;
	RefreshDisplayedExperience(AnimationState.StartExperiencePoints);
	RefreshStatusText();
}

void UTunaSweeperRaidExperienceWidget::SetContinueReady(bool bReady)
{
	bContinueReady = bReady;
	RefreshStatusText();
}

void UTunaSweeperRaidExperienceWidget::BuildWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RaidExperienceRoot"));
	RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RaidExperienceBox"));
	HeaderText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RaidExperienceHeader"));
	LevelText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RaidExperienceLevel"));
	ExperienceProgressBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("RaidExperienceGaugeBox"));
	ExperienceProgressBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("RaidExperienceGauge"));
	ExperienceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RaidExperienceText"));
	GainText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RaidExperienceGain"));
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("RaidExperienceStatus"));

	WidgetTree->RootWidget = RootBorder;
	if (!RootBorder || !RootBox)
	{
		return;
	}

	FSlateBrush RootBrush;
	RootBrush.DrawAs = ESlateBrushDrawType::Box;
	RootBrush.TintColor = FSlateColor(FLinearColor(0.006f, 0.008f, 0.012f, 0.96f));
	RootBorder->SetBrush(RootBrush);
	RootBorder->SetPadding(FMargin(RootPadding));
	RootBorder->SetHorizontalAlignment(HAlign_Center);
	RootBorder->SetVerticalAlignment(VAlign_Center);

	UVerticalBox* PanelBox = RootBox;
	RootBorder->SetContent(PanelBox);

	ConfigureTextBlock(HeaderText, 26.0f, FLinearColor(0.66f, 0.82f, 1.0f, 1.0f));
	ConfigureTextBlock(LevelText, 68.0f, FLinearColor::White);
	ConfigureTextBlock(ExperienceText, 22.0f, FLinearColor(0.82f, 0.88f, 0.95f, 1.0f));
	ConfigureTextBlock(GainText, 24.0f, FLinearColor(0.44f, 1.0f, 0.58f, 1.0f));
	ConfigureTextBlock(StatusText, 18.0f, FLinearColor(0.62f, 0.70f, 0.78f, 1.0f));

	if (HeaderText)
	{
		HeaderText->SetText(TunaSweeperRaidExperience::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.raid_experience.header"),
			TEXT("RAID EXPERIENCE")));
	}
	if (ExperienceProgressBar)
	{
		ExperienceProgressBar->SetPercent(0.0f);
		ExperienceProgressBar->SetFillColorAndOpacity(FLinearColor(0.12f, 0.82f, 0.38f, 1.0f));
	}
	if (ExperienceProgressBox)
	{
		ExperienceProgressBox->SetWidthOverride(560.0f);
		ExperienceProgressBox->SetHeightOverride(18.0f);
		if (ExperienceProgressBar)
		{
			ExperienceProgressBox->SetContent(ExperienceProgressBar);
		}
	}

	ConfigureVerticalSlot(PanelBox->AddChildToVerticalBox(HeaderText), 0.0f);
	ConfigureVerticalSlot(PanelBox->AddChildToVerticalBox(LevelText), 18.0f);
	ConfigureVerticalSlot(
		PanelBox->AddChildToVerticalBox(ExperienceProgressBox ? Cast<UWidget>(ExperienceProgressBox) : Cast<UWidget>(ExperienceProgressBar)),
		PanelPadding);
	ConfigureVerticalSlot(PanelBox->AddChildToVerticalBox(ExperienceText), 12.0f);
	ConfigureVerticalSlot(PanelBox->AddChildToVerticalBox(GainText), 22.0f);
	ConfigureVerticalSlot(PanelBox->AddChildToVerticalBox(StatusText), 34.0f);
}

void UTunaSweeperRaidExperienceWidget::RefreshStaticText()
{
	if (HeaderText)
	{
		HeaderText->SetText(TunaSweeperRaidExperience::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.raid_experience.header"),
			TEXT("RAID EXPERIENCE")));
	}
}

void UTunaSweeperRaidExperienceWidget::RefreshDisplayedExperience(int64 DisplayExperiencePoints)
{
	LastDisplayedExperiencePoints = DisplayExperiencePoints;
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	const int64 ClampedExperience = FMath::Clamp<int64>(
		DisplayExperiencePoints,
		AnimationState.StartExperiencePoints,
		AnimationState.TargetExperiencePoints);
	const int32 DisplayLevel = TunaGameInstance->GetExperienceLevelForTotal(ClampedExperience);
	const int32 MaxLevel = TunaGameInstance->GetMaxExperienceLevel();
	const int64 LevelStartExperience = TunaGameInstance->GetExperienceForLevel(DisplayLevel);
	const int64 NextLevelExperience = TunaGameInstance->GetExperienceForLevel(DisplayLevel + 1);
	const int64 CurrentLevelExperience = FMath::Max<int64>(0, ClampedExperience - LevelStartExperience);
	const int64 RequiredLevelExperience = FMath::Max<int64>(1, NextLevelExperience - LevelStartExperience);
	const bool bAtMaxLevel = DisplayLevel >= MaxLevel;
	const float Progress = bAtMaxLevel
		? 1.0f
		: static_cast<float>(CurrentLevelExperience) / static_cast<float>(RequiredLevelExperience);

	if (LevelText)
	{
		LevelText->SetText(FText::Format(
			TunaSweeperRaidExperience::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.raid_experience.level_pattern"),
				TEXT("LEVEL {0}")),
			FText::AsNumber(DisplayLevel)));
	}
	if (ExperienceProgressBar)
	{
		ExperienceProgressBar->SetPercent(FMath::Clamp(Progress, 0.0f, 1.0f));
	}
	if (ExperienceText)
	{
		if (bAtMaxLevel)
		{
			ExperienceText->SetText(TunaSweeperRaidExperience::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.raid_experience.max_level"),
				TEXT("MAX LEVEL")));
		}
		else
		{
			ExperienceText->SetText(FText::Format(
				TunaSweeperRaidExperience::ResolveUiText(
					TunaGameInstance,
					TEXT("ui.raid_experience.exp_pattern"),
					TEXT("{0} / {1} EXP")),
				FText::AsNumber(CurrentLevelExperience),
				FText::AsNumber(RequiredLevelExperience)));
		}
	}
	if (GainText)
	{
		GainText->SetText(FText::Format(
			TunaSweeperRaidExperience::ResolveUiText(
				TunaGameInstance,
				TEXT("ui.raid_experience.gain_pattern"),
				TEXT("+{0} EXP")),
			FText::AsNumber(AnimationState.GainedExperiencePoints)));
	}
}

void UTunaSweeperRaidExperienceWidget::RefreshStatusText()
{
	if (!StatusText)
	{
		return;
	}

	if (!bAnimationFinished)
	{
		StatusText->SetText(TunaSweeperRaidExperience::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.raid_experience.calculating"),
			TEXT("Calculating raid rewards...")));
		return;
	}

	StatusText->SetText(bContinueReady
		? TunaSweeperRaidExperience::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.raid_experience.return_ready"),
			TEXT("Press any key or click to return to bunker"))
		: TunaSweeperRaidExperience::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.raid_experience.loading_bunker"),
			TEXT("Loading bunker...")));
}

void UTunaSweeperRaidExperienceWidget::HandleLanguageChanged()
{
	RefreshStaticText();
	RefreshDisplayedExperience(LastDisplayedExperiencePoints);
	RefreshStatusText();
}

bool UTunaSweeperRaidExperienceWidget::TryRequestContinue()
{
	if (!bAnimationFinished || !bContinueReady || bContinueRequested)
	{
		return false;
	}

	bContinueRequested = true;
	OnContinueRequested.Broadcast();
	return true;
}
