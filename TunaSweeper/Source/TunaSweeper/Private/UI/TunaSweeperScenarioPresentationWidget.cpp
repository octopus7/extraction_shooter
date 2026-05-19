#include "UI/TunaSweeperScenarioPresentationWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

namespace TunaSweeperScenarioPresentation
{
	const FName OpeningScenarioFlag(TEXT("scenario.opening.awakening"));
	const FName BunkerLevelName(TEXT("BunkerMap"));
	const TCHAR* OpeningBackgroundTexturePath = TEXT("/Game/UI/Story/T_Story_OpeningLightParticles.T_Story_OpeningLightParticles");
	constexpr float FadeInSeconds = 1.2f;
	constexpr float MinimumLineSeconds = 2.2f;
	constexpr float FadeOutSeconds = 1.25f;
	constexpr float SystemTypewriterCharactersPerSecond = 13.0f;
	constexpr float SystemStatusTypewriterDelaySeconds = 0.35f;
	constexpr float MonologueTypewriterCharactersPerSecond = 15.0f;
	constexpr float MonologueFullyVisibleHoldSeconds = 0.7f;
	constexpr float MonologueFontSize = 34.0f;
	constexpr float MonologueMinWidth = 280.0f;
	constexpr float MonologueMaxWidth = 1120.0f;
}

namespace
{
	UCanvasPanelSlot* AddFullScreenChild(UCanvasPanel* RootCanvas, UWidget* Widget, int32 ZOrder)
	{
		if (!RootCanvas || !Widget)
		{
			return nullptr;
		}

		UCanvasPanelSlot* Slot = RootCanvas->AddChildToCanvas(Widget);
		if (Slot)
		{
			Slot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			Slot->SetOffsets(FMargin(0.0f));
			Slot->SetAlignment(FVector2D::ZeroVector);
			Slot->SetZOrder(ZOrder);
		}
		return Slot;
	}

	FSlateFontInfo MakeFont(UTextBlock* TextBlock, int32 Size)
	{
		FSlateFontInfo FontInfo = TextBlock ? TextBlock->GetFont() : FSlateFontInfo();
		FontInfo.Size = Size;
		return FontInfo;
	}

	float EstimateLeftAlignedTextWidth(const FString& Text, float FontSize)
	{
		float Width = 0.0f;
		for (const TCHAR Character : Text)
		{
			if (FChar::IsWhitespace(Character))
			{
				Width += FontSize * 0.36f;
			}
			else if (Character < 0x80)
			{
				Width += FontSize * 0.54f;
			}
			else
			{
				Width += FontSize * 0.94f;
			}
		}

		return FMath::Clamp(
			Width,
			TunaSweeperScenarioPresentation::MonologueMinWidth,
			TunaSweeperScenarioPresentation::MonologueMaxWidth);
	}
}

TSharedRef<SWidget> UTunaSweeperScenarioPresentationWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	if (WidgetTree && !WidgetTree->RootWidget)
	{
		BuildPresentationWidget();
	}

	return Super::RebuildWidget();
}

void UTunaSweeperScenarioPresentationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SetIsFocusable(true);
	InitializeMonologueLines();
	BuildPresentationWidget();

	CurrentLineIndex = 0;
	PhaseElapsedSeconds = 0.0f;
	Phase = ETunaSweeperScenarioPresentationPhase::FadeIn;
	bTravelStarted = false;
	ResetSystemTextTypewriter();
	BeginCurrentLine();
	SetFadeOverlayOpacity(1.0f);
}

void UTunaSweeperScenarioPresentationWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	PhaseElapsedSeconds += InDeltaTime;

	if (Phase == ETunaSweeperScenarioPresentationPhase::FadeIn)
	{
		const float Alpha = FMath::Clamp(1.0f - PhaseElapsedSeconds / TunaSweeperScenarioPresentation::FadeInSeconds, 0.0f, 1.0f);
		SetFadeOverlayOpacity(Alpha);
		if (Alpha <= 0.0f)
		{
			Phase = ETunaSweeperScenarioPresentationPhase::DisplayLine;
			PhaseElapsedSeconds = 0.0f;
		}
		return;
	}

	if (Phase == ETunaSweeperScenarioPresentationPhase::DisplayLine)
	{
		UpdateSystemTextTypewriter(InDeltaTime);
		UpdateMonologueTypewriter(InDeltaTime);
		if (IsCurrentLineFullyVisible() && PhaseElapsedSeconds >= GetCurrentLineAutoAdvanceSeconds())
		{
			AdvanceLine();
		}
		return;
	}

	if (Phase == ETunaSweeperScenarioPresentationPhase::FadeOut)
	{
		UpdateSystemTextTypewriter(InDeltaTime);
		UpdateMonologueTypewriter(InDeltaTime);
		const float Alpha = FMath::Clamp(PhaseElapsedSeconds / TunaSweeperScenarioPresentation::FadeOutSeconds, 0.0f, 1.0f);
		SetFadeOverlayOpacity(Alpha);
		if (Alpha >= 1.0f)
		{
			TravelToBunker();
		}
	}
}

FReply UTunaSweeperScenarioPresentationWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	AdvanceOrFillLine();
	return FReply::Handled();
}

FReply UTunaSweeperScenarioPresentationWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	AdvanceOrFillLine();
	return FReply::Handled();
}

void UTunaSweeperScenarioPresentationWidget::AdvanceOrFillLine()
{
	if (Phase == ETunaSweeperScenarioPresentationPhase::FadeIn)
	{
		Phase = ETunaSweeperScenarioPresentationPhase::DisplayLine;
		PhaseElapsedSeconds = 0.0f;
		SetFadeOverlayOpacity(0.0f);
		return;
	}

	if (Phase == ETunaSweeperScenarioPresentationPhase::DisplayLine)
	{
		if (!IsCurrentLineFullyVisible())
		{
			MonologueVisibleCharacterCount = CurrentMonologueFullText.Len();
			MonologueTypewriterAccumulator = static_cast<float>(MonologueVisibleCharacterCount);
			UpdateVisibleMonologueText();
			return;
		}

		AdvanceLine();
		return;
	}

	if (Phase == ETunaSweeperScenarioPresentationPhase::FadeOut)
	{
		TravelToBunker();
	}
}

