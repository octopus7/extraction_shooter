#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperPowerFacilityActor.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Particles/ParticleEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperPowerFacilityAssetTest,
	"TunaSweeper.Interaction.PowerFacility.BlueprintAsset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperPowerFacilityAssetTest::RunTest(const FString& Parameters)
{
	auto* Blueprint = LoadObject<UBlueprint>(nullptr, TEXT("/Game/Interaction/BP_PowerFacility.BP_PowerFacility"));
	if (!TestNotNull(TEXT("Power facility BP reloads from disk"), Blueprint)) return false;
	TestTrue(TEXT("Power facility BP compiled"), Blueprint->Status == BS_UpToDate);
	TestTrue(TEXT("Dedicated native parent"), Blueprint->ParentClass == ATunaSweeperPowerFacilityActor::StaticClass());
	UWorld* World = UWorld::CreateWorld(EWorldType::EditorPreview, false);
	auto* Actor = World->SpawnActor<ATunaSweeperPowerFacilityActor>(Blueprint->GeneratedClass);
	if (!TestNotNull(TEXT("BP can instantiate"), Actor)) { World->DestroyWorld(false); return false; }
	auto* Mesh = Actor->FindComponentByClass<UStaticMeshComponent>();
	auto* Sparks = Actor->FindComponentByClass<UParticleSystemComponent>();
	TestNotNull(TEXT("Electrical cabinet mesh is assigned"), Mesh->GetStaticMesh().Get());
	if (TestNotNull(TEXT("Fault particle asset assigned"), Sparks->Template.Get()))
	{
		TestEqual(TEXT("One lightweight spark emitter"), Sparks->Template->Emitters.Num(), 1);
		for (const auto& Emitter : Sparks->Template->Emitters)
		{
			TestTrue(TEXT("Spark emitter has LOD"), !Emitter->LODLevels.IsEmpty());
			for (const auto& Lod : Emitter->LODLevels)
			{
				TestEqual(TEXT("Fault sparks loop until repaired"), Lod->RequiredModule->EmitterLoops, 0);
				TestNotNull(TEXT("Spark material assigned"), Lod->RequiredModule->Material.Get());
			}
		}
	}
	TestTrue(TEXT("Broken editor preview shows sparks"), Sparks->IsVisible());
	auto* Preview = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("bPreviewRepairedInEditor"));
	Preview->SetPropertyValue_InContainer(Actor, true);
	Actor->RefreshFacilityState();
	TestFalse(TEXT("Repaired editor preview hides sparks"), Sparks->IsVisible());
	TestFalse(TEXT("Repaired editor preview stops sparks"), Sparks->IsActive());
	TestTrue(TEXT("Preview preserves facility mesh visibility"), Mesh->IsVisible());
	TestTrue(TEXT("BP has no shared persistence ID"), FindFProperty<FNameProperty>(Actor->GetClass(), TEXT("ProgressObjectId"))->GetPropertyValue_InContainer(Actor).IsNone());
	World->DestroyWorld(false);
	World->RemoveFromRoot();
	return true;
}
#endif
