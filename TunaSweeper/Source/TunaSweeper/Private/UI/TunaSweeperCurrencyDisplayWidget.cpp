#include "UI/TunaSweeperCurrencyDisplayWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "UI/TunaSweeperUIFont.h"
#include "Widgets/SWidget.h"

namespace TunaSweeperCurrencyDisplay
{
	const TCHAR* CurrencyCoinIconPath =
		TEXT("/Game/UI/Icons/T_UIIcon_CurrencyCoin_White.T_UIIcon_CurrencyCoin_White");
	constexpr float CoinIconSize = 28.0f;
	constexpr float BalanceFontSize = 28.0f;
	constexpr float CoinToTextGap = 8.0f;
	constexpr float GradientHorizontalPadding = 14.0f;
	constexpr float GradientVerticalPadding = 4.0f;

	template <typename WidgetClass>
	FName MakeWidgetName(UWidgetTree* WidgetTree, const TCHAR* DesiredName)
	{
		const FName BaseName(DesiredName);
		return WidgetTree && WidgetTree->FindWidget(BaseName)
			? MakeUniqueObjectName(WidgetTree, WidgetClass::StaticClass(), BaseName)
			: BaseName;
	}

	void AddHorizontalGradientQuad(
		const FSlateRenderTransform& RenderTransform,
		const FVector2D& Position,
		const FVector2D& Size,
		const FLinearColor& LeftColor,
		const FLinearColor& RightColor,
		TArray<FSlateVertex>& OutVertices,
		TArray<SlateIndex>& OutIndices)
	{
		const SlateIndex BaseIndex = static_cast<SlateIndex>(OutVertices.Num());
		const FColor LeftVertexColor = LeftColor.ToFColor(true);
		const FColor RightVertexColor = RightColor.ToFColor(true);
		const FVector2D TopLeft = Position;
		const FVector2D TopRight = Position + FVector2D(Size.X, 0.0f);
		const FVector2D BottomLeft = Position + FVector2D(0.0f, Size.Y);
		const FVector2D BottomRight = Position + Size;

		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(TopLeft),
			FVector2f::ZeroVector,
			LeftVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(TopRight),
			FVector2f::ZeroVector,
			RightVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(BottomLeft),
			FVector2f::ZeroVector,
			LeftVertexColor));
		OutVertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(
			RenderTransform,
			FVector2f(BottomRight),
			FVector2f::ZeroVector,
			RightVertexColor));

		OutIndices.Add(BaseIndex);
		OutIndices.Add(BaseIndex + 1);
		OutIndices.Add(BaseIndex + 2);
		OutIndices.Add(BaseIndex + 1);
		OutIndices.Add(BaseIndex + 3);
		OutIndices.Add(BaseIndex + 2);
	}

	void DrawCurrencyGradientBackground(
		const FGeometry& AllottedGeometry,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId)
	{
		if (!FSlateApplication::IsInitialized())
		{
			return;
		}

		const FSlateBrush* WhiteBrush = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
		if (!WhiteBrush || !FSlateApplication::Get().GetRenderer())
		{
			return;
		}

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		if (LocalSize.X <= 2.0f || LocalSize.Y <= 2.0f)
		{
			return;
		}

		const FVector2D BackgroundPosition(-GradientHorizontalPadding, -GradientVerticalPadding);
		const FVector2D BackgroundSize(
			LocalSize.X + GradientHorizontalPadding * 2.0f,
			LocalSize.Y + GradientVerticalPadding * 2.0f);
		const float FadeWidth = FMath::Clamp(BackgroundSize.X * 0.28f, 12.0f, 36.0f);
		const float SolidWidth = FMath::Max(1.0f, BackgroundSize.X - FadeWidth * 2.0f);
		const FLinearColor TransparentColor(0.0f, 0.0f, 0.0f, 0.0f);
		const FLinearColor SolidColor(0.0f, 0.0f, 0.0f, 0.48f);

		const FSlateRenderTransform& RenderTransform = AllottedGeometry.GetAccumulatedRenderTransform();
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		Vertices.Reserve(12);
		Indices.Reserve(18);

		AddHorizontalGradientQuad(
			RenderTransform,
			BackgroundPosition,
			FVector2D(FadeWidth, BackgroundSize.Y),
			TransparentColor,
			SolidColor,
			Vertices,
			Indices);
		AddHorizontalGradientQuad(
			RenderTransform,
			BackgroundPosition + FVector2D(FadeWidth, 0.0f),
			FVector2D(SolidWidth, BackgroundSize.Y),
			SolidColor,
			SolidColor,
			Vertices,
			Indices);
		AddHorizontalGradientQuad(
			RenderTransform,
			BackgroundPosition + FVector2D(FadeWidth + SolidWidth, 0.0f),
			FVector2D(FadeWidth, BackgroundSize.Y),
			SolidColor,
			TransparentColor,
			Vertices,
			Indices);

		FSlateDrawElement::MakeCustomVerts(
			OutDrawElements,
			LayerId,
			FSlateApplication::Get().GetRenderer()->GetResourceHandle(*WhiteBrush),
			Vertices,
			Indices,
			nullptr,
			0,
			0);
	}
}

