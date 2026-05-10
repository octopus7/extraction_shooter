#include "UI/TunaSweeperTempOpenLootWidget.h"

#include "Components/Button.h"
#include "Components/TileView.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "UI/TunaSweeperTempOpenLootTileItemObject.h"

void UTunaSweeperTempOpenLootWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (TempCloseButton)
	{
		TempCloseButton->OnClicked.RemoveDynamic(this, &UTunaSweeperTempOpenLootWidget::HandleTempCloseClicked);
		TempCloseButton->OnClicked.AddDynamic(this, &UTunaSweeperTempOpenLootWidget::HandleTempCloseClicked);
	}

	PopulateTempLootTiles();
}

void UTunaSweeperTempOpenLootWidget::HandleTempCloseClicked()
{
	if (APlayerController* PlayerController = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetHideCursorDuringCapture(false);
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		PlayerController->bShowMouseCursor = true;
	}

	RemoveFromParent();
}

void UTunaSweeperTempOpenLootWidget::PopulateTempLootTiles()
{
	if (!TempLootTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return;
	}

	TempTileItemObjects.Reset();
	TempLootTileView->ClearListItems();
	TempLootTileView->SetEntryWidth(128.0f);
	TempLootTileView->SetEntryHeight(150.0f);

	for (const FTunaSweeperTempOpenLootItemData& ItemData : TunaGameInstance->GetOrCreateTempOpenLootItems())
	{
		UTunaSweeperTempOpenLootTileItemObject* ItemObject = NewObject<UTunaSweeperTempOpenLootTileItemObject>(this);
		if (!ItemObject)
		{
			continue;
		}

		ItemObject->Initialize(ItemData);
		TempTileItemObjects.Add(ItemObject);
		TempLootTileView->AddItem(ItemObject);
	}
}

