#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Housing/TunaSweeperHousingTypes.h"
#include "TimerManager.h"
#include "TunaSweeperHousingPanelWidget.generated.h"

class UBorder;
class UButton;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

DECLARE_DELEGATE_TwoParams(FTunaSweeperHousingEntryClickedDelegate, FName, FGuid);
DECLARE_DELEGATE_OneParam(FTunaSweeperHousingEntryStoreDelegate, FGuid);

UCLASS()
class TUNASWEEPER_API UTunaSweeperHousingFacilityEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitializeEntry(
		const FTunaSweeperHousingFacilityView& InView,
		FTunaSweeperHousingEntryClickedDelegate InClickedDelegate,
		FTunaSweeperHousingEntryStoreDelegate InStoreDelegate);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildEntryWidget();
	void RefreshEntryView();
	void ClearStoreHoldTimer();
	void HandleStoreHoldElapsed();

	UFUNCTION()
	void HandleEntryClicked();

	UFUNCTION()
	void HandleEntryPressed();

	UFUNCTION()
	void HandleEntryReleased();

	UPROPERTY(Transient)
	TObjectPtr<UButton> EntryButton;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StateText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> DetailText;

	FTunaSweeperHousingFacilityView View;
	FTunaSweeperHousingEntryClickedDelegate ClickedDelegate;
	FTunaSweeperHousingEntryStoreDelegate StoreDelegate;
	FTimerHandle StoreHoldTimerHandle;
	float PressedSeconds = 0.0f;
	bool bPressed = false;
	bool bLongPressTriggered = false;
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperHousingPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	void RefreshHousingPanel();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void BuildPanelWidget();
	void RebuildFacilityEntries(const TArray<FTunaSweeperHousingFacilityView>& FacilityViews);
	void HandleHousingStateChanged();
	void HandleFacilityClicked(FName FacilityId, FGuid InstanceId);
	void HandleFacilityStoreRequested(FGuid InstanceId);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootPanel;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> PanelStack;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> GuideText;

	UPROPERTY(Transient)
	TObjectPtr<UScrollBox> FacilityListScrollBox;
};
