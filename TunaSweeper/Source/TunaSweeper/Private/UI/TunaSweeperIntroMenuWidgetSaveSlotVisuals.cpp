#include "TunaSweeperIntroMenuWidgetShared.h"

void UTunaSweeperIntroMenuWidget::EnsureSaveSlotSelectionRingWidgets()
{
	EnsureSaveSlotSelectionRingContent(
		SaveSlot1Button,
		SaveSlot1Text,
		GeneratedSaveSlot1SelectionRingImage,
		TEXT("SaveSlot1Content"),
		TEXT("SaveSlot1SelectionRingImage"));
	EnsureSaveSlotSelectionRingContent(
		SaveSlot2Button,
		SaveSlot2Text,
		GeneratedSaveSlot2SelectionRingImage,
		TEXT("SaveSlot2Content"),
		TEXT("SaveSlot2SelectionRingImage"));
	EnsureSaveSlotSelectionRingContent(
		SaveSlot3Button,
		SaveSlot3Text,
		GeneratedSaveSlot3SelectionRingImage,
		TEXT("SaveSlot3Content"),
		TEXT("SaveSlot3SelectionRingImage"));
}

void UTunaSweeperIntroMenuWidget::EnsureSaveSlotSelectionRingContent(
	UButton* SlotButton,
	UTextBlock* SlotText,
	TObjectPtr<UImage>& RingImage,
	const TCHAR* ContentWidgetName,
	const TCHAR* RingWidgetName)
{
	if (!WidgetTree || !SlotButton || !SlotText)
	{
		return;
	}

	const FName ContentName(ContentWidgetName);
	const FName RingName(RingWidgetName);
	UOverlay* ContentOverlay = Cast<UOverlay>(FindIntroWidget(ContentName));
	if (!ContentOverlay)
	{
		ContentOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), ContentName);
	}
	if (!ContentOverlay)
	{
		return;
	}

	ContentOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	ContentOverlay->SetClipping(EWidgetClipping::ClipToBounds);

	if (!RingImage)
	{
		RingImage = Cast<UImage>(FindIntroWidget(RingName));
	}
	if (!RingImage)
	{
		RingImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), RingName);
	}
	if (RingImage)
	{
		ConfigureSaveSlotSelectionRingImage(RingImage);
		if (RingImage->GetParent() != ContentOverlay)
		{
			RingImage->RemoveFromParent();
			UOverlaySlot* RingSlot = ContentOverlay->AddChildToOverlay(RingImage);
			if (RingSlot)
			{
				RingSlot->SetHorizontalAlignment(HAlign_Right);
				RingSlot->SetVerticalAlignment(VAlign_Center);
				RingSlot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
			}
		}
	}

	if (SlotButton->GetContent() != ContentOverlay)
	{
		SlotButton->SetContent(ContentOverlay);
	}

	SlotText->RemoveFromParent();
	UOverlaySlot* TextSlot = ContentOverlay->AddChildToOverlay(SlotText);
	if (TextSlot)
	{
		TextSlot->SetHorizontalAlignment(HAlign_Fill);
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetPadding(FMargin(56.0f, 0.0f));
	}
	SlotText->SetJustification(ETextJustify::Center);
	SlotText->SetMargin(FMargin(0.0f));
}

void UTunaSweeperIntroMenuWidget::ConfigureSaveSlotSelectionRingImage(UImage* RingImage)
{
	if (!RingImage)
	{
		return;
	}

	FSlateBrush RingBrush;
	RingBrush.DrawAs = ESlateBrushDrawType::Image;
	RingBrush.SetResourceObject(GetOrCreateSaveSlotSelectionRingTexture());
	RingBrush.TintColor = FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 0.95f));
	RingBrush.SetImageSize(FVector2D(28.0f, 28.0f));

	RingImage->SetBrush(RingBrush);
	RingImage->SetVisibility(ESlateVisibility::Collapsed);
	RingImage->SetRenderOpacity(0.0f);
	RingImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
}

