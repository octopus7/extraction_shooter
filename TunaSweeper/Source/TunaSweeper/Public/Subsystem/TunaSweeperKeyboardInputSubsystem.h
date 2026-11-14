#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperKeyboardInputSubsystem.generated.h"

class APawn;
struct FKey;
class IInputProcessor;
class UTunaSweeperMapWidget;

UCLASS()
class TUNASWEEPER_API UTunaSweeperKeyboardInputSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "TunaSweeper|Input")
	void ReceiveQuickSlotKeyInput(int32 SlotNumber, APawn* InstigatorPawn);

	bool CaptureScreenshotFromHotkey();
	bool HandleMovementCheatHotkey(const FKey& Key);
	void RegisterMovementCheatMap(UTunaSweeperMapWidget* Map);
	void UnregisterMovementCheatMap(UTunaSweeperMapWidget* Map);

private:
	TSharedPtr<IInputProcessor> ScreenshotInputProcessor;
	TWeakObjectPtr<UTunaSweeperMapWidget> MovementCheatMap;
};
