#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "CoastalGullProximitySubsystem.generated.h"

class ACoastalGull;

UCLASS()
class COASTALWILDLIFE_API UCoastalGullProximitySubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
public:
	void RegisterGull(ACoastalGull* Gull);
	void UnregisterGull(ACoastalGull* Gull);
	virtual void Deinitialize() override;

private:
	friend class FCoastalGullMotionTest;
	void Refresh();
	void ApplyPlayerLocations(const TArray<FVector>& Locations);

	TArray<TWeakObjectPtr<ACoastalGull>> Gulls;
	FTimerHandle RefreshTimer;
};
