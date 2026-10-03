#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TunaRocketConfig.generated.h"

USTRUCT(BlueprintType)
struct TUNAGUIDEDROCKET_API FTunaRocketSettings
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight", meta=(ClampMin="0", Units="cm/s")) float Speed = 1200;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Flight", meta=(ClampMin="0", Units="deg/s")) float TurnRate = 30;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance", meta=(ClampMin="0", Units="s")) float GuidanceDelay = .15f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance", meta=(ClampMin="0", Units="s")) float GuidanceDuration = 1.1f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Guidance", meta=(ClampMin="0", ClampMax="89", Units="deg")) float GuidanceConeHalfAngle = 65;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fuse", meta=(ClampMin="0.05", ClampMax="30", Units="s")) float Lifetime = 3;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Collision", meta=(ClampMin="0.1", Units="cm")) float CollisionRadius = 6;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion", meta=(ClampMin="0")) float Damage = 0;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Explosion", meta=(ClampMin="0", Units="cm")) float DamageRadius = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.03", Units="s")) float TrailInterval = .08f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.01", Units="s")) float TrailLifetime = .45f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.01", Units="s")) float ExplosionLifetime = .55f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.1", Units="cm")) float TrailSize = 9;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Effects", meta=(ClampMin="0.1", Units="cm")) float ExplosionVisualRadius = 100;
    void Normalize();
};

UCLASS(BlueprintType)
class TUNAGUIDEDROCKET_API UTunaRocketConfig : public UDataAsset
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Rocket") FTunaRocketSettings Settings;
};
