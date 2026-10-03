#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunaRocketEffect.generated.h"
class UStaticMesh;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UStaticMeshComponent;

// Small standalone mesh particles; replace the class or use Niagara in the rocket BP.
UCLASS(Blueprintable)
class TUNAGUIDEDROCKET_API ATunaRocketEffect : public AActor
{
    GENERATED_BODY()
public:
    ATunaRocketEffect();
    virtual void Tick(float DeltaSeconds) override;
    void Initialize(bool bExplosion, float Duration, float Radius);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Effect") TObjectPtr<UStaticMesh> ParticleMesh;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Effect") TObjectPtr<UMaterialInterface> ParticleMaterial;
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Effect") FLinearColor TrailColor = FLinearColor(.25f, .35f, .45f);
    UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Effect") FLinearColor ExplosionColor = FLinearColor(1, .18f, .025f);
private:
    UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> Particles;
    UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> Material;
    float Age = 0, Life = .5f, Size = 100;
    bool bBurst = false;
};
