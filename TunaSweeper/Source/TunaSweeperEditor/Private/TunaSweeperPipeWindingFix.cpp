#include "Misc/AutomationTest.h"
#include "Engine/StaticMesh.h"
#include "RawMesh.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaPipeWindingFix, "TunaSweeper.PipeMaintenance.FixWinding",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaPipeWindingFix::RunTest(const FString& Parameters)
{
    for (const TCHAR* Name : { TEXT("SM_BunkerPipe_Broken"), TEXT("SM_BunkerPipe_Repaired") })
    {
        auto* Mesh = LoadObject<UStaticMesh>(nullptr, *(FString(TEXT("/Game/Interaction/BunkerPipe/")) + Name));
        if (!TestNotNull(TEXT("Mesh loads"), Mesh)) return false;
        FRawMesh Raw;
        auto& Source = Mesh->GetSourceModel(0);
        Source.LoadRawMesh(Raw);
        for (int32 I = 0; I < Raw.WedgeIndices.Num(); I += 3)
        {
            Swap(Raw.WedgeIndices[I + 1], Raw.WedgeIndices[I + 2]);
            for (int32 UV = 0; UV < MAX_MESH_TEXTURE_COORDS; ++UV)
                if (Raw.WedgeTexCoords[UV].Num()) Swap(Raw.WedgeTexCoords[UV][I + 1], Raw.WedgeTexCoords[UV][I + 2]);
            if (Raw.WedgeColors.Num()) Swap(Raw.WedgeColors[I + 1], Raw.WedgeColors[I + 2]);
        }
        Raw.WedgeTangentX.Empty(); Raw.WedgeTangentY.Empty(); Raw.WedgeTangentZ.Empty();
        Source.BuildSettings.bRecomputeNormals = true;
        Source.BuildSettings.bRecomputeTangents = true;
        Source.SaveRawMesh(Raw);
        Mesh->Build(false);
        Mesh->PostEditChange();
        Mesh->MarkPackageDirty();
        FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone;
        TestTrue(TEXT("Reversed mesh saved"), UPackage::SavePackage(Mesh->GetOutermost(), Mesh,
            *FPackageName::LongPackageNameToFilename(Mesh->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args));
    }
    return true;
}
