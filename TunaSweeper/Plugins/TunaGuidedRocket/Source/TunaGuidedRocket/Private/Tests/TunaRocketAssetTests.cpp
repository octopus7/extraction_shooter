#include "TunaGuidedRocket.h"
#include "TunaRocketEffect.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaRocketAssetTest, "TunaGuidedRocket.Assets.SampleDefaults", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaRocketAssetTest::RunTest(const FString& Parameters)
{
    auto* Class = LoadClass<ATunaGuidedRocket>(nullptr, TEXT("/TunaGuidedRocket/Examples/BP_TestGuidedRocket.BP_TestGuidedRocket_C"));
    if (!TestNotNull(TEXT("Example rocket BP loads"), Class)) return false;
    const auto* Defaults = Class->GetDefaultObject<ATunaGuidedRocket>();
    TestNotNull(TEXT("BP selects a DA"), Defaults->Configuration.Get());
    UStaticMesh* Mesh = Defaults->RocketMesh->GetStaticMesh();
    TestNotNull(TEXT("BP stores native mesh component defaults"), Mesh);
    if (Mesh) TestTrue(TEXT("Small test rocket is under 50cm long"), Mesh->GetBounds().BoxExtent.X < 25);
    UStaticMesh* Exhaust = Defaults->ExhaustMesh->GetStaticMesh();
    TestNotNull(TEXT("BP exhaust mesh"), Exhaust);
    TestNotNull(TEXT("BP effect class"), Defaults->SimpleEffectClass.Get());
    if (Defaults->SimpleEffectClass)
    {
        const auto* FX = Defaults->SimpleEffectClass->GetDefaultObject<ATunaRocketEffect>();
        TestNotNull(TEXT("Effect particle mesh"), FX->ParticleMesh.Get());
        TestNotNull(TEXT("Effect material"), FX->ParticleMaterial.Get());
    }
    if (Defaults->Configuration) TestEqual(TEXT("Sample is safe visual testing"), Defaults->Configuration->Settings.Damage, 0.f);
    return true;
}
#endif
