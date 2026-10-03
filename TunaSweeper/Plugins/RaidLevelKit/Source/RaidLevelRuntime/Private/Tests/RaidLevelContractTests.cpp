#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Raid/RaidLevelIdentity.h"
#include "Raid/TunaSweeperLootAnchorPreviewDataAsset.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Internationalization/StringTableRegistry.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaidLevelSavedContractTest, "TunaSweeper.RaidPlacement.PluginSavedContract", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaidLevelSavedContractTest::RunTest(const FString& Parameters)
{
    UBlueprint* BP = LoadObject<UBlueprint>(nullptr, TEXT("/RaidLevelKit/Placement/BP_RaidPlacementAnchor.BP_RaidPlacementAnchor"));
    if (!TestNotNull(TEXT("Saved Blueprint reloads"), BP)) return false;
    TestEqual(TEXT("Native parent redirects to plugin module"), BP->ParentClass.Get(), ATunaSweeperRaidPlacementAnchor::StaticClass());
    TestEqual(TEXT("Native class belongs to runtime plugin"), BP->ParentClass->GetOutermost()->GetName(), FString(TEXT("/Script/RaidLevelRuntime")));
    auto LoadRedirected = [](const TCHAR* OldPath) { FSoftObjectPath Path(OldPath); Path.FixupCoreRedirects(); return Path.TryLoad(); };
    TestEqual(TEXT("Old class redirect resolves"), Cast<UClass>(LoadRedirected(TEXT("/Script/TunaSweeper.TunaSweeperRaidPlacementAnchor"))), BP->ParentClass.Get());
    UEnum* OldEnum = Cast<UEnum>(LoadRedirected(TEXT("/Script/TunaSweeper.ETunaSweeperRaidPlacementAnchorKind")));
    TestEqual(TEXT("Enum redirect resolves"), OldEnum, StaticEnum<ETunaSweeperRaidPlacementAnchorKind>());
    if (OldEnum)
    {
        TestEqual(TEXT("Enemy serialized ordinal"), OldEnum->GetValueByNameString(TEXT("Enemy")), int64(0));
        TestEqual(TEXT("Loot serialized ordinal"), OldEnum->GetValueByNameString(TEXT("LootContainer")), int64(1));
        TestEqual(TEXT("Memo serialized ordinal"), OldEnum->GetValueByNameString(TEXT("Memo")), int64(2));
        TestEqual(TEXT("Authored serialized ordinal"), OldEnum->GetValueByNameString(TEXT("AuthoredActor")), int64(3));
    }
    TestEqual(TEXT("Preview struct redirect resolves"), Cast<UScriptStruct>(LoadRedirected(TEXT("/Script/TunaSweeper.TunaSweeperLootAnchorPreviewDefinition"))), FTunaSweeperLootAnchorPreviewDefinition::StaticStruct());
    UTunaSweeperLootAnchorPreviewDataAsset* Catalog = LoadObject<UTunaSweeperLootAnchorPreviewDataAsset>(nullptr, TEXT("/RaidLevelKit/Placement/DA_LootAnchorPreviews.DA_LootAnchorPreviews"));
    if (TestNotNull(TEXT("Saved data asset reloads"), Catalog))
    {
        TestEqual(TEXT("All serialized struct entries survive"), Catalog->PreviewDefinitions.Num(), 3);
        for (const auto& Entry : Catalog->PreviewDefinitions)
        {
            TestTrue(TEXT("Preview mesh resides in plugin"), Entry.PreviewMesh.ToSoftObjectPath().ToString().StartsWith(TEXT("/RaidLevelKit/")));
            TestNotNull(TEXT("Preview mesh loads"), Entry.PreviewMesh.LoadSynchronous());
        }
    }
    TestTrue(TEXT("Independent keyed editor strings registered"), FStringTableRegistry::Get().FindStringTable(TEXT("RaidLevelKit.Editor")).IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRaidLevelStructureTest, "TunaSweeper.RaidPlacement.PluginStructure", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaidLevelStructureTest::RunTest(const FString& Parameters)
{
    using K = ETunaSweeperRaidPlacementAnchorKind;
    TArray<FRaidPlacementDescriptor> Rows = {{1,K::Enemy,true},{1,K::Enemy,true},{1000,K::LootContainer,false},{2000,K::Memo,false},{3000,K::AuthoredActor,false}};
    TestTrue(TEXT("Only opted-in enemies may share id"), ValidateRaidPlacementStructure(Rows).IsEmpty());
    Rows[1].bAllowDuplicatePlacementId = false;
    auto Issues = ValidateRaidPlacementStructure(Rows);
    TestEqual(TEXT("One non-opted-in enemy rejects group"), Issues.Num(), 1);
    if (!Issues.IsEmpty())
    {
        TestEqual(TEXT("Validation returns localization key"), Issues[0].StringKey, FName(TEXT("Validation.DuplicatePlacementId")));
        TestEqual(TEXT("Validation returns named substitution"), Issues[0].Arguments.FindRef(TEXT("PlacementId")), FString(TEXT("1")));
    }
    Rows = {{1000,K::LootContainer,true},{1000,K::LootContainer,true}};
    TestEqual(TEXT("Loot never permits duplicate"), ValidateRaidPlacementStructure(Rows).Num(), 1);
    Rows = {{2000,K::Memo,true},{2000,K::Memo,true}};
    TestEqual(TEXT("Memo never permits duplicate"), ValidateRaidPlacementStructure(Rows).Num(), 1);
    Rows = {{1,K::Enemy,true},{1,K::Memo,true}};
    TestEqual(TEXT("Cross-kind duplicates rejected"), ValidateRaidPlacementStructure(Rows).Num(), 1);
    Rows = {{0,K::Enemy,false},{-1,K::Enemy,false},{2,static_cast<K>(255),false}};
    TestEqual(TEXT("Non-positive ids and unknown kinds rejected"), ValidateRaidPlacementStructure(Rows).Num(), 3);
    return true;
}
#endif
