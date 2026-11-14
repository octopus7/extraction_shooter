#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "TunaSweeperMovementCheatSubsystem.generated.h"

// External process ABI. Activation is intentionally absent from reflected/save/config state.
struct alignas(8) FTunaSweeperMovementCheatControl
{
	uint64 Magic = 0x54534D4F56454348ULL;
	uint32 Version = 1;
	uint32 Size = 32;
	volatile uint32 EnableToken = 0;
	uint32 ProcessId = 0;
	uint64 Reserved = 0;
};

extern "C" DLLEXPORT FTunaSweeperMovementCheatControl TunaSweeperMovementCheatControl;

UCLASS()
class TUNASWEEPER_API UTunaSweeperMovementCheatSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool IsTickableWhenPaused() const override { return true; }
	virtual TStatId GetStatId() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
	bool IsEnabled() const;
	void CycleSpeed();
	float GetSpeedMultiplier() const;
	bool IsCombatSuppressed() const;
	static bool IsCombatSuppressedFor(const UObject* WorldContext);
	static FName GetCurrentMapId(const UObject* WorldContext);
	bool TryTeleportToMapPosition(FName ExpectedMapId, const FVector& WorldPosition);
private:
	int32 SpeedIndex = 0;
};
