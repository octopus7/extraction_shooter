#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TunaSweeperKeyboardInputSubsystem.generated.h"

class APawn;
class IInputProcessor;

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

private:
	TSharedPtr<IInputProcessor> ScreenshotInputProcessor;
};
