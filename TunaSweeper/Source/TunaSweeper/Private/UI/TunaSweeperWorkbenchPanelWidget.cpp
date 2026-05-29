#include "UI/TunaSweeperWorkbenchPanelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Widget.h"
#include "Engine/Texture2D.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperItemStackTileItemObject.h"
#include "UI/TunaSweeperUIFont.h"
#include "UI/TunaSweeperUiText.h"

namespace TunaSweeperWorkbenchPanel
{
	const FLinearColor EnabledButtonColor(0.05f, 0.33f, 0.78f, 1.0f);
	const FLinearColor DisabledButtonColor(0.24f, 0.25f, 0.27f, 0.78f);
	const FLinearColor NormalTextColor(0.90f, 0.94f, 0.98f, 1.0f);
	const FLinearColor MissingTextColor(0.92f, 0.18f, 0.12f, 1.0f);

	using TunaSweeperUiText::ResolveUiText;

	FText ResolveItemName(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		int32 ItemId,
		ETunaSweeperItemTextLanguage Language)
	{
		FText ItemName;
		if (ItemDataSubsystem && ItemDataSubsystem->TryGetItemNameText(ItemId, Language, ItemName))
		{
			return ItemName;
		}

		return FText::Format(
			ResolveUiText(TunaGameInstance, TEXT("ui.common.item_fallback"), TEXT("Item {0}")),
			FText::AsNumber(ItemId));
	}

	FTunaSweeperItemStackTileData BuildItemTileData(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		int32 ItemId,
		int32 Quantity,
		const FTunaSweeperItemSlotReference& SlotReference,
		ETunaSweeperItemSlotSource Source,
		int32 SourceIndex,
		ETunaSweeperItemTextLanguage Language)
	{
		FTunaSweeperItemStackTileData TileData;
		TileData.ItemStack.ItemId = ItemId;
		TileData.ItemStack.Quantity = FMath::Max(1, Quantity);
		TileData.ItemInstance.ItemId = ItemId;
		TileData.ItemInstance.Quantity = FMath::Max(1, Quantity);
		TileData.Source = Source;
		TileData.SourceIndex = SourceIndex;
		TileData.SlotReference = SlotReference;
		TileData.DisplayName = ResolveItemName(TunaGameInstance, ItemDataSubsystem, ItemId, Language);
		TileData.bIsEmpty = ItemId == INDEX_NONE;

		if (!TileData.bIsEmpty && ItemDataSubsystem)
		{
			FTunaSweeperItemDefinition ItemDefinition;
			if (ItemDataSubsystem->TryGetItemDefinition(ItemId, ItemDefinition))
			{
				TileData.ItemDefinition = ItemDefinition;
				TileData.bHasItemDefinition = true;
				ItemDataSubsystem->TryGetItemTextByKey(ItemDefinition.DescriptionStringKey, Language, TileData.DescriptionText);
				const FString IconObjectPath = ItemDataSubsystem->BuildItemIconObjectPath(ItemDefinition);
				if (!IconObjectPath.IsEmpty())
				{
					TileData.IconTexture = TSoftObjectPtr<UTexture2D>(FSoftObjectPath(IconObjectPath));
				}
			}
		}

		return TileData;
	}

	FText BuildStackListText(
		const UTunaSweeperGameInstance* TunaGameInstance,
		UTunaSweeperItemDataSubsystem* ItemDataSubsystem,
		const TArray<FTunaSweeperItemStack>& ItemStacks,
		ETunaSweeperItemTextLanguage Language)
	{
		TArray<FString> Lines;
		for (const FTunaSweeperItemStack& ItemStack : ItemStacks)
		{
			Lines.Add(FString::Printf(
				TEXT("%s x%d"),
				*ResolveItemName(TunaGameInstance, ItemDataSubsystem, ItemStack.ItemId, Language).ToString(),
				FMath::Max(1, ItemStack.Quantity)));
		}
		return FText::FromString(FString::Join(Lines, TEXT("\n")));
	}