UTexture2D* UTunaSweeperCurrencyDisplayWidget::LoadCurrencyCoinIconTexture()
{
	static TWeakObjectPtr<UTexture2D> CachedTexture;
	if (CachedTexture.IsValid())
	{
		return CachedTexture.Get();
	}

	UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, TunaSweeperCurrencyDisplay::CurrencyCoinIconPath);
	CachedTexture = LoadedTexture;
	return LoadedTexture;
}

void UTunaSweeperCurrencyDisplayWidget::RefreshCurrencyBalance()
{
	const UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	const UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance
		? TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>()
		: nullptr;
	SetCurrencyAmount(QuestSubsystem ? QuestSubsystem->GetCoinBalance() : 0);
}

void UTunaSweeperCurrencyDisplayWidget::SetCurrencyAmount(int32 InCurrencyAmount)
{
	CurrencyAmount = FMath::Max(0, InCurrencyAmount);
	ApplyCurrencyPresentation();
}

void UTunaSweeperCurrencyDisplayWidget::EnsureCurrencyContent()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree"), RF_Transient);
	}

	CacheNamedWidgets();
	if (!WidgetTree->RootWidget || !CoinImage || !BalanceText)
	{
		BuildNativeWidgetTree();
	}

	ApplyCurrencyPresentation();
}

TSharedRef<SWidget> UTunaSweeperCurrencyDisplayWidget::RebuildWidget()
{
	EnsureCurrencyContent();

	TSharedRef<SWidget> RebuiltWidget = Super::RebuildWidget();
	CacheNamedWidgets();
	ApplyCurrencyPresentation();
	return RebuiltWidget;
}

void UTunaSweeperCurrencyDisplayWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	EnsureCurrencyContent();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
			QuestSubsystem->OnQuestProgressChanged.AddUObject(
				this,
				&UTunaSweeperCurrencyDisplayWidget::HandleQuestProgressChanged);
		}
	}

	RefreshCurrencyBalance();
}

void UTunaSweeperCurrencyDisplayWidget::NativeDestruct()
{
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		if (UTunaSweeperQuestSubsystem* QuestSubsystem = TunaGameInstance->GetSubsystem<UTunaSweeperQuestSubsystem>())
		{
			QuestSubsystem->OnQuestProgressChanged.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UTunaSweeperCurrencyDisplayWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureCurrencyContent();
}

int32 UTunaSweeperCurrencyDisplayWidget::NativePaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	TunaSweeperCurrencyDisplay::DrawCurrencyGradientBackground(AllottedGeometry, OutDrawElements, LayerId);

	const int32 PaintedLayerId = Super::NativePaint(
		Args,
		AllottedGeometry,
		MyCullingRect,
		OutDrawElements,
		LayerId + 1,
		InWidgetStyle,
		bParentEnabled);
	return FMath::Max(PaintedLayerId, LayerId + 1);
}