void UTunaSweeperScenarioPresentationWidget::BuildPresentationWidget()
{
	if (!WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget)
	{
		return;
	}

	UCanvasPanel* RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(),
		TEXT("ScenarioPresentationRoot"));
	BackgroundImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("ScenarioBackgroundImage"));
	VignetteOverlay = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ScenarioVignetteOverlay"));
	TitleText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ScenarioTitleText"));
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ScenarioStatusText"));
	MonologueText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ScenarioMonologueText"));
	PromptText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TEXT("ScenarioPromptText"));
	FadeOverlay = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(),
		TEXT("ScenarioFadeOverlay"));

	if (!RootCanvas || !BackgroundImage || !VignetteOverlay || !TitleText || !StatusText || !MonologueText || !PromptText || !FadeOverlay)
	{
		return;
	}

	WidgetTree->RootWidget = RootCanvas;

	FSlateBrush BackgroundBrush;
	BackgroundBrush.DrawAs = ESlateBrushDrawType::Image;
	BackgroundBrush.TintColor = FSlateColor(FLinearColor(0.78f, 0.88f, 0.96f, 1.0f));
	BackgroundBrush.ImageSize = FVector2D(1920.0f, 1080.0f);
	if (UTexture2D* BackgroundTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperScenarioPresentation::OpeningBackgroundTexturePath))
	{
		BackgroundBrush.SetResourceObject(BackgroundTexture);
	}
	BackgroundImage->SetBrush(BackgroundBrush);
	AddFullScreenChild(RootCanvas, BackgroundImage, 0);

	VignetteOverlay->SetBrushColor(FLinearColor::Black);
	VignetteOverlay->SetRenderOpacity(0.38f);
	VignetteOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	AddFullScreenChild(RootCanvas, VignetteOverlay, 1);

	TitleText->SetText(FText::FromString(TEXT("\uC7AC\uAE30\uB3D9 \uAE30\uB85D")));
	TitleText->SetFont(MakeFont(TitleText, 22));
	TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.86f, 0.94f, 1.0f, 0.88f)));
	TitleText->SetShadowOffset(FVector2D(1.5f, 1.5f));
	TitleText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
	if (UCanvasPanelSlot* TitleSlot = RootCanvas->AddChildToCanvas(TitleText))
	{
		TitleSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		TitleSlot->SetAlignment(FVector2D::ZeroVector);
		TitleSlot->SetPosition(FVector2D(72.0f, 62.0f));
		TitleSlot->SetSize(FVector2D(520.0f, 42.0f));
		TitleSlot->SetZOrder(3);
	}

	StatusText->SetText(FText::FromString(TEXT("B-07 \uBC99\uCEE4 / \uBE44\uC0C1 \uAE30\uC0C1")));
	StatusText->SetFont(MakeFont(StatusText, 18));
	StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.62f, 0.76f, 0.86f, 0.84f)));
	StatusText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	StatusText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.68f));
	if (UCanvasPanelSlot* StatusSlot = RootCanvas->AddChildToCanvas(StatusText))
	{
		StatusSlot->SetAnchors(FAnchors(0.0f, 0.0f));
		StatusSlot->SetAlignment(FVector2D::ZeroVector);
		StatusSlot->SetPosition(FVector2D(72.0f, 100.0f));
		StatusSlot->SetSize(FVector2D(620.0f, 34.0f));
		StatusSlot->SetZOrder(3);
	}

	MonologueText->SetFont(MakeFont(MonologueText, 34));
	MonologueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.96f, 0.98f, 1.0f, 0.95f)));
	MonologueText->SetJustification(ETextJustify::Left);
	MonologueText->SetAutoWrapText(false);
	MonologueText->SetShadowOffset(FVector2D(2.0f, 2.0f));
	MonologueText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.84f));
	if (UCanvasPanelSlot* MonologueSlot = RootCanvas->AddChildToCanvas(MonologueText))
	{
		MonologueSlot->SetAnchors(FAnchors(0.5f, 0.5f));
		MonologueSlot->SetAlignment(FVector2D(0.0f, 0.5f));
		MonologueSlot->SetPosition(FVector2D(-520.0f, 0.0f));
		MonologueSlot->SetSize(FVector2D(1040.0f, 96.0f));
		MonologueSlot->SetZOrder(3);
	}

	PromptText->SetText(FText::FromString(TEXT("\uD074\uB9AD\uD574\uC11C \uACC4\uC18D")));
	PromptText->SetFont(MakeFont(PromptText, 17));
	PromptText->SetColorAndOpacity(FSlateColor(FLinearColor(0.70f, 0.82f, 0.90f, 0.78f)));
	PromptText->SetJustification(ETextJustify::Right);
	PromptText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	PromptText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.64f));
	if (UCanvasPanelSlot* PromptSlot = RootCanvas->AddChildToCanvas(PromptText))
	{
		PromptSlot->SetAnchors(FAnchors(1.0f, 1.0f));
		PromptSlot->SetAlignment(FVector2D(1.0f, 1.0f));
		PromptSlot->SetPosition(FVector2D(-70.0f, -70.0f));
		PromptSlot->SetSize(FVector2D(360.0f, 34.0f));
		PromptSlot->SetZOrder(3);
	}

	FadeOverlay->SetBrushColor(FLinearColor::Black);
	FadeOverlay->SetRenderOpacity(1.0f);
	FadeOverlay->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* FadeSlot = RootCanvas->AddChildToCanvas(FadeOverlay))
	{
		FadeSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		FadeSlot->SetOffsets(FMargin(0.0f));
		FadeSlot->SetAlignment(FVector2D::ZeroVector);
		FadeSlot->SetZOrder(10);
	}
}

