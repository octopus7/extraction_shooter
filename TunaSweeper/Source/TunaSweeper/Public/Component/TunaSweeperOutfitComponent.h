#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TunaSweeperOutfitComponent.generated.h"

class UAnimInstance;
class UMaterialInterface;
class UPhysicsAsset;
class USkeletalMesh;
class USkeletalMeshComponent;
struct FTunaSweeperOutfitDefinition;

/** Owns appearance only; gameplay animation, face expressions and physics stay on the original components. */
UCLASS(ClassGroup = (TunaSweeper), meta = (BlueprintSpawnableComponent))
class TUNASWEEPER_API UTunaSweeperOutfitComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UTunaSweeperOutfitComponent();
    bool ApplyOutfit(const FTunaSweeperOutfitDefinition& Definition);
    void RestoreSelectedOutfit();
    void RefreshVisibility(bool bHousingHidden);
    USkeletalMeshComponent* GetClothingMesh() const { return ClothingMesh; }
    FName GetAppliedOutfitId() const { return AppliedOutfitId; }

protected:
    virtual void BeginPlay() override;

private:
    bool CacheOriginalAppearance();
    bool IsCompatibleMesh(const USkeletalMesh* Mesh) const;
    bool EnsureClothingComponent();

    UPROPERTY(Transient)
    TObjectPtr<USkeletalMesh> OriginalBodyMesh;
    UPROPERTY(Transient)
    TObjectPtr<UPhysicsAsset> OriginalPhysicsAsset;
    UPROPERTY(Transient)
    TSubclassOf<UAnimInstance> OriginalPostProcessClass;
    UPROPERTY(Transient)
    TArray<TObjectPtr<UMaterialInterface>> OriginalMaterials;
    UPROPERTY(Transient)
    TObjectPtr<USkeletalMeshComponent> ClothingMesh;

    struct FOriginalSkirt
    {
        TWeakObjectPtr<USkeletalMeshComponent> Mesh;
        bool bVisible = true;
        bool bHiddenInGame = false;
        bool bTickEnabled = true;
    };
    TArray<FOriginalSkirt> OriginalSkirts;
    FName AppliedOutfitId = TEXT("Maid");
    bool bHousingVisualHidden = false;
};