	void SetWidgetModeVisible(UWidget* Widget, bool bVisible)
	{
		if (Widget)
		{
			Widget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
}

void UTunaSweeperWorkbenchPanelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TunaSweeperUIFont::ApplyFontToWidgetTree(this);

	if (CraftButton)
	{
		CraftButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleCraftButtonClicked);
		CraftButton->OnClicked.AddDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleCraftButtonClicked);
	}
	if (DismantleButton)
	{
		DismantleButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleDismantleButtonClicked);
		DismantleButton->OnClicked.AddDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleDismantleButtonClicked);
	}
	if (BlueprintRegisterButton)
	{
		BlueprintRegisterButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleBlueprintRegisterButtonClicked);
		BlueprintRegisterButton->OnClicked.AddDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleBlueprintRegisterButtonClicked);
	}

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnInventoryStateChanged.AddUObject(this, &UTunaSweeperWorkbenchPanelWidget::RefreshWorkbenchView);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.AddUObject(this, &UTunaSweeperWorkbenchPanelWidget::RefreshWorkbenchView);
	}

	RefreshWorkbenchView();
}

void UTunaSweeperWorkbenchPanelWidget::NativeDestruct()
{
	if (CraftButton)
	{
		CraftButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleCraftButtonClicked);
	}
	if (DismantleButton)
	{
		DismantleButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleDismantleButtonClicked);
	}
	if (BlueprintRegisterButton)
	{
		BlueprintRegisterButton->OnClicked.RemoveDynamic(this, &UTunaSweeperWorkbenchPanelWidget::HandleBlueprintRegisterButtonClicked);
	}
	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->OnInventoryStateChanged.RemoveAll(this);
		TunaGameInstance->OnLanguageChanged.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UTunaSweeperWorkbenchPanelWidget::SetWorkbenchContext(int32 WorkbenchId, ETunaSweeperWorkbenchMode WorkbenchMode)
{
	ActiveWorkbenchId = FMath::Max(1, WorkbenchId);
	ActiveWorkbenchMode = WorkbenchMode;
	SelectedCraftRecipeSlotIndex = INDEX_NONE;
	SelectedDismantleSlot = FTunaSweeperItemSlotReference();
	SelectedBlueprintSlot = FTunaSweeperItemSlotReference();

	if (UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>())
	{
		TunaGameInstance->SetActiveWorkbench(ActiveWorkbenchId, ActiveWorkbenchMode);
	}

	RefreshWorkbenchView();
}

void UTunaSweeperWorkbenchPanelWidget::RefreshWorkbenchView()
{
	SetPanelModeVisibility();
	TileObjects.Reset();
	PopulateCraftRecipes();
	PopulateDismantleItems();
	PopulateBlueprintItems();
	RefreshCraftDetails();
	RefreshDismantleDetails();
	RefreshBlueprintDetails();
}

void UTunaSweeperWorkbenchPanelWidget::SelectCraftRecipe(int32 RecipeSlotIndex)
{
	SelectedCraftRecipeSlotIndex = RecipeSlotIndex;
	RefreshCraftDetails();
}

void UTunaSweeperWorkbenchPanelWidget::SelectDismantleCandidate(const FTunaSweeperItemSlotReference& SlotReference)
{
	SelectedDismantleSlot = SlotReference;
	RefreshDismantleDetails();
}

void UTunaSweeperWorkbenchPanelWidget::SelectBlueprintItem(const FTunaSweeperItemSlotReference& SlotReference)
{
	SelectedBlueprintSlot = SlotReference;
	RefreshBlueprintDetails();
}

