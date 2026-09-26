#if WITH_DEV_AUTOMATION_TESTS
#include "Character/TunaSweeperOutfitCatalog.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperOutfitComponent.h"
#include "Component/TunaSweeperScratchComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/TunaSweeperSaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace TunaSweeperOutfitTests
{
constexpr auto Flags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
const FName ExpectedOutfitIds[] = {TEXT("Maid"), TEXT("SchoolUniform"), TEXT("MechanicOutfit"),
    TEXT("Sportswear"), TEXT("BunnyPajamas"), TEXT("AdventurerOutfit"), TEXT("Raincoat")};
struct FContextAccess : UGameInstance
{
    static void Attach(UGameInstance* Instance, FWorldContext* Context)
    {
        auto Member = &FContextAccess::WorldContext;
        Instance->*Member = Context;
    }
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperOutfitSaveTest,
    "TunaSweeper.Outfits.SaveAndFallback", TunaSweeperOutfitTests::Flags)
bool FTunaSweeperOutfitSaveTest::RunTest(const FString&)
{
    // Catches a missing serialized field and unknown/legacy IDs leaking into runtime selection.
    auto* Save = NewObject<UTunaSweeperSaveGame>();
    TestEqual(TEXT("Legacy/new saves start in the maid outfit"), Save->SelectedOutfitId, FName(TEXT("Maid")));
    for (FName Id : TunaSweeperOutfitTests::ExpectedOutfitIds)
    {
        TestTrue(*FString::Printf(TEXT("%s is supported"), *Id.ToString()), TunaSweeperOutfits::IsSupportedOutfitId(Id));
        Save->SelectedOutfitId = Id;
        TArray<uint8> Bytes;
        if (!TestTrue(TEXT("Outfit serializes"), UGameplayStatics::SaveGameToMemory(Save, Bytes))) return false;
        auto* Loaded = Cast<UTunaSweeperSaveGame>(UGameplayStatics::LoadGameFromMemory(Bytes));
        if (!TestNotNull(TEXT("Saved outfit deserializes"), Loaded)) return false;
        TestEqual(*FString::Printf(TEXT("%s survives serialization and restore validation"), *Id.ToString()),
            TunaSweeperOutfits::SanitizePersistedOutfitId(Loaded->SelectedOutfitId), Id);
    }
    TestEqual(TEXT("Missing legacy ID becomes maid"), TunaSweeperOutfits::SanitizePersistedOutfitId(NAME_None), FName(TEXT("Maid")));
    TestEqual(TEXT("Retired/unknown ID becomes maid"), TunaSweeperOutfits::SanitizePersistedOutfitId(TEXT("RemovedOutfit")), FName(TEXT("Maid")));

    auto* Game = NewObject<UTunaSweeperGameInstance>();
    Game->bInventoryStateInitialized = true;
    Game->bOutfitUnlocksLoaded = true;
    Game->SelectedOutfitId = TEXT("Raincoat");
    int32 Notifications = 0;
    Game->OnOutfitChanged.AddLambda([&Notifications] { ++Notifications; });
    TestFalse(TEXT("An explicit unknown selection is rejected"), Game->TryEquipOutfit(TEXT("RemovedOutfit")));
    TestEqual(TEXT("Rejected request does not change selection"), Game->GetSelectedOutfitId(), FName(TEXT("Raincoat")));
    TestEqual(TEXT("Rejected request does not notify listeners"), Notifications, 0);
    Game->ResetRuntimeStateForSaveSlotSelection();
    Game->bInventoryStateInitialized = true;
    TestEqual(TEXT("Changing slots clears the previous outfit"), Game->GetSelectedOutfitId(), FName(TEXT("Maid")));
    Game->OnOutfitChanged.Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperOutfitRuntimeTest,
    "TunaSweeper.Outfits.RealPlayerSwitchAndRestore", TunaSweeperOutfitTests::Flags)
bool FTunaSweeperOutfitRuntimeTest::RunTest(const FString&)
{
    // Catches reset animation/face, mismatched meshes, resurrected maid skirt and incomplete maid restoration.
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("Transient world"), World)) return false;
    GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
    ON_SCOPE_EXIT { World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot(); };
    UClass* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
        TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    if (!TestNotNull(TEXT("Actual player blueprint"), PlayerClass)) return false;
    auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass);
    auto* Outfit = Player ? Player->GetOutfitComponent() : nullptr;
    if (!TestNotNull(TEXT("Player owns outfit component"), Outfit)) return false;
    auto* Catalog = LoadObject<UTunaSweeperOutfitCatalog>(nullptr,
        TEXT("/Game/Characters/Player/LunaMk2/Outfits/DA_LunaMk2_Outfits.DA_LunaMk2_Outfits"));
    if (!TestNotNull(TEXT("Authored seven-outfit catalog"), Catalog)) return false;
    auto* Body = Player->GetMesh();
    auto* OriginalBody = Body->GetSkeletalMeshAsset();
    auto* OriginalAnimClass = Body->GetAnimClass();
    auto* OriginalPhysics = Body->GetPhysicsAsset();
    const auto OriginalMaterials = Body->GetMaterials();
    TArray<USkeletalMeshComponent*> Meshes;
    Player->GetComponents(Meshes);
    USkeletalMeshComponent* Face = nullptr;
    TArray<USkeletalMeshComponent*> Skirts;
    for (auto* Mesh : Meshes)
    {
        if (Mesh->GetName().Contains(TEXT("Face"))) Face = Mesh;
        if (Mesh->GetName().StartsWith(TEXT("Skirt"))) Skirts.Add(Mesh);
    }
    if (!TestNotNull(TEXT("Existing face component"), Face)) return false;
    auto* FaceAsset = Face->GetSkeletalMeshAsset();
    auto* FaceParent = Face->GetAttachParent();
    auto* FaceLeader = Face->LeaderPoseComponent.Get();
    Face->SetMorphTarget(TEXT("Smile"), 0.65f);
    const auto* Maid = Catalog->FindOutfit(TEXT("Maid"));
    if (!TestNotNull(TEXT("Catalog contains maid"), Maid)) return false;
    for (FName Id : TunaSweeperOutfitTests::ExpectedOutfitIds)
    {
        if (Id == TEXT("Maid")) continue;
        const auto* Definition = Catalog->FindOutfit(Id);
        if (!TestNotNull(*Id.ToString(), Definition)) continue;
        if (!TestTrue(*FString::Printf(TEXT("%s applies"), *Id.ToString()), Outfit->ApplyOutfit(*Definition))) continue;
        TestEqual(TEXT("Main mesh component is unchanged"), Player->GetMesh(), Body);
        TestEqual(TEXT("Locomotion/weapon animation class is preserved"), Body->GetAnimClass(), OriginalAnimClass);
        TestEqual(TEXT("Original ragdoll physics is preserved"), Body->GetPhysicsAsset(), OriginalPhysics);
        TestEqual(TEXT("Face asset is never swapped"), Face->GetSkeletalMeshAsset(), FaceAsset);
        TestEqual(TEXT("Face attachment is untouched"), Face->GetAttachParent(), FaceParent);
        TestEqual(TEXT("Face leader setup is untouched"), Face->LeaderPoseComponent.Get(), FaceLeader);
        TestEqual(TEXT("Face expression survives"), Face->GetMorphTarget(TEXT("Smile")), 0.65f);
        TestEqual(TEXT("Clothing follows the actual body"), Outfit->GetClothingMesh()->LeaderPoseComponent.Get(), static_cast<USkinnedMeshComponent*>(Body));
        TestTrue(TEXT("Clothing has identity body-relative transform"), Outfit->GetClothingMesh()->GetRelativeTransform().Equals(FTransform::Identity));
        Player->SetHousingModeVisualHidden(true);
        Player->SetHousingModeVisualHidden(false);
        for (auto* Skirt : Skirts) TestFalse(TEXT("Housing close cannot resurrect the maid skirt"), Skirt->IsVisible());
        FTunaSweeperOutfitDefinition Broken = *Definition;
        Broken.ClothingMesh.Reset();
        auto* AppliedBody = Body->GetSkeletalMeshAsset();
        TestFalse(TEXT("Missing clothing rejects the complete change"), Outfit->ApplyOutfit(Broken));
        TestEqual(TEXT("Failed change preserves the current body"), Body->GetSkeletalMeshAsset(), AppliedBody);
        // Leader-pose followers have no local bone array. Their roll ghosts must copy the leader's actual pose.
        auto* Pose = LoadObject<UAnimSequence>(nullptr,
            TEXT("/Game/Characters/Player/LunaMk2/Animations/Title/AS_LunaMk2_Title_B.AS_LunaMk2_Title_B"));
        if (TestNotNull(TEXT("Non-reference pose for afterimage regression"), Pose))
        {
            Body->PlayAnimation(Pose, false);
            Body->SetPosition(1.4f, false);
            Body->TickAnimation(0.0f, false);
            Body->RefreshBoneTransforms();
            auto* Scratch = Player->GetScratchComponent();
            Scratch->SpawnAfterimage(1.0);
            int32 MatchedMeshes = 0;
            for (const auto& Afterimage : Scratch->ActiveAfterimages)
            {
                auto* Ghost = Afterimage.Mesh.Get();
                if (!Ghost || (Ghost->GetSkinnedAsset() != Body->GetSkinnedAsset() &&
                    Ghost->GetSkinnedAsset() != Outfit->GetClothingMesh()->GetSkinnedAsset())) continue;
                ++MatchedMeshes;
                Ghost->RefreshBoneTransforms();
                for (FName Bone : {FName(TEXT("hand_l")), FName(TEXT("calf_r")), FName(TEXT("pelvis"))})
                    TestTrue(TEXT("Body and clothing ghosts copy the current animated pose"),
                        Ghost->GetBoneTransformByName(Bone, EBoneSpaces::ComponentSpace).Equals(
                            Body->GetSocketTransform(Bone, RTS_Component), 0.01f));
            }
            TestEqual(TEXT("Masked body and garment both produce a ghost"), MatchedMeshes, 2);
            Scratch->DestroyAllAfterimages();
            Body->SetAnimInstanceClass(OriginalAnimClass);
        }
        TestTrue(TEXT("Maid can be restored after every outfit"), Outfit->ApplyOutfit(*Maid));
        TestEqual(TEXT("Original maid body returns"), Body->GetSkeletalMeshAsset(), OriginalBody);
        TestFalse(TEXT("Maid hides additional clothing"), Outfit->GetClothingMesh()->IsVisible());
        TestEqual(TEXT("Maid material count returns"), Body->GetNumMaterials(), OriginalMaterials.Num());
        for (int32 Index = 0; Index < OriginalMaterials.Num(); ++Index)
            TestEqual(TEXT("Original maid material returns"), Body->GetMaterial(Index), OriginalMaterials[Index]);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperOutfitSaveFailureTest,
    "TunaSweeper.Outfits.SaveFailureRollback", TunaSweeperOutfitTests::Flags)
