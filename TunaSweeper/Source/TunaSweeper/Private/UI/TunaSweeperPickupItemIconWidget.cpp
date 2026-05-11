#include "UI/TunaSweeperPickupItemIconWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"

void UTunaSweeperPickupItemIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	CacheNamedWidgets();
	ApplyIconTexture();
}

void UTunaSweeperPickupItemIconWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	CacheNamedWidgets();
	ApplyIconTexture();
}

void UTunaSweeperPickupItemIconWidget::SetIconTexture(UTexture2D* InIconTexture)
{
	CachedIconTexture = InIconTexture;
	ApplyIconTexture();
}

void UTunaSweeperPickupItemIconWidget::CacheNamedWidgets()
{
	if (!ItemIconImage && WidgetTree)
	{
		ItemIconImage = Cast<UImage>(WidgetTree->FindWidget(TEXT("ItemIconImage")));
	}
}

void UTunaSweeperPickupItemIconWidget::ApplyIconTexture()
{
	if (!ItemIconImage)
	{
		return;
	}

	ItemIconImage->SetColorAndOpacity(FLinearColor::White);
	ItemIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
	ItemIconImage->SetOpacity(1.0f);

	if (CachedIconTexture)
	{
		ItemIconImage->SetBrushFromTexture(CachedIconTexture, true);
		ItemIconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
	}
}