UTexture2D* UTunaSweeperIntroMenuWidget::GetOrCreateSaveSlotSelectionRingTexture()
{
	if (SaveSlotSelectionRingTexture)
	{
		return SaveSlotSelectionRingTexture;
	}

	constexpr int32 TextureSize = 32;
	constexpr float InnerRadius = 9.5f;
	constexpr float OuterRadius = 13.5f;
	constexpr float GapStart = 0.72f;
	constexpr float GapEnd = 0.96f;
	const FVector2D Center((TextureSize - 1) * 0.5f, (TextureSize - 1) * 0.5f);

	TArray<FColor> Pixels;
	Pixels.SetNumZeroed(TextureSize * TextureSize);

	for (int32 Y = 0; Y < TextureSize; ++Y)
	{
		for (int32 X = 0; X < TextureSize; ++X)
		{
			const FVector2D Delta(static_cast<float>(X) - Center.X, static_cast<float>(Y) - Center.Y);
			const float Radius = Delta.Size();
			if (Radius < InnerRadius || Radius > OuterRadius)
			{
				continue;
			}

			float Angle = FMath::Atan2(Delta.Y, Delta.X) / (2.0f * PI);
			if (Angle < 0.0f)
			{
				Angle += 1.0f;
			}
			if (Angle >= GapStart && Angle <= GapEnd)
			{
				continue;
			}

			const float RingEdgeAlpha = FMath::Clamp(
				FMath::Min(Radius - InnerRadius, OuterRadius - Radius) / 1.35f,
				0.0f,
				1.0f);
			const float TailAlpha = Angle < GapStart
				? FMath::Lerp(0.45f, 1.0f, Angle / GapStart)
				: 0.85f;
			const uint8 Alpha = static_cast<uint8>(FMath::RoundToInt(220.0f * RingEdgeAlpha * TailAlpha));
			Pixels[Y * TextureSize + X] = FColor(236, 255, 221, Alpha);
		}
	}

	SaveSlotSelectionRingTexture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
	if (!SaveSlotSelectionRingTexture)
	{
		return nullptr;
	}

	SaveSlotSelectionRingTexture->NeverStream = true;
	SaveSlotSelectionRingTexture->SRGB = true;

	FTexturePlatformData* PlatformData = SaveSlotSelectionRingTexture->GetPlatformData();
	if (!PlatformData || PlatformData->Mips.Num() == 0)
	{
		SaveSlotSelectionRingTexture->UpdateResource();
		return SaveSlotSelectionRingTexture;
	}

	void* TextureData = PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
	PlatformData->Mips[0].BulkData.Unlock();
	SaveSlotSelectionRingTexture->UpdateResource();

	return SaveSlotSelectionRingTexture;
}

void UTunaSweeperIntroMenuWidget::UpdateSaveSlotSelectionRingAnimation(float InDeltaTime)
{
	if (SelectedSaveSlotIndex == INDEX_NONE || !IsSaveSlotSelectionVisible())
	{
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot1SelectionRingImage, false);
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot2SelectionRingImage, false);
		SetSaveSlotSelectionRingSelected(GeneratedSaveSlot3SelectionRingImage, false);
		return;
	}

	SaveSlotSelectionRingAngle = FMath::Fmod(SaveSlotSelectionRingAngle + InDeltaTime * 140.0f, 360.0f);
	const float RingOpacity = 0.82f + 0.08f * FMath::Sin(FMath::DegreesToRadians(SaveSlotSelectionRingAngle));

	auto ApplyRingTransform = [this, RingOpacity](UImage* RingImage, bool bSelected)
	{
		if (!RingImage)
		{
			return;
		}

		if (!bSelected)
		{
			RingImage->SetVisibility(ESlateVisibility::Collapsed);
			RingImage->SetRenderOpacity(0.0f);
			return;
		}

		FWidgetTransform RingTransform;
		RingTransform.Angle = SaveSlotSelectionRingAngle;
		RingTransform.Scale = FVector2D(1.0f, 1.0f);

		RingImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		RingImage->SetRenderTransform(RingTransform);
		RingImage->SetRenderOpacity(RingOpacity);
	};

	ApplyRingTransform(GeneratedSaveSlot1SelectionRingImage, SelectedSaveSlotIndex == 1);
	ApplyRingTransform(GeneratedSaveSlot2SelectionRingImage, SelectedSaveSlotIndex == 2);
	ApplyRingTransform(GeneratedSaveSlot3SelectionRingImage, SelectedSaveSlotIndex == 3);
}

void UTunaSweeperIntroMenuWidget::SetSaveSlotSelectionRingSelected(UImage* RingImage, bool bSelected) const
{
	if (!RingImage)
	{
		return;
	}

	RingImage->SetVisibility(bSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bSelected)
	{
		RingImage->SetRenderOpacity(0.0f);
	}
}

