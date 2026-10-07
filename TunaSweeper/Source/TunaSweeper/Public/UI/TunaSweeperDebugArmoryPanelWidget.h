#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ComboBoxString.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperDebugArmoryPanelWidget.generated.h"

class UButton;
class UImage;
class USpinBox;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;

DECLARE_DELEGATE_OneParam(FTunaSweeperArmoryItemSelected, int32);

UCLASS()
class TUNASWEEPER_API UTunaSweeperArmoryItemCardWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	void Configure(int32 InItemId, UTexture2D* Icon, const FText& Name, bool bSelected,
		FTunaSweeperArmoryItemSelected InSelected);
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
private:
	void BuildCard();
	void RefreshCard();
	UFUNCTION() void HandleClicked();
	UPROPERTY(Transient) TObjectPtr<UButton> CardButton;
	UPROPERTY(Transient) TObjectPtr<UImage> IconImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> IconTexture;
	int32 ItemId = INDEX_NONE;
	FText ItemName;
	bool bIsSelected = false;
	FTunaSweeperArmoryItemSelected SelectedDelegate;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperDebugArmoryPanelWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|DebugArmory")
	void OpenArmory();
	void RefreshArmory();
protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
private:
	void BuildPanel();
	void RefreshItems();
	void SelectItem(int32 ItemId);
	UFUNCTION() void HandleCategoryChanged(FString SelectedItem, ESelectInfo::Type SelectionType);
	UFUNCTION() UWidget* GenerateCategoryLabel(FString Option);
	UFUNCTION() void HandleSupplyClicked();
	UFUNCTION() void HandleCloseClicked();
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> ItemGrid;
	UPROPERTY(Transient) TObjectPtr<UComboBoxString> CategoryCombo;
	UPROPERTY(Transient) TObjectPtr<USpinBox> QuantitySpinBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CategoryText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> QuantityText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedNameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> SupplyButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SupplyText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CloseText;
	UPROPERTY(Transient) TArray<TObjectPtr<UTunaSweeperArmoryItemCardWidget>> ItemCards;
	TArray<FTunaSweeperItemDefinition> Catalog;
	int32 CategoryIndex = 0;
	int32 SelectedItemId = INDEX_NONE;
	FName StatusKey;
	bool bRefreshingCategories = false;
};
