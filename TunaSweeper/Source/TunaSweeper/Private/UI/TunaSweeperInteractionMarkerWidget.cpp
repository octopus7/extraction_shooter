#include "UI/TunaSweeperInteractionMarkerWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Rendering/Texture2DResource.h"
#include "Styling/SlateBrush.h"
#include "UI/TunaSweeperUIFont.h"

namespace
{
	const TCHAR* OpenedCheckTexturePath = TEXT("/Game/UI/Interaction/T_InteractionMarkerOpenedCheck.T_InteractionMarkerOpenedCheck");

	void ApplyPaintOpacity(UWidget* Widget, float Opacity)
	{
		if (!Widget)
		{
			return;
		}

		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);

		if (UImage* Image = Cast<UImage>(Widget))
		{
			Image->SetRenderOpacity(1.0f);
			Image->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, ClampedOpacity));
			Image->SetBrushTintColor(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, ClampedOpacity)));
			Image->SetOpacity(ClampedOpacity);
			return;
		}

		if (UTextBlock* TextBlock = Cast<UTextBlock>(Widget))
		{
			FLinearColor TextColor = TextBlock->GetColorAndOpacity().GetSpecifiedColor();
			TextColor.A = ClampedOpacity;
			TextBlock->SetColorAndOpacity(FSlateColor(TextColor));

			FLinearColor ShadowColor = TextBlock->GetShadowColorAndOpacity();
			ShadowColor.A = ClampedOpacity;
			TextBlock->SetShadowColorAndOpacity(ShadowColor);
			TextBlock->SetRenderOpacity(1.0f);
			return;
		}

		if (UBorder* Border = Cast<UBorder>(Widget))
		{
			FLinearColor BrushColor = Border->GetBrushColor();
			BrushColor.A = ClampedOpacity;
			Border->SetBrushColor(BrushColor);
			Border->SetRenderOpacity(1.0f);
			return;
		}

		Widget->SetRenderOpacity(ClampedOpacity);
	}

	FSlateBrush BuildBrushWithPaintOpacity(const FSlateBrush& SourceBrush, float Opacity)
	{
		FSlateBrush Brush = SourceBrush;
		const float ClampedOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);

		FLinearColor TintColor = Brush.TintColor.GetSpecifiedColor();
		TintColor.A *= ClampedOpacity;
		Brush.TintColor = FSlateColor(TintColor);

		FLinearColor OutlineColor = Brush.OutlineSettings.Color.GetSpecifiedColor();
		OutlineColor.A *= ClampedOpacity;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);

		return Brush;
	}

	FSlateBrush BuildRoundedBrush(const FVector2D& ImageSize, const FLinearColor& FillColor, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(FLinearColor::Transparent), 0.0f);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush BuildRoundedOutlineBrush(
		const FVector2D& ImageSize,
		const FLinearColor& FillColor,
		const FLinearColor& OutlineColor,
		float Radius,
		float OutlineWidth)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings = FSlateBrushOutlineSettings(Radius, FSlateColor(OutlineColor), OutlineWidth);
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	FSlateBrush BuildCircularBrush(const FVector2D& ImageSize, const FLinearColor& FillColor)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(FillColor);
		Brush.SetImageSize(ImageSize);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
		Brush.OutlineSettings.Color = FSlateColor(FLinearColor::Transparent);
		Brush.OutlineSettings.Width = 0.0f;
		Brush.OutlineSettings.bUseBrushTransparency = false;
		return Brush;
	}

	bool IsPointInsideTriangle(const FVector2D& Point, const FVector2D& A, const FVector2D& B, const FVector2D& C)
	{
		const auto Sign = [](const FVector2D& P1, const FVector2D& P2, const FVector2D& P3)
		{
			return (P1.X - P3.X) * (P2.Y - P3.Y) - (P2.X - P3.X) * (P1.Y - P3.Y);
		};

		const float D1 = Sign(Point, A, B);
		const float D2 = Sign(Point, B, C);
		const float D3 = Sign(Point, C, A);
		const bool bHasNegative = D1 < 0.0f || D2 < 0.0f || D3 < 0.0f;
		const bool bHasPositive = D1 > 0.0f || D2 > 0.0f || D3 > 0.0f;
		return !(bHasNegative && bHasPositive);
	}

	UTexture2D* CreateIndicatorTriangleTexture(bool bPointUp)
	{
		constexpr int32 Width = 14;
		constexpr int32 Height = 8;
		constexpr int32 SamplesPerAxis = 4;

		UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PF_B8G8R8A8);
		if (!Texture || !Texture->GetPlatformData() || Texture->GetPlatformData()->Mips.Num() == 0)
		{
			return Texture;
		}

		Texture->Filter = TF_Bilinear;
		Texture->NeverStream = true;
		Texture->SRGB = true;

		constexpr float VisualCenterCorrectionX = 0.65f;
		const float CenterX = Width * 0.5f + VisualCenterCorrectionX;
		const FVector2D TopPoint(CenterX, 0.6f);
		const FVector2D BottomLeft(CenterX - 5.8f, Height - 1.1f);
		const FVector2D BottomRight(CenterX + 5.8f, Height - 1.1f);
		const FVector2D A = bPointUp ? TopPoint : FVector2D(CenterX, Height - 0.6f);
		const FVector2D B = bPointUp ? BottomLeft : FVector2D(CenterX - 5.8f, 1.1f);
		const FVector2D C = bPointUp ? BottomRight : FVector2D(CenterX + 5.8f, 1.1f);

		TArray<FColor> Pixels;
		Pixels.SetNumZeroed(Width * Height);

		for (int32 Y = 0; Y < Height; ++Y)
		{
			for (int32 X = 0; X < Width; ++X)
			{
				int32 CoveredSamples = 0;
				for (int32 SampleY = 0; SampleY < SamplesPerAxis; ++SampleY)
				{
					for (int32 SampleX = 0; SampleX < SamplesPerAxis; ++SampleX)
					{
						const FVector2D SamplePoint(
							X + (SampleX + 0.5f) / SamplesPerAxis,
							Y + (SampleY + 0.5f) / SamplesPerAxis);
						if (IsPointInsideTriangle(SamplePoint, A, B, C))
						{
							++CoveredSamples;
						}
					}
				}

				const uint8 Alpha = static_cast<uint8>(
					FMath::RoundToInt(255.0f * CoveredSamples / FMath::Square(SamplesPerAxis)));
				Pixels[Y * Width + X] = FColor(255, 255, 255, Alpha);
			}
		}

		FTexture2DMipMap& Mip = Texture->GetPlatformData()->Mips[0];
		void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Data, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
		Mip.BulkData.Unlock();
		Texture->UpdateResource();
		return Texture;
	}

	bool TextArraysEqual(const TArray<FText>& LeftTexts, const TArray<FText>& RightTexts)
	{
		if (LeftTexts.Num() != RightTexts.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < LeftTexts.Num(); ++Index)
		{
			if (!LeftTexts[Index].EqualTo(RightTexts[Index]))
			{
				return false;
			}
		}

		return true;
	}

	constexpr float FocusElasticDurationSeconds = 0.22f;

	float ResolveFocusElasticScale(float ElapsedSeconds)
	{
		const float NormalizedTime = FMath::Clamp(ElapsedSeconds / FocusElasticDurationSeconds, 0.0f, 1.0f);
		if (NormalizedTime >= 1.0f)
		{
			return 1.0f;
		}

		const float Decay = FMath::Pow(1.0f - NormalizedTime, 2.0f);
		const float Wave = FMath::Sin(NormalizedTime * UE_PI * 3.5f);
		return 1.0f + Wave * Decay * 0.16f;
	}

	void ApplyRuntimeTextFont(
		UTextBlock* TextBlock,
		const UTextBlock* ReferenceTextBlock,
		float SizeOffset = 0.0f,
		ETunaSweeperUIFontWeight Weight = ETunaSweeperUIFontWeight::Preserve)
	{
		if (!TextBlock)
		{
			return;
		}

		const float BaseSize = ReferenceTextBlock
			? ReferenceTextBlock->GetFont().Size
			: TextBlock->GetFont().Size;
		TunaSweeperUIFont::ApplyFont(TextBlock, FMath::Max(1.0f, BaseSize + SizeOffset), Weight);
	}
}

void UTunaSweeperInteractionMarkerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheNamedWidgets();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	CacheNamedWidgets();

	if (IsDesignTime())
	{
		CachedAlpha = 1.0f;
		CachedRingScale = 1.0f;
		CachedLabelAlpha = 1.0f;
	}

	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (FocusScaleElapsedSeconds < FocusElasticDurationSeconds)
	{
		FocusScaleElapsedSeconds = FMath::Min(FocusElasticDurationSeconds, FocusScaleElapsedSeconds + InDeltaTime);
		ApplyMultiOptionFocusScales();
	}
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerText(const FText& InText)
{
	TArray<FText> NewOptionTexts;
	NewOptionTexts.Add(InText);

	constexpr int32 NewFocusedOptionIndex = 0;
	const bool bOptionsChanged = !TextArraysEqual(CachedOptionTexts, NewOptionTexts);
	const bool bFocusChanged = CachedFocusedOptionIndex != NewFocusedOptionIndex;

	CachedDisplayText = InText;
	CachedOptionTexts = NewOptionTexts;
	CachedFocusedOptionIndex = NewFocusedOptionIndex;
	if (bOptionsChanged || bFocusChanged)
	{
		FocusScaleElapsedSeconds = 0.0f;
		bMultiOptionListDirty = true;
	}
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetInteractionOptions(const TArray<FText>& InOptions, int32 InFocusedOptionIndex)
{
	const int32 NewFocusedOptionIndex = InOptions.IsValidIndex(InFocusedOptionIndex)
		? InFocusedOptionIndex
		: 0;
	const bool bOptionsChanged = !TextArraysEqual(CachedOptionTexts, InOptions);
	const bool bFocusChanged = CachedFocusedOptionIndex != NewFocusedOptionIndex;

	CachedOptionTexts = InOptions;
	CachedFocusedOptionIndex = NewFocusedOptionIndex;
	if (CachedOptionTexts.IsValidIndex(CachedFocusedOptionIndex))
	{
		CachedDisplayText = CachedOptionTexts[CachedFocusedOptionIndex];
	}
	if ((bOptionsChanged || bFocusChanged) && CachedOptionTexts.Num() > 0)
	{
		FocusScaleElapsedSeconds = 0.0f;
	}
	bMultiOptionListDirty = bMultiOptionListDirty || bOptionsChanged || bFocusChanged;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetRequirementPreview(
	UTexture2D* InIconTexture,
	int32 InRequiredQuantity,
	bool bInShowRequirement)
{
	const int32 NewRequiredQuantity = FMath::Max(0, InRequiredQuantity);
	const bool bRequirementChanged =
		CachedRequirementIconTexture.Get() != InIconTexture ||
		CachedRequiredQuantity != NewRequiredQuantity ||
		bCachedShowRequirement != bInShowRequirement;

	CachedRequirementIconTexture = InIconTexture;
	CachedRequiredQuantity = NewRequiredQuantity;
	bCachedShowRequirement = bInShowRequirement;
	bMultiOptionListDirty = bMultiOptionListDirty || bRequirementChanged;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerPresentation(float InAlpha, float InRingScale, float InLabelAlpha)
{
	const bool bWasLabelHidden = CachedLabelAlpha <= 0.01f;
	CachedAlpha = FMath::Clamp(InAlpha, 0.0f, 1.0f);
	CachedRingScale = FMath::Max(InRingScale, 0.01f);
	CachedLabelAlpha = FMath::Clamp(InLabelAlpha, 0.0f, 1.0f);
	if (bWasLabelHidden && CachedLabelAlpha > 0.01f && CachedOptionTexts.Num() > 0)
	{
		FocusScaleElapsedSeconds = 0.0f;
	}
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::SetMarkerOpened(bool bInOpened)
{
	bCachedOpened = bInOpened;
	ApplyState();
}

void UTunaSweeperInteractionMarkerWidget::CacheNamedWidgets()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!MarkerRoot)
	{
		MarkerRoot = WidgetTree->FindWidget(TEXT("MarkerRoot"));
	}

	if (!RingImage)
	{
		RingImage = WidgetTree->FindWidget(TEXT("RingImage"));
		if (!RingImage)
		{
			RingImage = WidgetTree->FindWidget(TEXT("RingText"));
		}
	}
	if (!CachedRingBrushImageWidget)
	{
		CachedRingBrushImageWidget = Cast<UImage>(WidgetTree->FindWidget(TEXT("RingBrushImage")));
		if (!CachedRingBrushImageWidget)
		{
			CachedRingBrushImageWidget = Cast<UImage>(RingImage);
		}
	}
	if (!bHasCachedRingBrush)
	{
		if (CachedRingBrushImageWidget)
		{
			CachedRingBrush = CachedRingBrushImageWidget->GetBrush();
			bHasCachedRingBrush = true;
		}
	}

	if (!FilledImage)
	{
		FilledImage = WidgetTree->FindWidget(TEXT("FilledImage"));
		if (!FilledImage)
		{
			FilledImage = WidgetTree->FindWidget(TEXT("FilledText"));
		}
	}
	if (!CachedFilledBrushImageWidget)
	{
		CachedFilledBrushImageWidget = Cast<UImage>(WidgetTree->FindWidget(TEXT("FilledBrushImage")));
		if (!CachedFilledBrushImageWidget)
		{
			CachedFilledBrushImageWidget = Cast<UImage>(FilledImage);
		}
	}
	if (!bHasCachedFilledBrush)
	{
		if (CachedFilledBrushImageWidget)
		{
			CachedFilledBrush = CachedFilledBrushImageWidget->GetBrush();
			bHasCachedFilledBrush = true;
		}
	}

	if (!LabelBackground)
	{
		LabelBackground = Cast<UBorder>(WidgetTree->FindWidget(TEXT("LabelBackground")));
	}

	if (!DisplayNameText)
	{
		DisplayNameText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("DisplayNameText")));
	}

	if (!RequirementRoot)
	{
		RequirementRoot = WidgetTree->FindWidget(TEXT("RequirementRoot"));
	}

	if (!RequirementIconImage)
	{
		RequirementIconImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("RequirementIconImage")));
	}

	if (!RequirementQuantityText)
	{
		RequirementQuantityText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("RequirementQuantityText")));
	}

	EnsureRequirementWidgets();
}