void UTunaSweeperScenarioPresentationWidget::InitializeMonologueLines()
{
	MonologueLines.Reset();
	MonologueLines.Add(FText::FromString(TEXT("\uAE30\uC5B5 \uB370\uC774\uD130 \uC190\uC0C1.")));
	MonologueLines.Add(FText::FromString(TEXT("\uC704\uCE58 \uD655\uC778... B-07 \uBC99\uCEE4.")));
	MonologueLines.Add(FText::FromString(TEXT("\uC0DD\uC874 \uAE30\uB2A5 \uC7AC\uC2DC\uB3D9.")));
	MonologueLines.Add(FText::FromString(TEXT("\uC804\uB825 \uBD80\uC871. \uB0B4\uBD80 \uC2DC\uC2A4\uD15C \uBD88\uC548\uC815.")));
	MonologueLines.Add(FText::FromString(TEXT("\uADF8\uB798\uB3C4 \uC6C0\uC9C1\uC77C \uC218 \uC788\uB2E4.")));
	MonologueLines.Add(FText::FromString(TEXT("\uBA3C\uC800 \uC774 \uC7A5\uC18C\uB97C \uD655\uC778\uD574\uC57C \uD55C\uB2E4.")));

	SystemTitleFullText = TEXT("\uC7AC\uAE30\uB3D9 \uAE30\uB85D");
	SystemStatusFullText = TEXT("B-07 \uBC99\uCEE4 / \uBE44\uC0C1 \uAE30\uC0C1");
}

void UTunaSweeperScenarioPresentationWidget::BeginCurrentLine()
{
	if (!MonologueText || MonologueLines.IsEmpty())
	{
		return;
	}

	CurrentLineIndex = FMath::Clamp(CurrentLineIndex, 0, MonologueLines.Num() - 1);
	CurrentMonologueFullText = MonologueLines[CurrentLineIndex].ToString();
	MonologueTypewriterAccumulator = 0.0f;
	MonologueVisibleCharacterCount = 0;
	UpdateMonologueTextPlacement();
	UpdateVisibleMonologueText();
}

void UTunaSweeperScenarioPresentationWidget::UpdateMonologueTextPlacement()
{
	if (!MonologueText || MonologueLines.IsEmpty())
	{
		return;
	}

	UCanvasPanelSlot* MonologueSlot = Cast<UCanvasPanelSlot>(MonologueText->Slot);
	if (!MonologueSlot)
	{
		return;
	}

	const float TextWidth = EstimateLeftAlignedTextWidth(CurrentMonologueFullText, TunaSweeperScenarioPresentation::MonologueFontSize);
	MonologueSlot->SetPosition(FVector2D(TextWidth * -0.5f, 0.0f));
	MonologueSlot->SetSize(FVector2D(TextWidth + 24.0f, 96.0f));
}

void UTunaSweeperScenarioPresentationWidget::UpdateMonologueTypewriter(float DeltaTime)
{
	if (IsCurrentLineFullyVisible())
	{
		return;
	}

	MonologueTypewriterAccumulator +=
		FMath::Max(0.0f, DeltaTime) *
		TunaSweeperScenarioPresentation::MonologueTypewriterCharactersPerSecond;

	const int32 TargetVisibleCharacters = FMath::Clamp(
		FMath::FloorToInt(MonologueTypewriterAccumulator),
		0,
		CurrentMonologueFullText.Len());

	if (TargetVisibleCharacters != MonologueVisibleCharacterCount)
	{
		MonologueVisibleCharacterCount = TargetVisibleCharacters;
		UpdateVisibleMonologueText();
	}
}