void UTunaSweeperCurrencyDisplayWidget::BuildNativeWidgetTree()
{
	if (!WidgetTree)
	{
		return;
	}

	RootBox = WidgetTree->ConstructWidget<UHorizontalBox>(
		UHorizontalBox::StaticClass(),
		TunaSweeperCurrencyDisplay::MakeWidgetName<UHorizontalBox>(WidgetTree, TEXT("RootBox")));
	USizeBox* CoinSizeBox = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TunaSweeperCurrencyDisplay::MakeWidgetName<USizeBox>(WidgetTree, TEXT("CoinSizeBox")));
	CoinImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TunaSweeperCurrencyDisplay::MakeWidgetName<UImage>(WidgetTree, TEXT("CoinImage")));
	BalanceText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(),
		TunaSweeperCurrencyDisplay::MakeWidgetName<UTextBlock>(WidgetTree, TEXT("BalanceText")));
	if (!RootBox || !CoinSizeBox || !CoinImage || !BalanceText)
	{
		return;
	}

	WidgetTree->RootWidget = RootBox;
	RootBox->SetVisibility(ESlateVisibility::HitTestInvisible);

	CoinSizeBox->SetWidthOverride(TunaSweeperCurrencyDisplay::CoinIconSize);
	CoinSizeBox->SetHeightOverride(TunaSweeperCurrencyDisplay::CoinIconSize);
	CoinSizeBox->SetContent(CoinImage);
	UHorizontalBoxSlot* CoinSlot = RootBox->AddChildToHorizontalBox(CoinSizeBox);
	if (CoinSlot)
	{
		CoinSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		CoinSlot->SetVerticalAlignment(VAlign_Center);
	}

	TunaSweeperUIFont::ApplyFont(BalanceText, TunaSweeperCurrencyDisplay::BalanceFontSize, ETunaSweeperUIFontWeight::Bold);
	BalanceText->SetAutoWrapText(false);
	BalanceText->SetJustification(ETextJustify::Left);
	BalanceText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	BalanceText->SetShadowOffset(FVector2D(1.0f, 1.0f));
	UHorizontalBoxSlot* BalanceSlot = RootBox->AddChildToHorizontalBox(BalanceText);
	if (BalanceSlot)
	{
		BalanceSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		BalanceSlot->SetVerticalAlignment(VAlign_Center);
		BalanceSlot->SetPadding(FMargin(TunaSweeperCurrencyDisplay::CoinToTextGap, 0.0f, 0.0f, 0.0f));
	}
}

void UTunaSweeperCurrencyDisplayWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootBox)
	{
		RootBox = Cast<UHorizontalBox>(WidgetTree->FindWidget(FName(TEXT("RootBox"))));
	}
	if (!CoinImage)
	{
		CoinImage = Cast<UImage>(WidgetTree->FindWidget(FName(TEXT("CoinImage"))));
	}
	if (!BalanceText)
	{
		BalanceText = Cast<UTextBlock>(WidgetTree->FindWidget(FName(TEXT("BalanceText"))));
	}
}

void UTunaSweeperCurrencyDisplayWidget::ApplyCurrencyPresentation()
{
	if (CoinImage)
	{
		CoinImage->SetBrushFromTexture(LoadCurrencyCoinIconTexture(), true);
		CoinImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		CoinImage->SetOpacity(1.0f);
	}

	if (BalanceText)
	{
		BalanceText->SetText(FText::AsNumber(CurrencyAmount));
		BalanceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		BalanceText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
		BalanceText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		TunaSweeperUIFont::ApplyFont(
			BalanceText,
			TunaSweeperCurrencyDisplay::BalanceFontSize,
			ETunaSweeperUIFontWeight::Bold);
	}
}

void UTunaSweeperCurrencyDisplayWidget::HandleQuestProgressChanged()
{
	RefreshCurrencyBalance();
}
