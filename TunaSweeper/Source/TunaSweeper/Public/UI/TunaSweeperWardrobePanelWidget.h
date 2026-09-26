#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TunaSweeperWardrobePanelWidget.generated.h"

class UButton;
class UImage;
class UTextBlock;
class UTexture2D;
class UUniformGridPanel;

DECLARE_DELEGATE_OneParam(FTunaSweeperOutfitCardSelected, FName);

UCLASS()
class TUNASWEEPER_API UTunaSweeperOutfitCardWidget : public UUserWidget
{

	GENERATED_BODY()

public:
	void Configure(FName InOutfitId, UTexture2D* Thumbnail, const FText& Name, const FText& Status,
		bool bSelected, bool bUnlocked, FTunaSweeperOutfitCardSelected InSelected);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	void BuildCard();
	void RefreshCard();
	UFUNCTION() void HandleClicked();

	UPROPERTY(Transient) TObjectPtr<UButton> CardButton;
	UPROPERTY(Transient) TObjectPtr<UImage> PortraitImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> NameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTexture2D> PortraitTexture;
	FName OutfitId;
	FText OutfitName;
	FText OutfitStatus;
	bool bIsSelected = false;
	bool bIsUnlocked = true;
	FTunaSweeperOutfitCardSelected SelectedDelegate;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperWardrobePanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Wardrobe")
	void OpenWardrobe();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Wardrobe")
	void RefreshWardrobe();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	void BuildPanel();
	void SelectOutfit(FName OutfitId);
	UFUNCTION() void HandleEquipClicked();
	UFUNCTION() void HandleCloseClicked();

	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> OutfitGrid;
	UPROPERTY(Transient) TObjectPtr<UImage> PreviewImage;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PreviewTitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SelectedNameText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UButton> EquipButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquipText;
	UPROPERTY(Transient) TObjectPtr<UButton> CloseButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CloseText;
	UPROPERTY(Transient) TArray<TObjectPtr<UTunaSweeperOutfitCardWidget>> OutfitCards;
	TArray<FName> CardOutfitIds;
	FName PreviewOutfitId;
	bool bApplyFailed = false;
};