bool UTunaSweeperWorkbenchPanelWidget::ExecuteSelectedWorkbenchAction()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	if (!TunaGameInstance)
	{
		return false;
	}

	if (ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::Craft)
	{
		return TunaGameInstance->TryCraftActiveWorkbenchRecipe(SelectedCraftRecipeSlotIndex);
	}

	if (ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle)
	{
		TArray<FTunaSweeperItemStack> OverflowItems;
		const bool bDismantled = TunaGameInstance->TryDismantleWorkbenchItemInSlot(SelectedDismantleSlot, OverflowItems);
		if (bDismantled)
		{
			if (ATunaSweeperPlayerController* TunaPlayerController = Cast<ATunaSweeperPlayerController>(GetOwningPlayer()))
			{
				TunaPlayerController->DropWorkbenchOverflowItems(OverflowItems);
			}
		}
		return bDismantled;
	}

	if (ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister)
	{
		return TunaGameInstance->TryRegisterWorkbenchBlueprintFromSlot(SelectedBlueprintSlot);
	}

	return false;
}

void UTunaSweeperWorkbenchPanelWidget::HandleCraftButtonClicked()
{
	ExecuteSelectedWorkbenchAction();
}

void UTunaSweeperWorkbenchPanelWidget::HandleDismantleButtonClicked()
{
	ExecuteSelectedWorkbenchAction();
}

void UTunaSweeperWorkbenchPanelWidget::HandleBlueprintRegisterButtonClicked()
{
	ExecuteSelectedWorkbenchAction();
}

void UTunaSweeperWorkbenchPanelWidget::PopulateCraftRecipes()
{
	if (!CraftRecipeTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TArray<FTunaSweeperWorkbenchRecipeView> Recipes;
	if (TunaGameInstance)
	{
		TunaGameInstance->GetActiveWorkbenchRecipes(Recipes);
	}

	if (!Recipes.IsValidIndex(SelectedCraftRecipeSlotIndex) && Recipes.Num() > 0)
	{
		SelectedCraftRecipeSlotIndex = 0;
	}

	CraftRecipeTileView->ClearListItems();
	for (const FTunaSweeperWorkbenchRecipeView& Recipe : Recipes)
	{
		FTunaSweeperItemSlotReference SlotReference;
		SlotReference.Source = ETunaSweeperItemSlotSource::WorkbenchRecipe;
		SlotReference.SlotIndex = Recipe.SlotIndex;
		FTunaSweeperItemStackTileData TileData = TunaSweeperWorkbenchPanel::BuildItemTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			Recipe.OutputItemId,
			Recipe.OutputQuantity,
			SlotReference,
			ETunaSweeperItemSlotSource::WorkbenchRecipe,
			Recipe.SlotIndex,
			Language);
		TileData.WorkbenchId = Recipe.WorkbenchId;
		TileData.WorkbenchRecipeId = Recipe.RecipeId;
		TileData.WorkbenchMissingIngredientCount = Recipe.MissingIngredientCount;
		TileData.bCanCraftWorkbenchRecipe = Recipe.bCanCraft;

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (TileObject)
		{
			TileObject->Initialize(TileData);
			TileObjects.Add(TileObject);
			CraftRecipeTileView->AddItem(TileObject);
		}
	}
}

