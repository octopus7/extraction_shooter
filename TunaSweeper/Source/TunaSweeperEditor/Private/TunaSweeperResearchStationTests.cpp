#if WITH_DEV_AUTOMATION_TESTS

#include "Components/StaticMeshComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperResearchStationActor.h"
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "PhysicsEngine/BodySetup.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperResearchStationPresentationTest,
	"TunaSweeper.Research.StationPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperResearchStationPresentationTest::RunTest(const FString& Parameters)
{
	UClass* StationClass = LoadClass<ATunaSweeperResearchStationActor>(nullptr,
		TEXT("/Game/Interaction/BP_ResearchSinkInteraction.BP_ResearchSinkInteraction_C"));
	if (!TestNotNull(TEXT("Placeable bathroom research station loads"), StationClass)) return false;
	UWorld* World = GEditor->GetEditorWorldContext().World();
	ATunaSweeperResearchStationActor* Station = World->SpawnActor<ATunaSweeperResearchStationActor>(StationClass);
	if (!TestNotNull(TEXT("Bathroom research station spawns"), Station)) return false;
	ON_SCOPE_EXIT { Station->Destroy(); };

	UStaticMeshComponent* Body = nullptr;
	UStaticMeshComponent* Scanner = nullptr;
	TArray<UStaticMeshComponent*> Meshes;
	Station->GetComponents(Meshes);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetFName() == TEXT("VisualMesh")) Body = Mesh;
		if (Mesh->GetFName() == TEXT("ScanBarMesh")) Scanner = Mesh;
	}
	if (!TestNotNull(TEXT("Visible mirror body exists"), Body) ||
		!TestNotNull(TEXT("Independently moving static scan bar exists"), Scanner)) return false;
	TestFalse(TEXT("Mirror is visible during play"), Body->bHiddenInGame);
	TestNotNull(TEXT("Mirror has an authored static mesh"), Body->GetStaticMesh().Get());
	TestNotNull(TEXT("Scan bar has an authored static mesh"), Scanner->GetStaticMesh().Get());
	if (Body->GetStaticMesh())
	{
		TestTrue(TEXT("Body uses the bathroom model instead of the placement cube"),
			Body->GetStaticMesh()->GetPathName().StartsWith(TEXT("/Game/Environment/Bunker/ResearchMirror/")));
		const UBodySetup* Collision = Body->GetStaticMesh()->GetBodySetup();
		TestTrue(TEXT("Mirror body has a simple collision box"), Collision && Collision->AggGeom.BoxElems.Num() == 1);
	}
	TestEqual(TEXT("Scan bar never blocks the player"), Scanner->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	const FVector Initial = Scanner->GetRelativeLocation();
	Station->DispatchBeginPlay();
	TestTrue(TEXT("Station enables its registered runtime tick"), Station->PrimaryActorTick.bCanEverTick && Station->IsActorTickEnabled());
	Station->Tick(1.0f);
	const FVector Upper = Scanner->GetRelativeLocation();
	TestTrue(TEXT("Scan moves upward after one quarter cycle"), Upper.Z > Initial.Z + 10.0f);
	TestTrue(TEXT("Scan stays on the mirror surface"), FMath::IsNearlyEqual(Upper.X, Initial.X) && FMath::IsNearlyEqual(Upper.Y, Initial.Y));
	Station->Tick(2.0f);
	TestTrue(TEXT("Scan moves below its rest position after three quarters"), Scanner->GetRelativeLocation().Z < Initial.Z - 10.0f);
	Station->Tick(1.0f);
	TestTrue(TEXT("Four-second loop returns to its initial position without drift"), Scanner->GetRelativeLocation().Equals(Initial, 0.01f));
	ATunaSweeperResearchStationActor* Disabled = World->SpawnActor<ATunaSweeperResearchStationActor>(StationClass);
	if (!TestNotNull(TEXT("Static presentation instance spawns"), Disabled)) return false;
	ON_SCOPE_EXIT { Disabled->Destroy(); };
	FBoolProperty* AnimateProperty = FindFProperty<FBoolProperty>(Disabled->GetClass(), TEXT("bAnimateScan"));
	if (!TestNotNull(TEXT("Animation toggle is editable"), AnimateProperty)) return false;
	AnimateProperty->SetPropertyValue_InContainer(Disabled, false);
	Disabled->DispatchBeginPlay();
	TestFalse(TEXT("Disabling animation leaves no runtime tick"), Disabled->IsActorTickEnabled());
	return true;
}

#endif
