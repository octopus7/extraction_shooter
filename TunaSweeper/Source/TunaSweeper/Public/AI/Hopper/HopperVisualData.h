#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "HopperVisualData.generated.h"
class UStaticMesh;
class USkeletalMesh;
class UAnimSequence;
USTRUCT(BlueprintType)
struct FHopperMeshPart
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) FName PartName;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> Mesh;
};
USTRUCT(BlueprintType)
struct FHopperPartMotion
{
 GENERATED_BODY()
 UPROPERTY(EditAnywhere) FName PartName;
 UPROPERTY(EditAnywhere) TArray<FTransform> Frames;
};
/** Rest geometry is centimeters, +X forward, +Z up, at ground origin. */
UCLASS(BlueprintType)
class TUNASWEEPER_API UHopperVisualData : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere) TArray<FHopperMeshPart> Parts;
 UPROPERTY(EditAnywhere) TArray<FHopperPartMotion> HatchMotion;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<USkeletalMesh> PilotMesh;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UStaticMesh> PilotGun;
 UPROPERTY(EditAnywhere) FTransform GunTransform;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotBoarding;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotSeated;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotDisembark;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotIdle;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotWalk;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotFire;
 UPROPERTY(EditAnywhere) TSoftObjectPtr<UAnimSequence> PilotMelee;
 UPROPERTY(EditAnywhere) FVector SeatLocation = FVector(17,0,119.7806);
 UPROPERTY(EditAnywhere) FVector LeftArmMount = FVector(-5.8716,56.7287,157.1782);
 UPROPERTY(EditAnywhere) FVector RightArmMount = FVector(-5.8716,-56.7287,157.1782);
 UPROPERTY(EditAnywhere) FVector LeftHip = FVector(-4.5166,39.0687,85.8);
 UPROPERTY(EditAnywhere) FVector LeftKnee = FVector(-5.307,44.658,53.7965);
 UPROPERTY(EditAnywhere) FVector LeftAnkle = FVector(-.2032,44.2628,18.089);
 UPROPERTY(EditAnywhere) FVector RightHip = FVector(-4.5166,-39.0687,85.8);
 UPROPERTY(EditAnywhere) FVector RightKnee = FVector(-5.307,-44.658,53.7965);
 UPROPERTY(EditAnywhere) FVector RightAnkle = FVector(-.2032,-44.2628,18.089);
};