void UTunaSweeperWorkbenchPanelWidget::PopulateDismantleItems()
{
	if (!DismantleInventoryTileView && !DismantleStorageTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TArray<FTunaSweeperWorkbenchDismantleCandidateView> Candidates;
	if (TunaGameInstance)
	{
		TunaGameInstance->GetActiveWorkbenchDismantleCandidates(Candidates);
	}
	FTunaSweeperWorkbenchDismantleCandidateView ExistingDismantleSelection;
	if ((!SelectedDismantleSlot.IsValid() ||
		!TunaGameInstance ||
		!TunaGameInstance->TryGetWorkbenchDismantleCandidateFromSlot(SelectedDismantleSlot, ExistingDismantleSelection)) &&
		Candidates.Num() > 0)
	{
		SelectedDismantleSlot = Candidates[0].SlotReference;
	}

	if (DismantleInventoryTileView)
	{
		DismantleInventoryTileView->ClearListItems();
	}
	if (DismantleStorageTileView)
	{
		DismantleStorageTileView->ClearListItems();
	}

	for (const FTunaSweeperWorkbenchDismantleCandidateView& Candidate : Candidates)
	{
		FTunaSweeperItemStackTileData TileData = TunaSweeperWorkbenchPanel::BuildItemTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			Candidate.ItemId,
			Candidate.Quantity,
			Candidate.SlotReference,
			ETunaSweeperItemSlotSource::WorkbenchDismantleItem,
			Candidate.ListIndex,
			Language);
		TileData.WorkbenchDismantleResultText = TunaSweeperWorkbenchPanel::BuildStackListText(
			TunaGameInstance,
			ItemDataSubsystem,
			Candidate.Results,
			Language);
		TileData.bCanDismantleWorkbenchItem = Candidate.bCanDismantle;

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (!TileObject)
		{
			continue;
		}
		TileObject->Initialize(TileData);
		TileObjects.Add(TileObject);

		if (Candidate.SlotReference.Source == ETunaSweeperItemSlotSource::Storage && DismantleStorageTileView)
		{
			DismantleStorageTileView->AddItem(TileObject);
		}
		else if (DismantleInventoryTileView)
		{
			DismantleInventoryTileView->AddItem(TileObject);
		}
	}
}

void UTunaSweeperWorkbenchPanelWidget::PopulateBlueprintItems()
{
	if (!BlueprintItemTileView)
	{
		return;
	}

	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	TArray<FTunaSweeperWorkbenchBlueprintItemView> BlueprintItems;
	if (TunaGameInstance)
	{
		TunaGameInstance->GetActiveWorkbenchBlueprintItems(BlueprintItems);
	}
	if (!SelectedBlueprintSlot.IsValid() && BlueprintItems.Num() > 0)
	{
		SelectedBlueprintSlot = BlueprintItems[0].SlotReference;
	}

	BlueprintItemTileView->ClearListItems();
	for (const FTunaSweeperWorkbenchBlueprintItemView& BlueprintItem : BlueprintItems)
	{
		FTunaSweeperItemStackTileData TileData = TunaSweeperWorkbenchPanel::BuildItemTileData(
			TunaGameInstance,
			ItemDataSubsystem,
			BlueprintItem.ItemId,
			BlueprintItem.Quantity,
			BlueprintItem.SlotReference,
			ETunaSweeperItemSlotSource::WorkbenchBlueprintItem,
			BlueprintItem.ListIndex,
			Language);
		TileData.WorkbenchBlueprintRecipeId = BlueprintItem.RecipeId;
		TileData.bCanRegisterWorkbenchBlueprint = BlueprintItem.bCanRegister;
		TileData.bWorkbenchBlueprintAlreadyUnlocked = BlueprintItem.bAlreadyUnlocked;

		UTunaSweeperItemStackTileItemObject* TileObject = NewObject<UTunaSweeperItemStackTileItemObject>(this);
		if (TileObject)
		{
			TileObject->Initialize(TileData);
			TileObjects.Add(TileObject);
			BlueprintItemTileView->AddItem(TileObject);
		}
	}
}

