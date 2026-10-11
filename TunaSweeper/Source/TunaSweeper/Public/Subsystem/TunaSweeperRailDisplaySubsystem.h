#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TunaSweeperRailDisplaySubsystem.generated.h"

// Keeps the reusable railway plugin independent of project localization data.
UCLASS()
class TUNASWEEPER_API UTunaSweeperRailDisplaySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
private:
    void ConfigureActor(AActor* Actor);
    FDelegateHandle SpawnHandle;
};
