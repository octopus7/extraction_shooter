#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"
#include "Component/TunaSweeperQuestMarkerComponent.h"
#include "HAL/FileManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestMarkerMaterialLifetimeTest,
    "TunaSweeper.Quest.Marker.TemplateAndInstanceMaterialLifetime",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperQuestMarkerMaterialLifetimeTest::RunTest(const FString& Parameters)
{
    const FString Suffix = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    UPackage* TemplatePackage = CreatePackage(*(TEXT("/Temp/QuestMarkerTemplate_") + Suffix));
    UPackage* InstancePackage = CreatePackage(*(TEXT("/Temp/QuestMarkerInstance_") + Suffix));
    const FString Filename = FPaths::ProjectSavedDir() / TEXT("Automation") / (TEXT("QuestMarker_") + Suffix + TEXT(".uasset"));
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
    ON_SCOPE_EXIT { IFileManager::Get().Delete(*Filename); };

    auto* Template = NewObject<UTunaSweeperQuestMarkerComponent>(TemplatePackage, TEXT("MarkerTemplate"),
        RF_Public | RF_Standalone | RF_ArchetypeObject);
    if (!TestNotNull(TEXT("Marker surface material exists"), Template->SurfaceMaterial.Get())) return false;
    Template->RebuildMarker();
    bool bTemplateSafe = true;
    for (int32 Slot = 0; Slot < 5; ++Slot)
        bTemplateSafe &= TestNull(TEXT("Template rebuild never creates a dynamic material"),
            Cast<UMaterialInstanceDynamic>(Template->GetMaterial(Slot)));
    if (!bTemplateSafe) return false;

    // Emulate a legacy Blueprint that saved an MID through OverrideMaterials.
    auto* LegacyMID = UMaterialInstanceDynamic::Create(Template->SurfaceMaterial, Template);
    Template->SetMaterial(0, LegacyMID);
    Template->PostLoad();
    TestEqual(TEXT("Legacy template load restores the stable material asset"),
        Template->GetMaterial(0), Template->SurfaceMaterial.Get());
    TestTrue(TEXT("Legacy template MID cannot be saved again"), LegacyMID->HasAnyFlags(RF_Transient));

    auto* Instance = DuplicateObject<UTunaSweeperQuestMarkerComponent>(Template, InstancePackage, TEXT("MarkerInstance"));
    Instance->ClearFlags(RF_ArchetypeObject | RF_ClassDefaultObject);
    Instance->RebuildMarker();
    auto* FirstMID = Cast<UMaterialInstanceDynamic>(Instance->GetMaterial(0));
    if (!TestNotNull(TEXT("Instance rebuild creates its animated material"), FirstMID)) return false;
    TestTrue(TEXT("Instance material is transient"), FirstMID->HasAnyFlags(RF_Transient));
    TestEqual(TEXT("Instance owns its material"), FirstMID->GetOuter(), static_cast<UObject*>(Instance));
    TestEqual(TEXT("Instance material retains authored tint"), FirstMID->K2_GetVectorParameterValue(TEXT("MarkerColor")), Instance->GoldColor);

    auto* Duplicate = DuplicateObject<UTunaSweeperQuestMarkerComponent>(Instance, InstancePackage, TEXT("MarkerDuplicate"));
    Duplicate->RebuildMarker();
    auto* DuplicateMID = Cast<UMaterialInstanceDynamic>(Duplicate->GetMaterial(0));
    TestNotNull(TEXT("Duplicated marker rebuilds its material"), DuplicateMID);
    TestTrue(TEXT("Duplicated marker cannot share mutable material with source"), DuplicateMID && DuplicateMID != FirstMID);
    if (DuplicateMID) TestEqual(TEXT("Duplicate owns its rebuilt material"), DuplicateMID->GetOuter(), static_cast<UObject*>(Duplicate));

    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    SaveArgs.SaveFlags = SAVE_NoError;
    SaveArgs.bSlowTask = false;
    TestTrue(TEXT("Instance package saves without private template material references"),
        UPackage::SavePackage(InstancePackage, Instance, *Filename, SaveArgs));
    return true;
}
#endif