bool FTunaSweeperOutfitSaveFailureTest::RunTest(const FString&)
{
    // Catches a changed appearance or selection leaking out after a failed save.
    using namespace TunaSweeperOutfitTests;
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    FWorldContext& Context = GEngine->CreateNewWorldContext(EWorldType::Game);
    Context.SetCurrentWorld(World);
    TStrongObjectPtr<UTunaSweeperGameInstance> Game(NewObject<UTunaSweeperGameInstance>(GEngine));
    FContextAccess::Attach(Game.Get(), &Context);
    Context.OwningGameInstance = Game.Get();
    World->SetGameInstance(Game.Get());
    Game->bInventoryStateInitialized = true;
    Game->RetiredDemoSaveSlotIndex = Game->ActiveSaveSlotIndex; // Refuses writes before touching real save files.
    Game->OutfitUnlockSavePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
        TEXT("OutfitRollback_") + FGuid::NewGuid().ToString(EGuidFormats::Digits) + TEXT(".sav"));
    ON_SCOPE_EXIT
    {
        World->SetGameInstance(nullptr); Context.OwningGameInstance = nullptr;
        FContextAccess::Attach(Game.Get(), nullptr);
        World->DestroyWorld(false); GEngine->DestroyWorldContext(World); World->RemoveFromRoot();
    };
    auto* Controller = World->SpawnActor<APlayerController>();
    auto* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
    Game->AddLocalPlayer(LocalPlayer, FPlatformUserId::CreateFromInternalId(0));
    Controller->SetPlayer(LocalPlayer);
    auto* PlayerClass = LoadClass<ATunaSweeperTopDownCharacter>(nullptr,
        TEXT("/Game/Characters/Player/BP_TunaSweeperPlayerCharacter.BP_TunaSweeperPlayerCharacter_C"));
    if (!TestNotNull(TEXT("Actual player blueprint"), PlayerClass)) return false;
    auto* Player = World->SpawnActor<ATunaSweeperTopDownCharacter>(PlayerClass);
    Controller->Possess(Player);
    auto* OriginalBody = Player->GetMesh()->GetSkeletalMeshAsset();
    const auto* Definition = Game->GetOutfitCatalog() ? Game->GetOutfitCatalog()->FindOutfit(TEXT("Sportswear")) : nullptr;
    if (!TestNotNull(TEXT("Real sportswear definition"), Definition)) return false;
    TestTrue(TEXT("Requested assets are usable before save failure"), Player->GetOutfitComponent()->ApplyOutfit(*Definition));
    const auto* Maid = Game->GetOutfitCatalog()->FindOutfit(TEXT("Maid"));
    if (!Maid || !Player->GetOutfitComponent()->ApplyOutfit(*Maid)) return false;
    // A previously selected outfit can become locked when the temporary override is disabled.
    // The same-ID path would otherwise return success without reaching the failing-save guard.
    Game->bUnlockAllOutfitsOverride = false;
    Game->SelectedOutfitId = TEXT("Sportswear");
    TestFalse(TEXT("Even a previously selected locked outfit cannot be equipped"), Game->TryEquipOutfit(TEXT("Sportswear")));
    TestEqual(TEXT("Locked equip leaves the visible maid unchanged"), Player->GetMesh()->GetSkeletalMeshAsset(), OriginalBody);
    Player->GetOutfitComponent()->RestoreSelectedOutfit();
    TestEqual(TEXT("Disabling override restores a consistent selected ID"), Game->GetSelectedOutfitId(), FName(TEXT("Maid")));
    TestEqual(TEXT("Disabling override restores the actual maid body"), Player->GetMesh()->GetSkeletalMeshAsset(), OriginalBody);
    Game->SelectedOutfitId = TEXT("Maid");
    Game->bUnlockAllOutfitsOverride = true;
    int32 Notifications = 0;
    Game->OnOutfitChanged.AddLambda([&Notifications] { ++Notifications; });
    TestFalse(TEXT("Failed save rejects equip"), Game->TryEquipOutfit(TEXT("Sportswear")));
    TestEqual(TEXT("Failed save restores selected ID"), Game->GetSelectedOutfitId(), FName(TEXT("Maid")));
    TestEqual(TEXT("Failed save restores the actual appearance"), Player->GetMesh()->GetSkeletalMeshAsset(), OriginalBody);
    TestEqual(TEXT("Failed save never publishes the temporary selection"), Notifications, 0);

    for (FName TargetId : {FName(TEXT("BunnyPajamas")), FName(TEXT("Raincoat"))})
    {
        const auto* Target = Game->GetOutfitCatalog()->FindOutfit(TargetId);
        if (!TestNotNull(*TargetId.ToString(), Target)) return false;
        if (!TestTrue(TEXT("Target assets are usable before save failure"), Player->GetOutfitComponent()->ApplyOutfit(*Target))) return false;
        if (!TestTrue(TEXT("Sportswear baseline applies before save failure"), Player->GetOutfitComponent()->ApplyOutfit(*Definition))) return false;
        Game->SelectedOutfitId = TEXT("Sportswear");
        auto* SportswearBody = Player->GetMesh()->GetSkeletalMeshAsset();
        auto* SportswearClothing = Player->GetOutfitComponent()->GetClothingMesh()->GetSkeletalMeshAsset();
        TestFalse(*FString::Printf(TEXT("Failed save rejects %s equip"), *TargetId.ToString()), Game->TryEquipOutfit(TargetId));
        TestEqual(TEXT("Failed save restores the previous non-maid selection"), Game->GetSelectedOutfitId(), FName(TEXT("Sportswear")));
        TestEqual(TEXT("Failed save restores the previous non-maid body"), Player->GetMesh()->GetSkeletalMeshAsset(), SportswearBody);
        TestEqual(TEXT("Failed save restores the previous non-maid clothing"), Player->GetOutfitComponent()->GetClothingMesh()->GetSkeletalMeshAsset(), SportswearClothing);
        TestEqual(TEXT("Failed save restores the component's applied outfit"), Player->GetOutfitComponent()->GetAppliedOutfitId(), FName(TEXT("Sportswear")));
        TestEqual(TEXT("Failed non-maid save never publishes the temporary selection"), Notifications, 0);
    }

    Game->SelectedOutfitId = TEXT("Raincoat");
    TestTrue(TEXT("Restoring selected raincoat succeeds without changing the saved choice"), Game->TryEquipOutfit(TEXT("Raincoat")));
    TestEqual(TEXT("Raincoat equip changes the actual component"), Player->GetOutfitComponent()->GetAppliedOutfitId(), FName(TEXT("Raincoat")));
    TestEqual(TEXT("Restoring the saved raincoat does not publish a new selection"), Notifications, 0);

    Game->SelectedOutfitId = TEXT("Sportswear");
    auto* InvalidCatalog = NewObject<UTunaSweeperOutfitCatalog>();
    InvalidCatalog->Outfits.Add(*Maid);
    FTunaSweeperOutfitDefinition MissingAsset = *Definition;
    MissingAsset.BodyMesh.Reset();
    InvalidCatalog->Outfits.Add(MissingAsset);
    Game->OutfitCatalog = InvalidCatalog;
    Player->GetOutfitComponent()->RestoreSelectedOutfit();
    TestEqual(TEXT("Unavailable restore asset also normalizes the selected ID"), Game->GetSelectedOutfitId(), FName(TEXT("Maid")));
    TestEqual(TEXT("Unavailable restore asset keeps the visible maid"), Player->GetMesh()->GetSkeletalMeshAsset(), OriginalBody);
    Game->OnOutfitChanged.Clear();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperOutfitUnlockTest,
    "TunaSweeper.Outfits.GlobalUnlockPersistence", TunaSweeperOutfitTests::Flags)
