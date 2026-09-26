#include "Component/TunaSweeperOutfitComponent.h"

#include "Animation/AnimInstance.h"
#include "Character/TunaSweeperOutfitCatalog.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"

UTunaSweeperOutfitComponent::UTunaSweeperOutfitComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTunaSweeperOutfitComponent::BeginPlay()
{
    Super::BeginPlay();
    CacheOriginalAppearance();
}

bool UTunaSweeperOutfitComponent::CacheOriginalAppearance()
{
    if (OriginalBodyMesh) return true;
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
    USkeletalMeshComponent* Body = Character ? Character->GetMesh() : nullptr;
    if (!Body || !Body->GetSkeletalMeshAsset()) return false;
    OriginalBodyMesh = Body->GetSkeletalMeshAsset();
    OriginalPhysicsAsset = Body->GetPhysicsAsset();
    OriginalPostProcessClass = OriginalBodyMesh->GetPostProcessAnimBlueprint();
    if (Body->GetPostProcessInstance()) OriginalPostProcessClass = Body->GetPostProcessInstance()->GetClass();
    for (UMaterialInterface* Material : Body->GetMaterials()) OriginalMaterials.Add(Material);
    TArray<USkeletalMeshComponent*> Meshes;
    Character->GetComponents(Meshes);
    for (auto* Mesh : Meshes)
    {
        if (Mesh && Mesh != Body && Mesh->GetName().StartsWith(TEXT("Skirt")))
        {
            OriginalSkirts.Add({Mesh, Mesh->IsVisible(), bool(Mesh->bHiddenInGame), Mesh->IsComponentTickEnabled()});
        }
    }
    return true;
}

bool UTunaSweeperOutfitComponent::IsCompatibleMesh(const USkeletalMesh* Mesh) const
{
    if (!Mesh || !OriginalBodyMesh || Mesh->GetSkeleton() != OriginalBodyMesh->GetSkeleton()) return false;
    const FReferenceSkeleton& Original = OriginalBodyMesh->GetRefSkeleton();
    const FReferenceSkeleton& Candidate = Mesh->GetRefSkeleton();
    if (Original.GetRawBoneNum() != Candidate.GetRawBoneNum()) return false;
    for (int32 Index = 0; Index < Original.GetRawBoneNum(); ++Index)
    {
        if (Original.GetBoneName(Index) != Candidate.GetBoneName(Index) ||
            Original.GetParentIndex(Index) != Candidate.GetParentIndex(Index)) return false;
    }
    return true;
}

bool UTunaSweeperOutfitComponent::EnsureClothingComponent()
{
    if (ClothingMesh) return true;
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
    if (!Character || !GetWorld()) return false;
    ClothingMesh = NewObject<USkeletalMeshComponent>(Character, TEXT("OutfitClothing"));
    if (!ClothingMesh) return false;
    Character->AddInstanceComponent(ClothingMesh);
    ClothingMesh->SetupAttachment(Character->GetMesh());
    ClothingMesh->SetRelativeTransform(FTransform::Identity);
    ClothingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ClothingMesh->SetGenerateOverlapEvents(false);
    ClothingMesh->SetCanEverAffectNavigation(false);
    // Bunny ears extend above the body bounds, so use the garment's own bounds.
    ClothingMesh->bUseBoundsFromLeaderPoseComponent = false;
    ClothingMesh->RegisterComponent();
    return true;
}

bool UTunaSweeperOutfitComponent::ApplyOutfit(const FTunaSweeperOutfitDefinition& Definition)
{
    if (!TunaSweeperOutfits::IsSupportedOutfitId(Definition.OutfitId) || !CacheOriginalAppearance()) return false;
    const bool bMaid = Definition.OutfitId == FName(TEXT("Maid"));
    USkeletalMesh* NewBody = bMaid ? OriginalBodyMesh.Get() : Definition.BodyMesh.LoadSynchronous();
    USkeletalMesh* NewClothing = bMaid ? nullptr : Definition.ClothingMesh.LoadSynchronous();
    // Complete all fallible work before changing the visible character.
    if (!IsCompatibleMesh(NewBody) || (!bMaid && (!IsCompatibleMesh(NewClothing) || !EnsureClothingComponent()))) return false;
    auto* Body = CastChecked<ATunaSweeperTopDownCharacter>(GetOwner())->GetMesh();
    Body->SetOverridePostProcessAnimBP(OriginalPostProcessClass, false);
    Body->SetSkeletalMesh(NewBody, false);
    Body->SetPhysicsAsset(OriginalPhysicsAsset, false);
    Body->EmptyOverrideMaterials();
    if (bMaid)
    {
        for (int32 Index = 0; Index < OriginalMaterials.Num(); ++Index) Body->SetMaterial(Index, OriginalMaterials[Index]);
    }
    if (ClothingMesh)
    {
        ClothingMesh->SetSkeletalMesh(NewClothing, false);
        ClothingMesh->SetLeaderPoseComponent(Body, true, false);
    }
    AppliedOutfitId = Definition.OutfitId;
    RefreshVisibility(bHousingVisualHidden);
    return true;
}

void UTunaSweeperOutfitComponent::RefreshVisibility(bool bHousingHidden)
{
    bHousingVisualHidden = bHousingHidden;
    const bool bMaid = AppliedOutfitId == FName(TEXT("Maid"));
    for (const FOriginalSkirt& Original : OriginalSkirts)
    {
        if (auto* Skirt = Original.Mesh.Get())
        {
            Skirt->SetVisibility(bMaid && Original.bVisible && !bHousingHidden, false);
            Skirt->SetHiddenInGame(!bMaid || Original.bHiddenInGame || bHousingHidden, false);
            Skirt->SetComponentTickEnabled(bMaid && Original.bTickEnabled);
        }
    }
    if (ClothingMesh)
    {
        ClothingMesh->SetVisibility(!bMaid && !bHousingHidden, false);
        ClothingMesh->SetHiddenInGame(bMaid || bHousingHidden, false);
    }
}

void UTunaSweeperOutfitComponent::RestoreSelectedOutfit()
{
    auto* Character = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
    const auto* Controller = Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
    if (!Controller || !Controller->IsLocalController() || !CacheOriginalAppearance()) return;
    auto* Game = Character->GetGameInstance<UTunaSweeperGameInstance>();
    if (!Game) return;
    const FName Selected = Game->GetSelectedOutfitId();
    auto* Catalog = Game->GetOutfitCatalog();
    const auto* Definition = Catalog ? Catalog->FindOutfit(Selected) : nullptr;
    if (Definition && Game->IsOutfitUnlocked(Selected) && ApplyOutfit(*Definition)) return;
    FTunaSweeperOutfitDefinition Maid;
    Maid.OutfitId = TEXT("Maid");
    if (ApplyOutfit(Maid)) Game->NotifyOutfitRestoreFallback(Selected);
}