void UTunaSweeperWorkbenchPanelWidget::RefreshCraftDetails()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	FTunaSweeperWorkbenchRecipeView RecipeView;
	const bool bHasRecipe =
		TunaGameInstance &&
		TunaGameInstance->TryGetActiveWorkbenchRecipeView(SelectedCraftRecipeSlotIndex, RecipeView);

	if (CraftIngredientList)
	{
		CraftIngredientList->ClearChildren();
		if (bHasRecipe && WidgetTree)
		{
			for (const FTunaSweeperWorkbenchIngredientView& Ingredient : RecipeView.Ingredients)
			{
				UTextBlock* IngredientText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
				if (!IngredientText)
				{
					continue;
				}
				TunaSweeperUIFont::ApplyFont(IngredientText, 16);
				IngredientText->SetColorAndOpacity(FSlateColor(Ingredient.MissingQuantity > 0
					? TunaSweeperWorkbenchPanel::MissingTextColor
					: TunaSweeperWorkbenchPanel::NormalTextColor));
				IngredientText->SetText(FText::Format(
					FText::FromString(TEXT("{0} {1}/{2}")),
					TunaSweeperWorkbenchPanel::ResolveItemName(TunaGameInstance, ItemDataSubsystem, Ingredient.ItemId, Language),
					FText::AsNumber(FMath::Max(0, Ingredient.AvailableQuantity)),
					FText::AsNumber(FMath::Max(1, Ingredient.RequiredQuantity))));
				if (UVerticalBoxSlot* IngredientSlot = CraftIngredientList->AddChildToVerticalBox(IngredientText))
				{
					IngredientSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
				}
			}
		}
	}

	if (CraftArrowText)
	{
		CraftArrowText->SetText(FText::FromString(TEXT("\u2193")));
	}
	if (CraftOutputText)
	{
		CraftOutputText->SetText(bHasRecipe
			? FText::Format(
				FText::FromString(TEXT("{0} x{1}")),
				TunaSweeperWorkbenchPanel::ResolveItemName(TunaGameInstance, ItemDataSubsystem, RecipeView.OutputItemId, Language),
				FText::AsNumber(FMath::Max(1, RecipeView.OutputQuantity)))
			: FText::GetEmpty());
	}
	if (CraftOutputImage)
	{
		UTexture2D* OutputTexture = nullptr;
		FTunaSweeperItemDefinition OutputDefinition;
		if (bHasRecipe &&
			ItemDataSubsystem &&
			ItemDataSubsystem->TryGetItemDefinition(RecipeView.OutputItemId, OutputDefinition))
		{
			OutputTexture = TSoftObjectPtr<UTexture2D>(
				FSoftObjectPath(ItemDataSubsystem->BuildItemIconObjectPath(OutputDefinition))).LoadSynchronous();
		}
		CraftOutputImage->SetBrushFromTexture(OutputTexture, true);
		CraftOutputImage->SetOpacity(OutputTexture ? 1.0f : 0.0f);
	}

	SetActionButtonState(CraftButton, bHasRecipe && RecipeView.bCanCraft);
}

void UTunaSweeperWorkbenchPanelWidget::RefreshDismantleDetails()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const ETunaSweeperItemTextLanguage Language = TunaGameInstance
		? TunaGameInstance->GetCurrentTextLanguage()
		: ETunaSweeperItemTextLanguage::English;

	FTunaSweeperWorkbenchDismantleCandidateView CandidateView;
	const bool bHasCandidate =
		TunaGameInstance &&
		TunaGameInstance->TryGetWorkbenchDismantleCandidateFromSlot(SelectedDismantleSlot, CandidateView);
	if (DismantleResultText)
	{
		DismantleResultText->SetText(bHasCandidate
			? TunaSweeperWorkbenchPanel::BuildStackListText(TunaGameInstance, ItemDataSubsystem, CandidateView.Results, Language)
			: FText::GetEmpty());
	}
	SetActionButtonState(DismantleButton, bHasCandidate && CandidateView.bCanDismantle);
}