UTexture2D* UTunaSweeperInteractionMarkerWidget::ResolveOpenedCheckTexture()
{
	if (!CachedOpenedCheckTexture)
	{
		CachedOpenedCheckTexture = LoadObject<UTexture2D>(nullptr, OpenedCheckTexturePath);
	}

	return CachedOpenedCheckTexture;
}

UTexture2D* UTunaSweeperInteractionMarkerWidget::ResolveIndicatorTriangleTexture(bool bPointUp)
{
	TObjectPtr<UTexture2D>* CachedTexture = bPointUp ? &CachedUpTriangleTexture : &CachedDownTriangleTexture;
	if (!CachedTexture->Get())
	{
		*CachedTexture = CreateIndicatorTriangleTexture(bPointUp);
	}

	return CachedTexture->Get();
}

void UTunaSweeperInteractionMarkerWidget::EnsureRequirementWidgets()
{
	if (!WidgetTree || !LabelBackground || (RequirementRoot && RequirementIconImage && RequirementQuantityText))
	{
		return;
	}

	if (!CachedLabelContentRow)
	{
		CachedLabelContentRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LabelContentRow")));
	}
	if (!CachedLabelContentRow)
	{
		CachedLabelContentRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("LabelContentRow_Runtime"));
		if (!CachedLabelContentRow)
		{
			return;
		}

		UWidget* ExistingContent = LabelBackground->GetContent();
		LabelBackground->SetContent(CachedLabelContentRow);
		if (DisplayNameText && ExistingContent == DisplayNameText)
		{
			if (UHorizontalBoxSlot* DisplayNameSlot = CachedLabelContentRow->AddChildToHorizontalBox(DisplayNameText))
			{
				DisplayNameSlot->SetHorizontalAlignment(HAlign_Left);
				DisplayNameSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	UHorizontalBox* RequirementHorizontalBox = Cast<UHorizontalBox>(RequirementRoot.Get());
	if (!RequirementHorizontalBox)
	{
		RequirementHorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			TEXT("RequirementRoot_Runtime"));
		RequirementRoot = RequirementHorizontalBox;
	}

	if (!RequirementHorizontalBox)
	{
		return;
	}

	if (!RequirementIconImage)
	{
		USizeBox* RequirementIconBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			TEXT("RequirementIconBox_Runtime"));
		RequirementIconImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			TEXT("RequirementIconImage_Runtime"));
		if (RequirementIconBox && RequirementIconImage)
		{
			RequirementIconBox->SetWidthOverride(22.0f);
			RequirementIconBox->SetHeightOverride(22.0f);
			RequirementIconBox->SetContent(RequirementIconImage);
			if (UHorizontalBoxSlot* IconSlot = RequirementHorizontalBox->AddChildToHorizontalBox(RequirementIconBox))
			{
				IconSlot->SetHorizontalAlignment(HAlign_Center);
				IconSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (!RequirementQuantityText)
	{
		RequirementQuantityText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			TEXT("RequirementQuantityText_Runtime"));
		if (RequirementQuantityText)
		{
			if (DisplayNameText)
			{
				RequirementQuantityText->SetColorAndOpacity(DisplayNameText->GetColorAndOpacity());
			}
			ApplyRuntimeTextFont(RequirementQuantityText, DisplayNameText, -1.0f);
			RequirementQuantityText->SetJustification(ETextJustify::Left);

			if (UHorizontalBoxSlot* QuantitySlot = RequirementHorizontalBox->AddChildToHorizontalBox(RequirementQuantityText))
			{
				QuantitySlot->SetPadding(FMargin(4.0f, 0.0f, 0.0f, 0.0f));
				QuantitySlot->SetHorizontalAlignment(HAlign_Left);
				QuantitySlot->SetVerticalAlignment(VAlign_Center);
			}
		}
	}

	if (RequirementRoot && RequirementRoot->GetParent() != CachedLabelContentRow)
	{
		if (UHorizontalBoxSlot* RequirementSlot = CachedLabelContentRow->AddChildToHorizontalBox(RequirementRoot))
		{
			RequirementSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			RequirementSlot->SetHorizontalAlignment(HAlign_Left);
			RequirementSlot->SetVerticalAlignment(VAlign_Center);
		}
	}
}

void UTunaSweeperInteractionMarkerWidget::EnsureSingleLabelContent()
{
	if (!WidgetTree || !LabelBackground)
	{
		return;
	}

	if (!CachedLabelContentRow)
	{
		CachedLabelContentRow = Cast<UHorizontalBox>(WidgetTree->FindWidget(TEXT("LabelContentRow")));
	}

	if (CachedLabelContentRow && LabelBackground->GetContent() != CachedLabelContentRow)
	{
		LabelBackground->SetContent(CachedLabelContentRow);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (CachedMultiOptionListRoot)
	{
		CachedMultiOptionListRoot->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperInteractionMarkerWidget::EnsureMultiOptionList()
{
	if (!WidgetTree || !LabelBackground)
	{
		return;
	}

	if (!CachedMultiOptionListRoot)
	{
		CachedMultiOptionListRoot = Cast<UVerticalBox>(WidgetTree->FindWidget(TEXT("MultiOptionListRoot")));
	}

	if (!CachedMultiOptionListRoot)
	{
		CachedMultiOptionListRoot = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("MultiOptionListRoot_Runtime"));
		bMultiOptionListDirty = true;
	}

	if (CachedMultiOptionListRoot && LabelBackground->GetContent() != CachedMultiOptionListRoot)
	{
		LabelBackground->SetContent(CachedMultiOptionListRoot);
	}

	if (DisplayNameText)
	{
		DisplayNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (CachedMultiOptionListRoot)
	{
		CachedMultiOptionListRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UTunaSweeperInteractionMarkerWidget::RebuildMultiOptionList()
{
	if (!WidgetTree || !CachedMultiOptionListRoot)
	{
		return;
	}

	CachedMultiOptionListRoot->ClearChildren();
	MultiOptionRows.Reset();

	const FSlateBrush DotBrush = BuildCircularBrush(FVector2D(8.0f, 8.0f), FLinearColor::White);
	const FSlateBrush FocusBackgroundBrush = BuildRoundedBrush(
		FVector2D(128.0f, 24.0f),
		FLinearColor(1.0f, 1.0f, 1.0f, 0.95f),
		2.0f);
	const FSlateBrush UnfocusedBackgroundBrush = BuildRoundedBrush(
		FVector2D(128.0f, 24.0f),
		FLinearColor(0.0f, 0.0f, 0.0f, 0.58f),
		2.0f);
	const bool bCanCycleOptions = CachedOptionTexts.Num() > 1;

	for (int32 OptionIndex = 0; OptionIndex < CachedOptionTexts.Num(); ++OptionIndex)
	{
		const bool bFocused = OptionIndex == CachedFocusedOptionIndex;
		const bool bShowFocusedCycleIndicator = bFocused && bCanCycleOptions;
		const bool bShowRequirementInRow = bFocused && bCachedShowRequirement && CachedRequiredQuantity > 0;

		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionRow_%d"), OptionIndex));
		USizeBox* IndicatorBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionIndicatorBox_%d"), OptionIndex));
		UVerticalBox* IndicatorStack = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionIndicator_%d"), OptionIndex));
		USizeBox* UpIndicatorBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionUpBox_%d"), OptionIndex));
		UImage* DotImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDot_%d"), OptionIndex));
		USizeBox* DotBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDotBox_%d"), OptionIndex));
		USizeBox* DownIndicatorBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDownBox_%d"), OptionIndex));
		UImage* UpIndicatorImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionUp_%d"), OptionIndex));
		UImage* DownIndicatorImage = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionDown_%d"), OptionIndex));
		UBorder* OptionBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionBackground_%d"), OptionIndex));
		UHorizontalBox* OptionContent = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionContent_%d"), OptionIndex));
		UTextBlock* OptionText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionText_%d"), OptionIndex));
		UBorder* KeyPromptBackground = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionKeyBackground_%d"), OptionIndex));
		UTextBlock* KeyPromptText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			*FString::Printf(TEXT("InteractionOptionKey_%d"), OptionIndex));

		if (!Row || !IndicatorBox || !IndicatorStack || !UpIndicatorBox || !DotImage || !DotBox ||
			!DownIndicatorBox || !UpIndicatorImage || !DownIndicatorImage || !OptionBackground ||
			!OptionContent || !OptionText || !KeyPromptBackground || !KeyPromptText)
		{
			continue;
		}

		const FLinearColor IndicatorColor = FLinearColor::White;
		IndicatorBox->SetWidthOverride(18.0f);
		IndicatorBox->SetHeightOverride(28.0f);
		IndicatorBox->SetContent(IndicatorStack);

		UpIndicatorBox->SetWidthOverride(14.0f);
		UpIndicatorBox->SetHeightOverride(8.0f);
		UpIndicatorBox->SetContent(UpIndicatorImage);
		if (UTexture2D* UpTexture = ResolveIndicatorTriangleTexture(true))
		{
			UpIndicatorImage->SetBrushFromTexture(UpTexture, true);
		}
		UpIndicatorImage->SetColorAndOpacity(IndicatorColor);
		UpIndicatorImage->SetBrushTintColor(FSlateColor(IndicatorColor));
		UpIndicatorImage->SetVisibility(bShowFocusedCycleIndicator ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

		if (UVerticalBoxSlot* UpSlot = IndicatorStack->AddChildToVerticalBox(UpIndicatorBox))
		{
			UpSlot->SetHorizontalAlignment(HAlign_Center);
			UpSlot->SetVerticalAlignment(VAlign_Center);
			UpSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 2.0f));
		}

		DotBox->SetWidthOverride(8.0f);
		DotBox->SetHeightOverride(8.0f);
		DotBox->SetContent(DotImage);
		DotImage->SetBrush(DotBrush);
		DotImage->SetColorAndOpacity(FLinearColor::White);
		if (UVerticalBoxSlot* DotSlot = IndicatorStack->AddChildToVerticalBox(DotBox))
		{
			DotSlot->SetHorizontalAlignment(HAlign_Center);
			DotSlot->SetVerticalAlignment(VAlign_Center);
			DotSlot->SetPadding(FMargin(0.0f));
		}

		DownIndicatorBox->SetWidthOverride(14.0f);
		DownIndicatorBox->SetHeightOverride(8.0f);
		DownIndicatorBox->SetContent(DownIndicatorImage);
		if (UTexture2D* DownTexture = ResolveIndicatorTriangleTexture(false))
		{
			DownIndicatorImage->SetBrushFromTexture(DownTexture, true);
		}
		DownIndicatorImage->SetColorAndOpacity(IndicatorColor);
		DownIndicatorImage->SetBrushTintColor(FSlateColor(IndicatorColor));
		DownIndicatorImage->SetVisibility(bShowFocusedCycleIndicator ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (UVerticalBoxSlot* DownSlot = IndicatorStack->AddChildToVerticalBox(DownIndicatorBox))
		{
			DownSlot->SetHorizontalAlignment(HAlign_Center);
			DownSlot->SetVerticalAlignment(VAlign_Center);
			DownSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
		}

		if (UHorizontalBoxSlot* IndicatorSlot = Row->AddChildToHorizontalBox(IndicatorBox))
		{
			IndicatorSlot->SetHorizontalAlignment(HAlign_Center);
			IndicatorSlot->SetVerticalAlignment(VAlign_Center);
		}

		Row->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		OptionBackground->SetBrush(bFocused ? FocusBackgroundBrush : UnfocusedBackgroundBrush);
		OptionBackground->SetPadding(bFocused ? FMargin(7.0f, 1.0f, 6.0f, 1.0f) : FMargin(7.0f, 1.0f, 6.0f, 1.0f));
		OptionBackground->SetContent(OptionContent);

		OptionText->SetText(CachedOptionTexts[OptionIndex]);
		OptionText->SetJustification(ETextJustify::Left);
		ApplyRuntimeTextFont(OptionText, DisplayNameText);
		OptionText->SetColorAndOpacity(FSlateColor(bFocused ? FLinearColor::Black : FLinearColor::White));
		if (UHorizontalBoxSlot* TextSlot = OptionContent->AddChildToHorizontalBox(OptionText))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Left);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (bShowRequirementInRow)
		{
			if (CachedRequirementIconTexture)
			{
				USizeBox* RowRequirementIconBox = WidgetTree->ConstructWidget<USizeBox>(
					USizeBox::StaticClass(),
					*FString::Printf(TEXT("InteractionOptionRequirementIconBox_%d"), OptionIndex));
				UImage* RowRequirementIcon = WidgetTree->ConstructWidget<UImage>(
					UImage::StaticClass(),
					*FString::Printf(TEXT("InteractionOptionRequirementIcon_%d"), OptionIndex));

				if (RowRequirementIconBox && RowRequirementIcon)
				{
					RowRequirementIconBox->SetWidthOverride(18.0f);
					RowRequirementIconBox->SetHeightOverride(18.0f);
					RowRequirementIcon->SetBrushFromTexture(CachedRequirementIconTexture, true);
					RowRequirementIcon->SetColorAndOpacity(FLinearColor::White);
					RowRequirementIconBox->SetContent(RowRequirementIcon);
					if (UHorizontalBoxSlot* RequirementIconSlot = OptionContent->AddChildToHorizontalBox(RowRequirementIconBox))
					{
						RequirementIconSlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
						RequirementIconSlot->SetHorizontalAlignment(HAlign_Left);
						RequirementIconSlot->SetVerticalAlignment(VAlign_Center);
					}
				}
			}

			UTextBlock* RowRequirementQuantityText = WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(),
				*FString::Printf(TEXT("InteractionOptionRequirementQuantity_%d"), OptionIndex));
			if (RowRequirementQuantityText)
			{
				RowRequirementQuantityText->SetText(FText::Format(
					FText::FromString(TEXT("x{0}")),
					FText::AsNumber(CachedRequiredQuantity)));
				RowRequirementQuantityText->SetJustification(ETextJustify::Left);
				ApplyRuntimeTextFont(RowRequirementQuantityText, DisplayNameText, -1.0f);
				RowRequirementQuantityText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
				if (UHorizontalBoxSlot* RequirementQuantitySlot = OptionContent->AddChildToHorizontalBox(RowRequirementQuantityText))
				{
					RequirementQuantitySlot->SetPadding(FMargin(CachedRequirementIconTexture ? 3.0f : 8.0f, 0.0f, 0.0f, 0.0f));
					RequirementQuantitySlot->SetHorizontalAlignment(HAlign_Left);
					RequirementQuantitySlot->SetVerticalAlignment(VAlign_Center);
				}
			}
		}

		KeyPromptBackground->SetVisibility(bFocused ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		KeyPromptBackground->SetBrush(BuildRoundedOutlineBrush(
			FVector2D(22.0f, 20.0f),
			FLinearColor::White,
			FLinearColor::Black,
			5.0f,
			1.0f));
		KeyPromptBackground->SetPadding(FMargin(5.0f, 0.0f, 5.0f, 0.0f));
		KeyPromptText->SetText(FText::FromString(TEXT("F")));
		KeyPromptText->SetJustification(ETextJustify::Center);
		ApplyRuntimeTextFont(KeyPromptText, DisplayNameText, -2.0f);
		KeyPromptText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));
		KeyPromptBackground->SetContent(KeyPromptText);
		if (UHorizontalBoxSlot* KeySlot = OptionContent->AddChildToHorizontalBox(KeyPromptBackground))
		{
			KeySlot->SetPadding(FMargin(8.0f, 0.0f, 0.0f, 0.0f));
			KeySlot->SetHorizontalAlignment(HAlign_Left);
			KeySlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UHorizontalBoxSlot* BackgroundSlot = Row->AddChildToHorizontalBox(OptionBackground))
		{
			BackgroundSlot->SetHorizontalAlignment(HAlign_Left);
			BackgroundSlot->SetVerticalAlignment(VAlign_Center);
		}

		if (UVerticalBoxSlot* RowSlot = CachedMultiOptionListRoot->AddChildToVerticalBox(Row))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Left);
			RowSlot->SetVerticalAlignment(VAlign_Center);
		}
		MultiOptionRows.Add(Row);
	}

	bMultiOptionListDirty = false;
	ApplyMultiOptionFocusScales();
}

