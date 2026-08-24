#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "InputCoreTypes.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Subsystem/TunaSweeperEnemySpawnSubsystem.h"
#include "TunaSweeperMapWidget.generated.h"

class UBackgroundBlur;
class UButton;
class UCanvasPanel;
class UImage;
class UTunaSweeperMapDefinition;
class UOverlay;
class USlider;
class UTextBlock;
class UTexture2D;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API UTunaSweeperMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Map")
	void RefreshMapView();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

private:
	void BuildMapWidget();
	void EnsureMapTextures();
	void EnsureFallbackMapTexture();
	void RefreshMapCanvas();
	void RefreshMapOverlayData();
	void AddMapOverlayToCanvas(const FTunaSweeperMapOverlayDefinition& MapOverlay);
	void RefreshMarkerIconButtons();
	void RefreshMarkerColorButtons();
	void SetMarkerIconIndex(int32 InMarkerIconIndex);
	void SetMarkerColorIndex(int32 InMarkerColorIndex);
	void SetMapZoom(float InMapZoom, const FVector2D* ZoomAnchorLocalPosition = nullptr);
	void ClampMapPan();
	bool TryCloseMapFromKey(const FKey& Key);
	void AddOrRemoveMarkerAtLocalPosition(const FVector2D& MapViewportLocalPosition);
	bool TryGetMapPositionFromLocal(const FVector2D& MapViewportLocalPosition, FVector2D& OutMapPosition) const;
	FVector2D MapPositionToLocal(const FVector2D& MapPosition) const;
	FVector2D MapPositionToTextureUV(const FVector2D& MapPosition) const;
	FVector2D GetMapBaseDrawSize() const;
	FVector2D GetMapScaledDrawSize() const;
	FVector2D GetMapDrawTopLeft() const;
	bool IsMouseInsideMapViewport(const FPointerEvent& InMouseEvent, FVector2D* OutMapViewportLocalPosition = nullptr) const;
	bool UpdatePlayerMapPosition();
	FVector2D ProjectWorldLocationToMapPosition(const FVector& WorldLocation) const;
	void HandleMapMarkersChanged();
	FText ResolveMapOverlayText(const FTunaSweeperMapOverlayDefinition& MapOverlay) const;
	FText GetMapOverlayIconGlyph(FName IconId) const;
	FLinearColor GetMapOverlayIconColor(FName IconId) const;
	FText GetMarkerGlyph(int32 MarkerIconIndex) const;
	FLinearColor GetMarkerColor(int32 MarkerColorIndex) const;
	void ConfigureChoiceButton(UButton* Button, const FLinearColor& FillColor, const FLinearColor& OutlineColor);

	UFUNCTION()
	void HandleZoomSliderChanged(float InValue);

	UFUNCTION()
	void HandleCircleMarkerIconClicked();

	UFUNCTION()
	void HandleDiamondMarkerIconClicked();

	UFUNCTION()
	void HandleTriangleMarkerIconClicked();

	UFUNCTION()
	void HandleAlertMarkerIconClicked();

	UFUNCTION()
	void HandleRedMarkerColorClicked();

	UFUNCTION()
	void HandleAmberMarkerColorClicked();

	UFUNCTION()
	void HandleGreenMarkerColorClicked();

	UFUNCTION()
	void HandleCyanMarkerColorClicked();

	UFUNCTION()
	void HandleVioletMarkerColorClicked();

	UFUNCTION()
	void HandleWhiteMarkerColorClicked();

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> RootOverlay;

	UPROPERTY(Transient)
	TObjectPtr<UBackgroundBlur> BackgroundBlur;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> MapCanvas;

	UPROPERTY(Transient)
	TObjectPtr<USlider> ZoomSlider;

	UPROPERTY(Transient)
	TObjectPtr<UImage> MapImage;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> MapTexture;

	UPROPERTY(Transient)
	TObjectPtr<UTunaSweeperMapDefinition> MapDefinition;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> PlayerIconTexture;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CircleMarkerIconButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> DiamondMarkerIconButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> TriangleMarkerIconButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AlertMarkerIconButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> RedMarkerColorButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> AmberMarkerColorButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> GreenMarkerColorButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> CyanMarkerColorButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> VioletMarkerColorButton;

	UPROPERTY(Transient)
	TObjectPtr<UButton> WhiteMarkerColorButton;

	TArray<FTunaSweeperMapMarkerSaveData> CachedMapMarkers;
	TArray<FTunaSweeperMapOverlayDefinition> CachedMapOverlays;
	FVector2D CachedPlayerMapPosition = FVector2D(0.5f, 0.5f);
	FVector2D LastMapViewportSize = FVector2D::ZeroVector;
	FVector2D MapPan = FVector2D::ZeroVector;
	FVector2D LastPanMouseLocalPosition = FVector2D::ZeroVector;
	FString ActiveMapTexturePath;
	float MapZoom = 1.0f;
	int32 SelectedMarkerIconIndex = 0;
	int32 SelectedMarkerColorIndex = 3;
	bool bIsPanningMap = false;
	bool bHasPlayerMapPosition = false;
	bool bIsUpdatingZoomSlider = false;
};