void UTunaSweeperScenarioPresentationWidget::UpdateVisibleMonologueText()
{
	if (MonologueText)
	{
		MonologueText->SetText(FText::FromString(CurrentMonologueFullText.Left(MonologueVisibleCharacterCount)));
	}

	if (PromptText)
	{
		PromptText->SetVisibility(Phase == ETunaSweeperScenarioPresentationPhase::DisplayLine && IsCurrentLineFullyVisible()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperScenarioPresentationWidget::ResetSystemTextTypewriter()
{
	SystemTypewriterElapsedSeconds = 0.0f;
	SystemTitleVisibleCharacters = 0;
	SystemStatusVisibleCharacters = 0;

	if (TitleText)
	{
		TitleText->SetText(FText::GetEmpty());
	}
	if (StatusText)
	{
		StatusText->SetText(FText::GetEmpty());
	}
}

void UTunaSweeperScenarioPresentationWidget::UpdateSystemTextTypewriter(float DeltaTime)
{
	SystemTypewriterElapsedSeconds += DeltaTime;

	const int32 TitleCharacters = SystemTitleFullText.Len();
	const int32 StatusCharacters = SystemStatusFullText.Len();
	SystemTitleVisibleCharacters = FMath::Clamp(
		FMath::FloorToInt(SystemTypewriterElapsedSeconds * TunaSweeperScenarioPresentation::SystemTypewriterCharactersPerSecond),
		0,
		TitleCharacters);

	const float StatusElapsedSeconds = FMath::Max(
		0.0f,
		SystemTypewriterElapsedSeconds -
			static_cast<float>(TitleCharacters) / TunaSweeperScenarioPresentation::SystemTypewriterCharactersPerSecond -
			TunaSweeperScenarioPresentation::SystemStatusTypewriterDelaySeconds);
	SystemStatusVisibleCharacters = FMath::Clamp(
		FMath::FloorToInt(StatusElapsedSeconds * TunaSweeperScenarioPresentation::SystemTypewriterCharactersPerSecond),
		0,
		StatusCharacters);

	if (TitleText)
	{
		TitleText->SetText(FText::FromString(SystemTitleFullText.Left(SystemTitleVisibleCharacters)));
	}
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(SystemStatusFullText.Left(SystemStatusVisibleCharacters)));
	}
}

void UTunaSweeperScenarioPresentationWidget::AdvanceLine()
{
	if (CurrentLineIndex + 1 < MonologueLines.Num())
	{
		++CurrentLineIndex;
		PhaseElapsedSeconds = 0.0f;
		BeginCurrentLine();
		return;
	}

	StartFadeOut();
}

void UTunaSweeperScenarioPresentationWidget::StartFadeOut()
{
	Phase = ETunaSweeperScenarioPresentationPhase::FadeOut;
	PhaseElapsedSeconds = 0.0f;
	if (PromptText)
	{
		PromptText->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperScenarioPresentationWidget::TravelToBunker()
{
	if (bTravelStarted)
	{
		return;
	}

	bTravelStarted = true;
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->BeginScenarioBunkerEntry(TunaSweeperScenarioPresentation::OpeningScenarioFlag);
	}

	UGameplayStatics::OpenLevel(this, TunaSweeperScenarioPresentation::BunkerLevelName);
}

void UTunaSweeperScenarioPresentationWidget::SetFadeOverlayOpacity(float Opacity)
{
	if (FadeOverlay)
	{
		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
		FadeOverlay->SetBrushColor(FLinearColor::Black);
		FadeOverlay->SetRenderOpacity(ClampedOpacity);
		FadeOverlay->SetVisibility(ClampedOpacity > 0.01f
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

float UTunaSweeperScenarioPresentationWidget::GetCurrentLineAutoAdvanceSeconds() const
{
	const float TypewriterSeconds =
		static_cast<float>(CurrentMonologueFullText.Len()) /
		TunaSweeperScenarioPresentation::MonologueTypewriterCharactersPerSecond;

	return FMath::Max(
		TunaSweeperScenarioPresentation::MinimumLineSeconds,
		TypewriterSeconds + TunaSweeperScenarioPresentation::MonologueFullyVisibleHoldSeconds);
}

bool UTunaSweeperScenarioPresentationWidget::IsCurrentLineFullyVisible() const
{
	return MonologueVisibleCharacterCount >= CurrentMonologueFullText.Len();
}