void UTunaSweeperInteractionMarkerWidget::ApplyMultiOptionFocusScales()
{
	const float FocusScale = ResolveFocusElasticScale(FocusScaleElapsedSeconds);
	for (int32 RowIndex = 0; RowIndex < MultiOptionRows.Num(); ++RowIndex)
	{
		if (UWidget* RowWidget = MultiOptionRows[RowIndex])
		{
			RowWidget->SetRenderScale(RowIndex == CachedFocusedOptionIndex
				? FVector2D(FocusScale, FocusScale)
				: FVector2D(1.0f, 1.0f));
		}
	}
}

void UTunaSweeperInteractionMarkerWidget::ApplyState()
{
	SetRenderOpacity(1.0f);
	const bool bUseMultiOptionList = CachedOptionTexts.Num() > 0;

	if (MarkerRoot)
	{
		MarkerRoot->SetRenderOpacity(1.0f);
	}

	if (RingImage)
	{
		RingImage->SetRenderScale(FVector2D(CachedRingScale));
		RingImage->SetRenderOpacity(1.0f);
	}
	if (CachedRingBrushImageWidget)
	{
		if (!bHasCachedRingBrush)
		{
			CachedRingBrush = CachedRingBrushImageWidget->GetBrush();
			bHasCachedRingBrush = true;
		}
		CachedRingBrushImageWidget->SetBrush(BuildBrushWithPaintOpacity(CachedRingBrush, CachedAlpha));
		CachedRingBrushImageWidget->SetRenderOpacity(1.0f);
		CachedRingBrushImageWidget->SetColorAndOpacity(FLinearColor::White);
	}
	else if (RingImage)
	{
		ApplyPaintOpacity(RingImage, CachedAlpha);
	}

	if (FilledImage)
	{
		FilledImage->SetRenderOpacity(1.0f);
	}
	if (CachedFilledBrushImageWidget)
	{
		if (!bHasCachedFilledBrush)
		{
			CachedFilledBrush = CachedFilledBrushImageWidget->GetBrush();
			bHasCachedFilledBrush = true;
		}

		if (bCachedOpened)
		{
			if (UTexture2D* OpenedCheckTexture = ResolveOpenedCheckTexture())
			{
				CachedFilledBrushImageWidget->SetBrushFromTexture(OpenedCheckTexture, false);
			}
		}
		else if (bHasCachedFilledBrush)
		{
			CachedFilledBrushImageWidget->SetBrush(BuildBrushWithPaintOpacity(CachedFilledBrush, CachedAlpha));
			CachedFilledBrushImageWidget->SetRenderOpacity(1.0f);
			CachedFilledBrushImageWidget->SetColorAndOpacity(FLinearColor::White);
		}
		if (bCachedOpened)
		{
			ApplyPaintOpacity(CachedFilledBrushImageWidget, CachedAlpha);
		}
	}
	else if (FilledImage)
	{
		ApplyPaintOpacity(FilledImage, CachedAlpha);
	}

	if (bUseMultiOptionList)
	{
		EnsureMultiOptionList();
		if (bMultiOptionListDirty)
		{
			RebuildMultiOptionList();
		}

		if (DisplayNameText)
		{
			DisplayNameText->SetText(CachedDisplayText);
			DisplayNameText->SetVisibility(ESlateVisibility::Collapsed);
		}
		if (CachedMultiOptionListRoot)
		{
			CachedMultiOptionListRoot->SetRenderOpacity(CachedLabelAlpha);
		}
		if (LabelBackground)
		{
			LabelBackground->SetBrushColor(FLinearColor::Transparent);
			LabelBackground->SetRenderOpacity(1.0f);
		}
	}
	else
	{
		EnsureSingleLabelContent();

		if (DisplayNameText)
		{
			DisplayNameText->SetText(CachedDisplayText);
			ApplyPaintOpacity(DisplayNameText, CachedLabelAlpha);
		}

		if (LabelBackground)
		{
			LabelBackground->SetBrushColor(FLinearColor::White);
			ApplyPaintOpacity(LabelBackground, CachedLabelAlpha);
		}
	}

	const bool bShowRequirement = !bUseMultiOptionList && bCachedShowRequirement && CachedRequiredQuantity > 0;
	if (RequirementRoot)
	{
		RequirementRoot->SetVisibility(bShowRequirement ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		ApplyPaintOpacity(RequirementRoot, CachedLabelAlpha);
	}

	if (RequirementIconImage)
	{
		if (bShowRequirement && CachedRequirementIconTexture)
		{
			RequirementIconImage->SetBrushFromTexture(CachedRequirementIconTexture, true);
			RequirementIconImage->SetOpacity(1.0f);
		}
		else
		{
			RequirementIconImage->SetBrushFromTexture(nullptr, false);
			RequirementIconImage->SetOpacity(0.0f);
		}
		ApplyPaintOpacity(RequirementIconImage, CachedLabelAlpha);
	}

	if (RequirementQuantityText)
	{
		RequirementQuantityText->SetText(
			bShowRequirement
				? FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(CachedRequiredQuantity))
				: FText::GetEmpty());
		ApplyPaintOpacity(RequirementQuantityText, CachedLabelAlpha);
	}
}
