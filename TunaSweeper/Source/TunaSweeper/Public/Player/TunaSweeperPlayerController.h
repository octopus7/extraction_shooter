#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Quest/TunaSweeperQuestTypes.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TimerManager.h"
#include "UI/TunaSweeperDialogueWidget.h"
#include "TunaSweeperPlayerController.generated.h"

class ACameraActor;
class UTunaSweeperGameHudWidget;
class UTunaSweeperIntroMenuWidget;
class UTunaSweeperQuestWidget;
class UTunaSweeperScenarioPresentationWidget;
class UTunaSweeperScreenFadeWidget;
class UInputAction;
class ATunaSweeperPickupItemActor;
struct FInputActionValue;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATunaSweeperPlayerController();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperGameHudWidget* GetGameHudWidget() const { return GameHudWidget; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ToggleInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ToggleMapPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void OpenLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void OpenStoragePanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void OpenShopPanel(int32 ShopId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	void OpenQuestPanel(FName QuestId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Memo")
	void OpenMemoPanel(int32 MemoId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Input")
	void ApplyDefaultGameInputMode();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Input")
	bool IsInventoryUiOpen() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool IsHousingPlacementActive() const;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Housing")
	bool IsHousingModeOpen() const;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool OpenHousingMode();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool StartHousingFacilityPlacement(FName FacilityId, FGuid ExistingInstanceId);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Housing")
	bool TryCommitHousingPlacement();

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|Dialogue")
	bool IsDialogueSequenceActive() const { return bDialogueSequenceActive; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Dialogue")
	bool StartDialogueSequence(const TArray<FTunaSweeperDialogueLine>& DialogueLines, FName CompletionFlag);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Quest")
	bool PlayQuestPresentation(FName QuestId, ETunaSweeperQuestPresentationTrigger Trigger);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Dialogue")
	void MoveDialogueCameraToFocusLocation(FVector FocusLocation, float BlendSeconds = 0.75f);

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Dialogue")
	void ReturnDialogueCameraToPlayer(float BlendSeconds = 0.9f);

	bool TryHandleHoveredItemInteract();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UTunaSweeperGameHudWidget> GameHudWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UTunaSweeperGameHudWidget> GameHudWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	TSoftClassPtr<UTunaSweeperIntroMenuWidget> IntroMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Intro")
	TObjectPtr<UTunaSweeperIntroMenuWidget> IntroMenuWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Scenario")
	TObjectPtr<UTunaSweeperScenarioPresentationWidget> ScenarioPresentationWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Scenario")
	TObjectPtr<UTunaSweeperScreenFadeWidget> ScreenFadeWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<UTunaSweeperDialogueWidget> DialogueWidget;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue")
	TObjectPtr<ACameraActor> DialogueCameraActor;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
	TSoftClassPtr<UTunaSweeperQuestWidget> QuestWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Quest")
	TObjectPtr<UTunaSweeperQuestWidget> QuestWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TSoftObjectPtr<UInputAction>> QuickSlotActions;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> MeleeQuickSlotAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TSoftObjectPtr<UInputAction> DropAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Loot")
	TSoftClassPtr<ATunaSweeperPickupItemActor> PickupItemActorClass;

private:
	void EnsureGameHudWidget();
	void EnsureIntroMenuWidget();
	void EnsureScenarioPresentationWidget();
	void BindHousingStateChanged();
	void HandleHousingStateChanged();
	void ApplyInitialTitleDisplaySettings();
	void ApplyLevelBgmState();
	bool ShowBunkerEntryFadeIfNeeded();
	void MaybeStartCanBotIntroDialogue();
	void BuildCanBotIntroDialogueLines(TArray<FTunaSweeperDialogueLine>& OutDialogueLines) const;
	void HandleDialogueLineActivated(const FTunaSweeperDialogueLine& DialogueLine);
	void HandleDialogueFinished();
	void FinishDialogueCameraReturn();
	void CancelPawnGameplayActions() const;
	bool IsIntroMap() const;
	bool IsOpeningScenarioMap() const;
	bool IsBunkerMap() const;
	bool IsRaidMap() const;
	bool GetMouseAimPointOnPlane(
		float PlaneZ,
		const FVector2D& ScreenOffset,
		FVector& OutAimPoint,
		FHitResult* OutAimHit = nullptr) const;
	bool FindDropLocationNearPlayer(FVector& OutDropLocation) const;
	ATunaSweeperPickupItemActor* SpawnDroppedPickupItem(int32 ItemId, int32 Quantity);
	void HandleQuickSlot(int32 SlotNumber);
	void HandleMeleeQuickSlotPressed();
	void HandleUseHoveredItem();
	void HandleToggleHoveredInventorySortLock();
	void HandleDrop(const FInputActionValue& Value);
	void HandleMeleeQuickSlot(const FInputActionValue& Value);
	void HandleQuickSlot1(const FInputActionValue& Value);
	void HandleQuickSlot2(const FInputActionValue& Value);
	void HandleQuickSlot3(const FInputActionValue& Value);
	void HandleQuickSlot4(const FInputActionValue& Value);
	void HandleQuickSlot5(const FInputActionValue& Value);
	void HandleQuickSlot6(const FInputActionValue& Value);
	void HandleQuickSlot7(const FInputActionValue& Value);
	void HandleQuickSlot8(const FInputActionValue& Value);
	void HandleHousingRotateLeft();
	void HandleHousingRotateRight();
	void HandleHousingCancel();
	void RestoreGameplayState(float HousingCameraBlendSeconds);
	void BeginHousingCameraMode();
	void EndHousingCameraMode(float BlendSeconds);
	void UpdateHousingCamera(float DeltaTime);
	void SetHousingCharacterVisualHidden(bool bShouldHide) const;
	FVector ResolveHousingCameraFocusLocation() const;
	FRotator ResolveHousingCameraRotation() const;
	FVector CalculateHousingCameraLocation(const FVector& FocusLocation, const FRotator& CameraRotation) const;
	void ClampHousingCameraFocusLocation(FVector& InOutFocusLocation) const;
	FVector2D GetHousingCameraMoveInput() const;
	void HandleHousingMoveForwardPressed();
	void HandleHousingMoveForwardReleased();
	void HandleHousingMoveBackwardPressed();
	void HandleHousingMoveBackwardReleased();
	void HandleHousingMoveRightPressed();
	void HandleHousingMoveRightReleased();
	void HandleHousingMoveLeftPressed();
	void HandleHousingMoveLeftReleased();

	FTimerHandle CanBotIntroDialogueTimerHandle;
	FTimerHandle DialogueCameraReturnTimerHandle;
	FName ActiveDialogueCompletionFlag;
	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> HousingCameraActor;
	FVector HousingCameraFocusLocation = FVector::ZeroVector;
	bool bDialogueSequenceActive = false;
	bool bDialogueCameraHasFocus = false;
	bool bHousingCameraActive = false;
	bool bHousingMoveForwardHeld = false;
	bool bHousingMoveBackwardHeld = false;
	bool bHousingMoveRightHeld = false;
	bool bHousingMoveLeftHeld = false;
};
