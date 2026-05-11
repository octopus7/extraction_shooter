#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "TunaSweeperPlayerController.generated.h"

class UTunaSweeperGameHudWidget;

UCLASS(BlueprintType, Blueprintable)
class TUNASWEEPER_API ATunaSweeperPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ATunaSweeperPlayerController();

	virtual void BeginPlay() override;
	virtual void PlayerTick(float DeltaTime) override;

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

private:
	void EnsureGameHudWidget();
	bool GetMouseAimPointOnPlane(float PlaneZ, FVector& OutAimPoint) const;
};
