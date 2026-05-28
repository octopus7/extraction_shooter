#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperGameHudWidget.generated.h"

class UTunaSweeperHudBottomStatusWidget;
class UTunaSweeperHudExternalPanelWidget;
class UTunaSweeperHudInventoryAreaWidget;
class UTunaSweeperHudItemInfoPanelWidget;
class UTunaSweeperHudQuickSlotBarWidget;
class UTunaSweeperHudTopReserveWidget;
class UTunaSweeperItemThumbnailSlotWidget;
class UTunaSweeperMapWidget;
class UTunaSweeperMemoWidget;
class UTunaSweeperQuestWidget;
class UTunaSweeperReloadRingWidget;
class UBorder;
class UHorizontalBox;
class UTextBlock;
class UWidget;

UENUM(BlueprintType)
enum class ETunaSweeperHudTransitionEdge : uint8
{
	Auto UMETA(DisplayName = "Auto"),
	FadeOnly UMETA(DisplayName = "Fade Only"),
	Left UMETA(DisplayName = "Left Edge"),
	Right UMETA(DisplayName = "Right Edge"),
	Top UMETA(DisplayName = "Top Edge"),
	Bottom UMETA(DisplayName = "Bottom Edge")
};

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperGameHudWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetCenterPanelsVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetInventoryAreaVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetItemInfoPanelVisible(bool bVisible);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowExternalPanel(ETunaSweeperHudExternalPanelMode PanelMode);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ToggleInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowMemoPanel(int32 MemoId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowQuestPanel(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetHudMode(ETunaSweeperHudMode InHudMode);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	ETunaSweeperHudMode GetHudMode() const { return ActiveHudMode; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsInventoryUiOpen() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudQuickSlotBarWidget* GetQuickSlotBarWidget() const { return QuickSlotBarWidget; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudBottomStatusWidget* GetBottomStatusWidget() const { return BottomStatusWidget; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudTopReserveWidget> TopStatusReserveWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudBottomStatusWidget> BottomStatusWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudQuickSlotBarWidget> QuickSlotBarWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> CenterContentPanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudInventoryAreaWidget> InventoryAreaWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudItemInfoPanelWidget> ItemInfoPanelWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTunaSweeperHudExternalPanelWidget> ExternalPanelWidget;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UWidget> UnsupportedModePanel;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> UnsupportedModeText;

	UPROPERTY(BlueprintReadOnly, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ModeTitleText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Top Status Direction Override"))
	ETunaSweeperHudTransitionEdge TopStatusReserveTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Mode Title Direction Override"))
	ETunaSweeperHudTransitionEdge ModeTitleTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Inventory Area Direction Override"))
	ETunaSweeperHudTransitionEdge InventoryAreaTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Item Info Direction Override"))
	ETunaSweeperHudTransitionEdge ItemInfoPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "External Panel Direction Override"))
	ETunaSweeperHudTransitionEdge ExternalPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Unsupported Mode Direction Override"))
	ETunaSweeperHudTransitionEdge UnsupportedModePanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Inventory Quick Slot Direction Override"))
	ETunaSweeperHudTransitionEdge InventoryQuickSlotPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Map Panel Direction Override"))
	ETunaSweeperHudTransitionEdge MapPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Memo Panel Direction Override"))
	ETunaSweeperHudTransitionEdge MemoPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Quest Panel Direction Override"))
	ETunaSweeperHudTransitionEdge QuestPanelTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Quest Tracker Direction Override"))
	ETunaSweeperHudTransitionEdge QuestTrackerTransitionEdge = ETunaSweeperHudTransitionEdge::Auto;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Bottom Status Direction Override"))
	ETunaSweeperHudTransitionEdge BottomStatusTransitionEdge = ETunaSweeperHudTransitionEdge::Bottom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Quick Slot Bar Direction Override"))
	ETunaSweeperHudTransitionEdge QuickSlotBarTransitionEdge = ETunaSweeperHudTransitionEdge::Bottom;

private:
	struct FHudWidgetTransition
	{
		TWeakObjectPtr<UWidget> Widget;
		FWidgetTransform StartTransform;
		FWidgetTransform EndTransform;
		float StartOpacity = 1.0f;
		float EndOpacity = 1.0f;
		float ElapsedSeconds = 0.0f;
		float DurationSeconds = 0.0f;
		ESlateVisibility FinalVisibility = ESlateVisibility::Collapsed;
		bool bShow = false;
	};

	void ApplyHudModeVisibility();
	void CacheHudTransitionBaseline(UWidget* Widget);
	void CloseLootContainerPanelIfOpen();
	void EnsureInventoryQuickSlotPanelWidget();
	void EnsureMapPanelWidget();
	void EnsureMemoPanelWidget();
	void EnsureQuestPanelWidget();
	ETunaSweeperHudTransitionEdge ResolveHudTransitionEdge(const UWidget* Widget, ETunaSweeperHudTransitionEdge DirectionOverride) const;
	void RefreshBottomStatusFromGameInstance();
	void RefreshQuickSlotsFromGameState();
	void RefreshInventoryQuickSlotPanel();
	void RefreshLocalizedTexts();
	void RefreshReloadWidgets();
	void RefreshDialogueHudVisibility();
	void EnsureQuestTrackerWidgets();
	void RefreshQuestTrackerFromQuestSubsystem();
	FVector2D GetHudTransitionHiddenTranslation(const UWidget* Widget, ETunaSweeperHudTransitionEdge Edge) const;
	bool HasActiveHudTransition(const UWidget* Widget) const;
	void SetTransitionedWidgetVisibility(UWidget* Widget, ESlateVisibility TargetVisibility, ETunaSweeperHudTransitionEdge DirectionOverride);
	void TickHudTransitions(float InDeltaTime);
	void CacheAmmoReloadWidgets();
	void BuildAmmoSelectorOptionTexts(TArray<FText>& OutOptionTexts, int32& OutFocusedIndex) const;
	void HandleSelectedInventoryItemChanged();
	void HandleQuestProgressChanged();
	void HandleLanguageChanged();
	bool IsDialogueSequenceActive() const;

	UFUNCTION()
	void HandleHudModeTabSelected(ETunaSweeperHudMode SelectedMode);

	UPROPERTY(Transient)
	ETunaSweeperHudMode ActiveHudMode = ETunaSweeperHudMode::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> CenterReloadGaugeRoot;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperReloadRingWidget> CenterReloadRingWidget;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> CenterReloadPromptRoot;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CenterReloadPercentText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> QuestTrackerRoot;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuestTrackerTitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> QuestTrackerObjectiveText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InventoryQuickSlotPanel;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> InventoryQuickSlotRow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryQuickSlotGuideText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTunaSweeperItemThumbnailSlotWidget>> InventoryQuickSlotWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperMapWidget> MapPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperMemoWidget> MemoPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperQuestWidget> QuestPanelWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> CenterReloadSegments;

	bool bClearExternalPanelModeAfterHide = false;
	TArray<FHudWidgetTransition> ActiveHudTransitions;
	TMap<TWeakObjectPtr<UWidget>, FWidgetTransform> HudTransitionBaseTransforms;
	TMap<TWeakObjectPtr<UWidget>, float> HudTransitionBaseOpacities;
};
