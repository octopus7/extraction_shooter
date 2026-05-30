#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Inventory/TunaSweeperInventoryTypes.h"
#include "Layout/Anchors.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "UI/TunaSweeperHudTypes.h"
#include "TunaSweeperGameHudWidget.generated.h"

class ATunaSweeperTopDownCharacter;
struct FGeometry;
class UTunaSweeperHudBottomStatusWidget;
class UTunaSweeperHudDebuffBarWidget;
class UTunaSweeperHudExternalPanelWidget;
class UTunaSweeperHousingPanelWidget;
class UTunaSweeperHudInventoryAreaWidget;
class UTunaSweeperHudItemInfoPanelWidget;
class UTunaSweeperHudQuickSlotBarWidget;
class UTunaSweeperHudTopReserveWidget;
class UTunaSweeperShopSellPanelWidget;
class UTunaSweeperExtractionProgressWidget;
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

UENUM(BlueprintType)
enum class ETunaSweeperDamageNumberType : uint8
{
	Normal UMETA(DisplayName = "Normal"),
	Critical UMETA(DisplayName = "Critical"),
	Headshot UMETA(DisplayName = "Headshot")
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
	void ShowStoragePanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowShopPanel(int32 ShopId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowWorkbenchPanel(
		int32 WorkbenchId,
		ETunaSweeperWorkbenchMode WorkbenchMode = ETunaSweeperWorkbenchMode::Craft);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowMemoPanel(int32 MemoId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowQuestPanel(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ShowMenuQuestPanel(FName QuestId = NAME_None);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void SetHudMode(ETunaSweeperHudMode InHudMode);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD|Extraction")
	void SetExtractionProgress(float CurrentSeconds, float RequiredSeconds, bool bActive);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	ETunaSweeperHudMode GetHudMode() const { return ActiveHudMode; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsInventoryUiOpen() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsShopPanelOpen() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	bool IsWorkbenchPanelOpen() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	ETunaSweeperWorkbenchMode GetWorkbenchPanelMode() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryAssignWorkbenchDismantleCandidateToTarget(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryAssignFocusedWorkbenchDismantleCandidateToTarget();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryAssignWorkbenchBlueprintItemToTarget(const FTunaSweeperItemSlotReference& SlotReference);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Workbench")
	bool TryAssignFocusedWorkbenchBlueprintItemToTarget();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Shop")
	bool TrySellSelectedShopItem();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD|Damage")
	void ShowDamageNumber(float DamageAmount, FVector WorldLocation, ETunaSweeperDamageNumberType DamageNumberType);

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudQuickSlotBarWidget* GetQuickSlotBarWidget() const { return QuickSlotBarWidget; }

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperHudBottomStatusWidget* GetBottomStatusWidget() const { return BottomStatusWidget; }

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual int32 NativePaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Bottom Status Direction Override"))
	ETunaSweeperHudTransitionEdge BottomStatusTransitionEdge = ETunaSweeperHudTransitionEdge::Bottom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Debuff Bar Direction Override"))
	ETunaSweeperHudTransitionEdge DebuffBarTransitionEdge = ETunaSweeperHudTransitionEdge::Bottom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Quick Slot Bar Direction Override"))
	ETunaSweeperHudTransitionEdge QuickSlotBarTransitionEdge = ETunaSweeperHudTransitionEdge::Bottom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Transitions", meta = (DisplayName = "Cursor Distance Direction Override"))
	ETunaSweeperHudTransitionEdge CursorDistanceTransitionEdge = ETunaSweeperHudTransitionEdge::Right;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ShotgunCrosshairRadius = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float ShotgunCrosshairThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "8", UIMin = "8"))
	int32 ShotgunCrosshairSegmentCount = 64;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair")
	FLinearColor ShotgunCrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.78f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairParenthesisOffset = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairParenthesisRadius = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairThickness = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairAimBarLength = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairAimBarStartDistance = 54.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "1.0", UIMin = "1.0"))
	float PrecisionCrosshairAimBarEndDistance = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float PrecisionCrosshairAimInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Crosshair")
	FLinearColor PrecisionCrosshairColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.82f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Extraction", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float ExtractionProgressTopOffset = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "TunaSweeper|HUD|Extraction")
	FVector2D ExtractionProgressWidgetSize = FVector2D(180.0f, 36.0f);

private:
	struct FDamageNumberPopup
	{
		TWeakObjectPtr<UTextBlock> TextWidget;
		FVector WorldLocation = FVector::ZeroVector;
		FVector2D ScreenDrift = FVector2D::ZeroVector;
		float ElapsedSeconds = 0.0f;
		float DurationSeconds = 0.75f;
		float RiseDistance = 48.0f;
		float PeakScale = 1.25f;
		float SettleScale = 1.0f;
		float FadeStartAlpha = 0.5f;
		ETunaSweeperDamageNumberType DamageNumberType = ETunaSweeperDamageNumberType::Normal;
	};

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
	void EnsureExtractionProgressWidget();
	void EnsureCursorDistanceWidget();
	void EnsureDebuffBarWidget();
	void EnsureInventoryQuickSlotPanelWidget();
	void EnsureHousingPanelWidget();
	void EnsureMapPanelWidget();
	void EnsureMemoPanelWidget();
	void EnsureQuestPanelWidgets();
	void EnsureShopSellPanelWidget();
	void SetShopSellPanelVisible(bool bVisible);
	ETunaSweeperHudTransitionEdge ResolveHudTransitionEdge(const UWidget* Widget, ETunaSweeperHudTransitionEdge DirectionOverride) const;
	void RefreshBottomStatusFromGameInstance();
	void RefreshDebuffBarFromPlayer();
	void RefreshQuickSlotsFromGameState();
	void RefreshInventoryQuickSlotPanel();
	void RefreshLocalizedTexts();
	void NormalizeCenterContentPanelLayout();
	void RefreshCancelableActionWidgets(const FGeometry* GeometryForPlacement = nullptr);
	void RefreshDialogueHudVisibility();
	void RefreshExtractionProgressWidget();
	void RefreshCursorDistanceWidget();
	void ForceCollapseHudWidget(UWidget* Widget);
	FVector2D GetHudTransitionHiddenTranslation(const UWidget* Widget, ETunaSweeperHudTransitionEdge Edge) const;
	bool HasActiveHudTransition(const UWidget* Widget) const;
	void SetTransitionedWidgetVisibility(UWidget* Widget, ESlateVisibility TargetVisibility, ETunaSweeperHudTransitionEdge DirectionOverride);
	void SetTransitionedWidgetVisibilityFromTranslation(UWidget* Widget, ESlateVisibility TargetVisibility, const FVector2D& HiddenTranslation);
	void TickHudTransitions(float InDeltaTime);
	void CacheAmmoCancelableActionWidgets();
	void BuildAmmoSelectorOptionTexts(TArray<FText>& OutOptionTexts, int32& OutFocusedIndex) const;
	void HandleSelectedInventoryItemChanged();
	void HandleQuestProgressChanged();
	void HandleHousingStateChanged();
	void HandleLanguageChanged();
	bool IsDialogueSequenceActive() const;
	bool IsHousingModeActive() const;
	bool IsGameplayBottomHudSuppressed() const;
	bool IsBunkerMap() const;
	FName GetSelectedWeaponTypeTag() const;
	bool IsWeaponCrosshairSuppressed() const;
	bool IsReloadGaugeReplacingCrosshair(const ATunaSweeperTopDownCharacter* TunaCharacter = nullptr) const;
	bool TryGetWeaponCrosshairLocalPosition(const FGeometry& AllottedGeometry, FVector2D& OutLocalPosition) const;
	void UpdateCenterCancelableActionGaugePlacement(const FGeometry& AllottedGeometry, bool bUseCrosshairPosition);
	void UpdateMouseCursorForReloadGauge(bool bShouldHideCursor);
	void UpdateCrosshairState(float InDeltaTime);
	void TickDamageNumberPopups(float InDeltaTime);
	void RemoveDamageNumberPopupAt(int32 PopupIndex);

	UFUNCTION()
	void HandleHudModeTabSelected(ETunaSweeperHudMode SelectedMode);

	UPROPERTY(Transient)
	ETunaSweeperHudMode ActiveHudMode = ETunaSweeperHudMode::None;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> CenterCancelableActionGaugeRoot;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperReloadRingWidget> CenterCancelableActionRingWidget;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UWidget> CenterReloadPromptRoot;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTextBlock> CenterCancelableActionPercentText;

	bool bReloadGaugeHidMouseCursor = false;
	bool bCenterCancelableActionGaugeSlotLayoutCached = false;
	FAnchors DefaultCenterCancelableActionGaugeAnchors;
	FVector2D DefaultCenterCancelableActionGaugeAlignment = FVector2D(0.5f, 0.5f);
	FVector2D DefaultCenterCancelableActionGaugePosition = FVector2D::ZeroVector;
	FVector2D DefaultCenterCancelableActionGaugeSize = FVector2D(96.0f, 96.0f);

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InventoryQuickSlotPanel;

	UPROPERTY(Transient)
	TObjectPtr<UHorizontalBox> InventoryQuickSlotRow;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> InventoryQuickSlotGuideText;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperExtractionProgressWidget> ExtractionProgressWidget;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CursorDistancePanel;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "TunaSweeper|HUD", meta = (BindWidgetOptional, AllowPrivateAccess = "true"))
	TObjectPtr<UTunaSweeperHudDebuffBarWidget> DebuffBarWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> CursorDistanceText;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UTunaSweeperItemThumbnailSlotWidget>> InventoryQuickSlotWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperHousingPanelWidget> HousingPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperMapWidget> MapPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperMemoWidget> MemoPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperQuestWidget> MenuQuestPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperQuestWidget> InteractionQuestPanelWidget;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperShopSellPanelWidget> ShopSellPanelWidget;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UBorder>> CenterCancelableActionSegments;

	bool bClearExternalPanelModeAfterHide = false;
	float ExtractionProgressCurrentSeconds = 0.0f;
	float ExtractionProgressRequiredSeconds = 4.0f;
	bool bExtractionProgressActive = false;
	float PrecisionCrosshairAimAlpha = 0.0f;
	int32 LastCursorDistanceMeters = INDEX_NONE;
	bool bDebuffBarHasActiveDebuffs = false;
	TArray<FDamageNumberPopup> DamageNumberPopups;
	TArray<FHudWidgetTransition> ActiveHudTransitions;
	TMap<TWeakObjectPtr<UWidget>, FWidgetTransform> HudTransitionBaseTransforms;
	TMap<TWeakObjectPtr<UWidget>, float> HudTransitionBaseOpacities;
	bool bQuestPanelOpenedFromInteraction = false;
};
