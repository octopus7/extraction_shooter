#if WITH_DEV_AUTOMATION_TESTS
#include "ShoreCrustacean.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FShoreCrustaceanTest, "TunaSweeper.Environment.CoastalFish.ShoreCrustacean",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FShoreCrustaceanTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World)) return false;
	auto* Animal = World->SpawnActor<AShoreCrustacean>();
	if (!TestNotNull(TEXT("Crustacean"), Animal))
	{
		World->DestroyWorld(false);
		return false;
	}
	Animal->RegisterAllActorTickFunctions(true,false);
	Animal->RouteExtent = FVector2D(80,35);
	Animal->BedHeightOffsets = {-8,0,8,-8,0,8,-8,0,8};
	Animal->InitializeAnimal();
	const FVector Start = Animal->Shell->GetComponentLocation();
	Animal->ApplyPlayerLocations({Start + FVector(0,0,10000)});
	TestTrue(TEXT("Active at 100m"), Animal->IsActorTickEnabled());
	Animal->Tick(0.65f);
	const FVector Moved = Animal->Shell->GetComponentLocation();
	TestTrue(TEXT("Crab travels sideways"), FMath::Abs(FVector::DotProduct((Moved-Start).GetSafeNormal(), Animal->Shell->GetForwardVector())) < 0.5);
	TestTrue(TEXT("Follows sloping visual bed"), FMath::Abs(Moved.Z - Moved.X * 0.1) < 0.05);
	TestTrue(TEXT("Shell aligns to bed normal"), Animal->Shell->GetUpVector().Equals(FVector(-0.1,0,1).GetSafeNormal(),0.015));
	TestTrue(TEXT("Walking legs are articulated"), Animal->Shell->GetBoneIndex(TEXT("leg_L_1")) >= 0);
	Animal->ApplyPlayerLocations({Moved + FVector(0,0,10001)});
	TestFalse(TEXT("Distant actor sleeps"), Animal->IsActorTickEnabled());
	const auto Frozen = Animal->Shell->GetComponentTransform();
	Animal->ApplyPlayerLocations({Moved});
	TestTrue(TEXT("Wake preserves current pose"), Frozen.Equals(Animal->Shell->GetComponentTransform()));
	Animal->bIsCrayfish = true;
	Animal->InitializeAnimal();
	const FVector CrayStart = Animal->Shell->GetComponentLocation();
	Animal->Tick(0.5f);
	TestTrue(TEXT("Crayfish walks head first"), FVector::DotProduct((Animal->Shell->GetComponentLocation()-CrayStart).GetSafeNormal(), Animal->Shell->GetForwardVector()) > 0.85);
	const int32 Feeler = Animal->Shell->GetBoneIndex(TEXT("feeler_L"));
	const int32 Tail = Animal->Shell->GetBoneIndex(TEXT("tail_02"));
	if (TestTrue(TEXT("Feelers and tail are rigged"), Feeler >= 0 && Tail >= 0))
	{
		const FQuat AntennaPose = Animal->Shell->BoneSpaceTransforms[Feeler].GetRotation();
		const FQuat TailPose = Animal->Shell->BoneSpaceTransforms[Tail].GetRotation();
		Animal->Tick(0.21f);
		TestFalse(TEXT("Feeler sweeps"), AntennaPose.Equals(Animal->Shell->BoneSpaceTransforms[Feeler].GetRotation(),0.0001));
		TestFalse(TEXT("Tail flexes"), TailPose.Equals(Animal->Shell->BoneSpaceTransforms[Tail].GetRotation(),0.0001));
	}
	Animal->MotionTime = 3.6;
	Animal->UpdatePose();
	const FVector Rest = Animal->Shell->GetComponentLocation();
	Animal->Tick(0.12f);
	TestTrue(TEXT("Scuttle has stationary pauses"), Rest.Equals(Animal->Shell->GetComponentLocation(),0.01));
	World->DestroyWorld(false);
	return true;
}
#endif
