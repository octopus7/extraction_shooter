#pragma once

#include "Subsystems/WorldSubsystem.h"
#include "TimerManager.h"
#include "TunaSweeperSupplyDropSubsystem.generated.h"

class ATunaSweeperSupplyDropActor;

UCLASS()
class TUNASWEEPER_API UTunaSweeperSupplyDropSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	const ATunaSweeperSupplyDropActor* GetDescendingDrop() const;

protected:
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void OnWorldEndPlay(UWorld& InWorld) override;

private:
	void TrySpawnDrop();

	TWeakObjectPtr<ATunaSweeperSupplyDropActor> ActiveDrop;
	FTimerHandle SpawnTimer;
	int32 ContainerDefinitionId = INDEX_NONE;
	int32 ContentsId = INDEX_NONE;
	bool bRolledForWorld = false;
};
