#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperPlayerController.generated.h"

class UTunaSweeperGameHudWidget;
class UTunaSweeperIntroMenuWidget;
class UInputAction;
struct FInputActionValue;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATunaSweeperPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void SetupInputComponent() override;

	UFUNCTION(BlueprintPure, Category = "TunaSweeper|HUD")
	UTunaSweeperGameHudWidget* GetGameHudWidget() const { return GameHudWidget; }

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void ToggleInventoryOnlyPanel();

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|HUD")
	void OpenLootContainerPanel(const FTunaSweeperLootContainerInstance& ContainerInstance);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "HUD")
	TSoftClassPtr<UTunaSweeperGameHudWidget> GameHudWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UTunaSweeperGameHudWidget> GameHudWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Intro")
	TSoftClassPtr<UTunaSweeperIntroMenuWidget> IntroMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly, Category = "Intro")
	TObjectPtr<UTunaSweeperIntroMenuWidget> IntroMenuWidget;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TSoftObjectPtr<UInputAction>> QuickSlotActions;

private:
	void EnsureGameHudWidget();
	void EnsureIntroMenuWidget();
	bool IsIntroMap() const;
	bool GetMouseAimPointOnPlane(float PlaneZ, FVector& OutAimPoint) const;
	void HandleQuickSlot(int32 SlotNumber);
	void HandleQuickSlot1(const FInputActionValue& Value);
	void HandleQuickSlot2(const FInputActionValue& Value);
	void HandleQuickSlot3(const FInputActionValue& Value);
	void HandleQuickSlot4(const FInputActionValue& Value);
	void HandleQuickSlot5(const FInputActionValue& Value);
	void HandleQuickSlot6(const FInputActionValue& Value);
	void HandleQuickSlot7(const FInputActionValue& Value);
	void HandleQuickSlot8(const FInputActionValue& Value);
};
