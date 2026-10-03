#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaRocketDemoRange.generated.h"
class ATunaGuidedRocket;
class UStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

// Optional sample actor: one salvo toward a sideways-moving target plus a fuse-only lane.
UCLASS(Blueprintable)
class TUNAGUIDEDROCKET_API ATunaRocketDemoRange : public AActor
{
    GENERATED_BODY()
public:
    ATunaRocketDemoRange();
    virtual void Tick(float DeltaSeconds) override;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo") TSubclassOf<ATunaGuidedRocket> RocketClass;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo") TObjectPtr<UStaticMesh> TargetMesh;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Demo") TObjectPtr<UMaterialInterface> TargetMaterial;
protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
private:
    void Fire();
    UPROPERTY(Transient) TObjectPtr<AActor> Target;
    float Cycle = 0;
};