bool FTunaSweeperOutfitUnlockTest::RunTest(const FString&)
{
    // Catches the temporary all-unlocked switch leaking into permanent data, and slot resets erasing unlocks.
    const FString Directory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Automation"),
        TEXT("OutfitUnlocks_") + FGuid::NewGuid().ToString(EGuidFormats::Digits));
    IFileManager::Get().MakeDirectory(*Directory, true);
    ON_SCOPE_EXIT { IFileManager::Get().DeleteDirectory(*Directory, false, true); };
    const FString SavePath = FPaths::Combine(Directory, TEXT("CosmeticUnlocks.sav"));
    auto* Game = NewObject<UTunaSweeperGameInstance>();
    Game->OutfitUnlockSavePath = SavePath;
    Game->bUnlockAllOutfitsOverride = false;
    Game->bInventoryStateInitialized = true;
    Game->SelectedOutfitId = TEXT("Sportswear");
    TestEqual(TEXT("A selection saved under the override falls back when locked"), Game->GetSelectedOutfitId(), FName(TEXT("Maid")));
    TestTrue(TEXT("Maid is always unlocked"), Game->IsOutfitUnlocked(TEXT("Maid")));
    TestFalse(TEXT("Locked outfit stays locked with override disabled"), Game->IsOutfitUnlocked(TEXT("Sportswear")));
    TestFalse(TEXT("Unknown ID is never unlocked"), Game->IsOutfitUnlocked(TEXT("Unknown")));
    Game->bUnlockAllOutfitsOverride = true;
    TestTrue(TEXT("Temporary override exposes existing outfits"), Game->IsOutfitUnlocked(TEXT("Sportswear")));
    TestFalse(TEXT("Override does not write permanent unlocks"), FPaths::FileExists(SavePath));
    Game->bUnlockAllOutfitsOverride = false;
    int32 Notifications = 0;
    Game->OnOutfitUnlocksChanged.AddLambda([&Notifications] { ++Notifications; });
    TestTrue(TEXT("Unlock is committed to the independent global file"), Game->TryUnlockOutfit(TEXT("Sportswear")));
    TestTrue(TEXT("Committed outfit is available"), Game->IsOutfitUnlocked(TEXT("Sportswear")));
    TestNotEqual(TEXT("Newly unlocked outfit is not falsely marked already equipped"), Game->GetSelectedOutfitId(), FName(TEXT("Sportswear")));
    TestTrue(TEXT("Global unlock file exists"), FPaths::FileExists(SavePath));
    TestEqual(TEXT("Successful unlock notifies once"), Notifications, 1);
    TestTrue(TEXT("Duplicate unlock is idempotent"), Game->TryUnlockOutfit(TEXT("Sportswear")));
    TestEqual(TEXT("Duplicate unlock does not notify again"), Notifications, 1);
    TestFalse(TEXT("Raincoat starts locked when only sportswear is owned"), Game->IsOutfitUnlocked(TEXT("Raincoat")));
    Game->bUnlockAllOutfitsOverride = true;
    TestTrue(TEXT("Temporary override also exposes raincoat"), Game->IsOutfitUnlocked(TEXT("Raincoat")));
    Game->bUnlockAllOutfitsOverride = false;
    TestFalse(TEXT("Raincoat override does not grant permanent ownership"), Game->IsOutfitUnlocked(TEXT("Raincoat")));
    TestTrue(TEXT("Raincoat unlock commits to the same global file"), Game->TryUnlockOutfit(TEXT("Raincoat")));
    TestTrue(TEXT("Raincoat unlock is idempotent"), Game->TryUnlockOutfit(TEXT("Raincoat")));
    TestEqual(TEXT("Each distinct committed unlock notifies once"), Notifications, 2);
    TestFalse(TEXT("Unknown unlock is rejected"), Game->TryUnlockOutfit(TEXT("Unknown")));
    Game->ResetRuntimeStateForSaveSlotSelection();
    TestTrue(TEXT("Save-slot reset preserves global unlocks"), Game->IsOutfitUnlocked(TEXT("Sportswear")));
    TestTrue(TEXT("Save-slot reset preserves raincoat ownership"), Game->IsOutfitUnlocked(TEXT("Raincoat")));
    Game->OnOutfitUnlocksChanged.Clear();

    auto* Restarted = NewObject<UTunaSweeperGameInstance>();
    Restarted->OutfitUnlockSavePath = SavePath;
    Restarted->bUnlockAllOutfitsOverride = false;
    TestTrue(TEXT("A fresh game instance restores the committed unlock"), Restarted->IsOutfitUnlocked(TEXT("Sportswear")));
    TestTrue(TEXT("A fresh game instance restores raincoat ownership"), Restarted->IsOutfitUnlocked(TEXT("Raincoat")));
    TestFalse(TEXT("Temporary override did not persist other outfits"), Restarted->IsOutfitUnlocked(TEXT("BunnyPajamas")));
    auto* OtherAccount = NewObject<UTunaSweeperGameInstance>();
    OtherAccount->OutfitUnlockSavePath = FPaths::Combine(Directory, TEXT("OtherAccount.sav"));
    OtherAccount->bUnlockAllOutfitsOverride = false;
    TestFalse(TEXT("An independent save root does not inherit unlocks"), OtherAccount->IsOutfitUnlocked(TEXT("Sportswear")));
    TestFalse(TEXT("An independent save root does not inherit raincoat"), OtherAccount->IsOutfitUnlocked(TEXT("Raincoat")));

    const FString Blocker = FPaths::Combine(Directory, TEXT("NotADirectory"));
    FFileHelper::SaveStringToFile(TEXT("block"), *Blocker);
    auto* Failed = NewObject<UTunaSweeperGameInstance>();
    Failed->OutfitUnlockSavePath = FPaths::Combine(Blocker, TEXT("CannotWrite.sav"));
    Failed->bUnlockAllOutfitsOverride = false;
    TestFalse(TEXT("Failed disk write rejects unlock"), Failed->TryUnlockOutfit(TEXT("SchoolUniform")));
    TestFalse(TEXT("Failed disk write rolls back the in-memory unlock"), Failed->IsOutfitUnlocked(TEXT("SchoolUniform")));
    TestFalse(TEXT("Failed disk write rejects raincoat unlock"), Failed->TryUnlockOutfit(TEXT("Raincoat")));
    TestFalse(TEXT("Failed disk write leaves raincoat locked"), Failed->IsOutfitUnlocked(TEXT("Raincoat")));
    return true;
}
#endif