void UTunaSweeperWorkbenchPanelWidget::RefreshBlueprintDetails()
{
	UTunaSweeperGameInstance* TunaGameInstance = GetGameInstance<UTunaSweeperGameInstance>();
	FTunaSweeperItemInstance ItemInstance;
	FTunaSweeperItemDefinition ItemDefinition;
	UTunaSweeperItemDataSubsystem* ItemDataSubsystem = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UTunaSweeperItemDataSubsystem>()
		: nullptr;
	const bool bHasBlueprint =
		TunaGameInstance &&
		ItemDataSubsystem &&
		TunaGameInstance->TryGetSlotItemInstance(SelectedBlueprintSlot, ItemInstance) &&
		ItemDataSubsystem->TryGetItemDefinition(ItemInstance.ItemId, ItemDefinition);
	const bool bCanRegister =
		bHasBlueprint &&
		!ItemDefinition.BlueprintRecipeId.IsNone() &&
		!TunaGameInstance->IsWorkbenchRecipeUnlocked(ItemDefinition.BlueprintRecipeId);

	if (BlueprintRegisterText)
	{
		BlueprintRegisterText->SetText(bHasBlueprint
			? FText::FromName(ItemDefinition.BlueprintRecipeId)
			: FText::GetEmpty());
	}
	SetActionButtonState(BlueprintRegisterButton, bCanRegister);
}

void UTunaSweeperWorkbenchPanelWidget::SetActionButtonState(UButton* Button, bool bEnabled) const
{
	if (!Button)
	{
		return;
	}

	Button->SetIsEnabled(bEnabled);
	Button->SetBackgroundColor(bEnabled
		? TunaSweeperWorkbenchPanel::EnabledButtonColor
		: TunaSweeperWorkbenchPanel::DisabledButtonColor);
}

void UTunaSweeperWorkbenchPanelWidget::SetPanelModeVisibility() const
{
	const bool bCraftMode = ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::Craft;
	const bool bDismantleMode = ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::Dismantle;
	const bool bBlueprintMode = ActiveWorkbenchMode == ETunaSweeperWorkbenchMode::BlueprintRegister;

	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftRecipeTileView, bCraftMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftIngredientList, bCraftMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftArrowText, bCraftMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftOutputImage, bCraftMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftOutputText, bCraftMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(CraftButton, bCraftMode);

	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleInventoryTileView, bDismantleMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleStorageTileView, bDismantleMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleInventoryHeaderText, bDismantleMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleStorageHeaderText, bDismantleMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleResultText, bDismantleMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(DismantleButton, bDismantleMode);

	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(BlueprintItemTileView, bBlueprintMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(BlueprintRegisterText, bBlueprintMode);
	TunaSweeperWorkbenchPanel::SetWidgetModeVisible(BlueprintRegisterButton, bBlueprintMode);

	if (WorkbenchTitleText)
	{
		switch (ActiveWorkbenchMode)
		{
		case ETunaSweeperWorkbenchMode::Dismantle:
			WorkbenchTitleText->SetText(TunaSweeperWorkbenchPanel::ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.workbench.dismantle"),
				TEXT("\uBD84\uD574")));
			break;
		case ETunaSweeperWorkbenchMode::BlueprintRegister:
			WorkbenchTitleText->SetText(TunaSweeperWorkbenchPanel::ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.workbench.blueprint_register"),
				TEXT("\uC124\uACC4\uB3C4 \uB4F1\uB85D")));
			break;
		default:
			WorkbenchTitleText->SetText(TunaSweeperWorkbenchPanel::ResolveUiText(
				GetGameInstance<UTunaSweeperGameInstance>(),
				TEXT("ui.workbench.craft"),
				TEXT("\uC81C\uC870")));
			break;
		}
	}

	if (DismantleInventoryHeaderText)
	{
		DismantleInventoryHeaderText->SetText(TunaSweeperWorkbenchPanel::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.inventory.title"),
			TEXT("\uC778\uBCA4\uD1A0\uB9AC")));
	}
	if (DismantleStorageHeaderText)
	{
		DismantleStorageHeaderText->SetText(TunaSweeperWorkbenchPanel::ResolveUiText(
			GetGameInstance<UTunaSweeperGameInstance>(),
			TEXT("ui.storage.title"),
			TEXT("\uCC3D\uACE0")));
	}
}
