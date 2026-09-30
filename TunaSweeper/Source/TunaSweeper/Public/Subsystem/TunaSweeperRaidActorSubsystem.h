#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperRaidActorSubsystem.generated.h"

UCLASS()
class TUNASWEEPER_API UTunaSweeperRaidActorSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;
    bool EnsureActorsSpawnedForWorld(UWorld* World);
    static FName GetPhysicalMapId(const UWorld* World);
    static FString GetCatalogObjectPath(FName PhysicalMapId);
private:
    bool bSpawned = false;
};