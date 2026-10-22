#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Interfaces/IPluginManager.h"
#include "PixelWorldCore.h"
#include "Engine/StaticMesh.h"

namespace
{
    FString Fixture(const TCHAR* Name)
    {
        return IPluginManager::Get().FindPlugin(TEXT("PixelWorldBuilder"))->GetBaseDir() / TEXT("Resources/Tests") / Name;
    }
    UPixelWorldProfile* MakeProfile()
    {
        auto* Profile = NewObject<UPixelWorldProfile>();
        Profile->ColorPng.FilePath = Fixture(TEXT("rgb.png"));
        Profile->MetersPerPixel = 10.f;
        Profile->Rules.SetNum(4);
        const FColor Colors[] = {FColor(128,64,32), FColor(17,129,253), FColor(1,2,3), FColor(254,127,63)};
        for (int32 I=0; I<4; ++I)
        {
            Profile->Rules[I].Red = Colors[I].R;
            Profile->Rules[I].Green = Colors[I].G;
            Profile->Rules[I].Blue = Colors[I].B;
            Profile->Rules[I].HeightMeters = I;
        }
        return Profile;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldRgbTest, "PixelWorldBuilder.Core.StoredRgb", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldRgbTest::RunTest(const FString&)
{
    FPixelWorldImage Image; FText Error;
    if (!TestTrue(TEXT("Decode PNG with sRGB and gAMA chunks"), PixelWorld::ReadColorPng(Fixture(TEXT("rgb.png")), Image, Error))) return false;
    TestEqual(TEXT("Width"), Image.Width, 2);
    TestEqual(TEXT("Height"), Image.Height, 2);
    if (!TestEqual(TEXT("Pixel count"), Image.Pixels.Num(),4)) return false;
    TestEqual(TEXT("Stored non-primary RGB is unchanged"), Image.Pixels[0], FColor(128,64,32));
    TestEqual(TEXT("Channels not reversed"), Image.Pixels[1], FColor(17,129,253));
    TestFalse(TEXT("16-bit color input is not silently quantized"), PixelWorld::ReadColorPng(Fixture(TEXT("height.png")), Image, Error));
    TestFalse(TEXT("Missing file rejected"), PixelWorld::ReadColorPng(Fixture(TEXT("missing.png")), Image, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldPlanTest, "PixelWorldBuilder.Core.OrientationAndValidation", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldPlanTest::RunTest(const FString&)
{
    auto* P = MakeProfile();
    TestEqual(TEXT("Top left is north-west"), PixelWorld::PixelCenter(0,0,2,2,*P), FVector(500,-500,0));
    TestEqual(TEXT("Bottom right is south-east"), PixelWorld::PixelCenter(1,1,2,2,*P), FVector(-500,500,0));
    TestEqual(TEXT("Non-square map uses height along X"), PixelWorld::PixelCenter(2,0,3,2,*P), FVector(500,1000,0));
    FPixelWorldPlan Plan; FText Error;
    if (!TestTrue(TEXT("Valid image builds plan"), PixelWorld::BuildPlan(*P,Plan,Error))) return false;
    TestEqual(TEXT("Valid component resolution"), Plan.VerticesX,64);
    TestEqual(TEXT("Physical north-south extent"), (Plan.VerticesX-1)*Plan.LandscapeScale.X,2000.0);
    TestEqual(TEXT("Each category counted"), Plan.RulePixelCounts[2],1);
    TestTrue(TEXT("Southwest height above northwest"), Plan.Heights[0]>Plan.Heights[63]);
    P->Rules[1]=P->Rules[0];
    TestFalse(TEXT("Duplicate palette entries rejected"), PixelWorld::BuildPlan(*P,Plan,Error));
    P=MakeProfile(); P->Rules.RemoveAt(3);
    TestFalse(TEXT("Unknown RGB rejected before generation"), PixelWorld::BuildPlan(*P,Plan,Error));
    P=MakeProfile(); P->MetersPerPixel=0;
    TestFalse(TEXT("Zero size rejected"), PixelWorld::BuildPlan(*P,Plan,Error));
    P=MakeProfile(); P->ColorPng.FilePath=Fixture(TEXT("transparent.png"));
    TestFalse(TEXT("Partial alpha is explicit error"), PixelWorld::BuildPlan(*P,Plan,Error));
    P=MakeProfile(); P->HeightPng.FilePath=Fixture(TEXT("height.png"));
    if (TestTrue(TEXT("16-bit heightmap accepted separately"), PixelWorld::BuildPlan(*P,Plan,Error)))
    {
        TestEqual(TEXT("Heightmap north-west minimum"),Plan.Heights[63],static_cast<uint16>(32768));
        TestEqual(TEXT("Heightmap north-east maximum"),Plan.Heights.Last(),static_cast<uint16>(35328));
        TestEqual(TEXT("Heightmap south-west midpoint"),Plan.Heights[0],static_cast<uint16>(34048));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPixelWorldScatterTest, "PixelWorldBuilder.Core.DeterministicScatter", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPixelWorldScatterTest::RunTest(const FString&)
{
    auto* P=MakeProfile();
    UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    for (auto& Rule:P->Rules)
    {
        Rule.HeightMeters=0;
        auto& V=Rule.Vegetation.AddDefaulted_GetRef(); V.Mesh=Mesh;
        V.DensityPer100SquareMeters=4; V.MinimumSpacingMeters=1;
    }
    FPixelWorldPlan A,B; FText Error;
    if (!TestTrue(TEXT("Scatter plan built"),PixelWorld::BuildPlan(*P,A,Error))) return false;
    if (!TestTrue(TEXT("Repeated plan built"),PixelWorld::BuildPlan(*P,B,Error))) return false;
    TestEqual(TEXT("Density uses world area"),A.Placements.Num(),16);
    if (!TestEqual(TEXT("Repeat count"),A.Placements.Num(),B.Placements.Num())) return false;
    for (int32 I=0;I<A.Placements.Num();++I)
    {
        TestTrue(TEXT("Same seed reproduces transforms"),A.Placements[I].Transform.Equals(B.Placements[I].Transform));
        const auto Pos=A.Placements[I].Transform.GetLocation();
        TestTrue(TEXT("Placement within world rectangle"), FMath::Abs(Pos.X)<1000 && FMath::Abs(Pos.Y)<1000);
        TestEqual(TEXT("Placement on terrain"),Pos.Z,0.0);
        for(int32 J=0;J<I;++J)
            if(A.Placements[I].RuleIndex==A.Placements[J].RuleIndex)
                TestTrue(TEXT("Spacing is enforced"),FVector::Dist2D(Pos,A.Placements[J].Transform.GetLocation())>=100.0);
    }
    P->Seed++;
    TestTrue(TEXT("Changed seed builds"),PixelWorld::BuildPlan(*P,B,Error));
    TestFalse(TEXT("Different seed changes scatter"),A.Placements[0].Transform.Equals(B.Placements[0].Transform));
    P->Rules[0].Vegetation[0].DensityPer100SquareMeters=1.e9f;
    TestFalse(TEXT("Excessive placement count rejected"),PixelWorld::BuildPlan(*P,B,Error));
    return true;
}
#endif
