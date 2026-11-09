#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaveDefenseSpawnPoint.generated.h"
UCLASS(Blueprintable)
class TUNASWEEPER_API AWaveDefenseSpawnPoint : public AActor
{
    GENERATED_BODY()
public:
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Wave Defense") FName LaneId;
};
