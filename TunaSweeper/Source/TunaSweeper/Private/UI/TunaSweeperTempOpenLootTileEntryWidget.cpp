#include "UI/TunaSweeperTempOpenLootTileEntryWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/TunaSweeperTempOpenLootTileItemObject.h"

void UTunaSweeperTempOpenLootTileEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);

	const UTunaSweeperTempOpenLootTileItemObject* ItemObject = Cast<UTunaSweeperTempOpenLootTileItemObject>(ListItemObject);
	if (!ItemObject)
	{
		return;
	}

	const FTunaSweeperTempOpenLootItemData& ItemData = ItemObject->GetItemData();
	if (TempItemIconImage)
	{
		if (UTexture2D* IconTexture = ItemData.IconTexture.LoadSynchronous())
		{
			TempItemIconImage->SetBrushFromTexture(IconTexture, true);
		}
	}

	if (TempItemQuantityText)
	{
		TempItemQuantityText->SetText(FText::Format(FText::FromString(TEXT("x{0}")), FText::AsNumber(ItemData.Quantity)));
	}
}

