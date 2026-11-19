#if WITH_DEV_AUTOMATION_TESTS

#include "CoastalFishHabitat.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Engine/SkinnedAsset.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "ReferenceSkeleton.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCoastalFishTest, "TunaSweeper.Environment.CoastalFish.Habitat",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCoastalFishTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World)) return false;
	ACoastalFishHabitat* Habitat = World->SpawnActor<ACoastalFishHabitat>();
	if (!TestNotNull(TEXT("Habitat"), Habitat))
	{
		World->DestroyWorld(false);
		return false;
	}
	Habitat->RegisterAllActorTickFunctions(true, false);
	Habitat->LargeCenter = FVector(25000,0,-700);
	Habitat->FryCenter = FVector(-25000,0,-60);
	Habitat->ResetSchool();
	Habitat->UpdateAdult(false);
	Habitat->UpdateAdult(true);
	Habitat->UpdateFry();
	TestTrue(TEXT("Fry school has 20–40 fish"), Habitat->School->GetInstanceCount() >= 20 && Habitat->School->GetInstanceCount() <= 40);
	auto AtDistance = [&](const FVector& Origin, double Distance)
	{
		Habitat->ApplyPlayerLocations({Origin + FVector(0,0,Distance)});
	};
	const FVector MediumStart = Habitat->MediumFish->GetComponentLocation();
	AtDistance(MediumStart, 10000.);
	TestTrue(TEXT("Inclusive 100m offscreen gate"), Habitat->bMediumActive && Habitat->IsActorTickEnabled());
	TestFalse(TEXT("Distant adults and fry sleep independently"), Habitat->bLargeActive || Habitat->bFryActive);
	Habitat->Tick(0.2f);
	TestFalse(TEXT("Medium advances"), MediumStart.Equals(Habitat->MediumFish->GetComponentLocation()));
	TestEqual(TEXT("Sleeping large phase"), Habitat->LargeTime, 0.);
	const int32 Tail = Habitat->MediumFish->GetBoneIndex(TEXT("tail"));
	const int32 Head = Habitat->MediumFish->GetBoneIndex(TEXT("body_01"));
	if (TestTrue(TEXT("Loaded articulated skeleton"), Tail >= 0 && Head >= 0))
	{
		const auto& Ref = Habitat->MediumFish->GetSkinnedAsset()->GetRefSkeleton().GetRefBonePose();
		TestFalse(TEXT("Medium tail deforms away from bind"), Habitat->MediumFish->BoneSpaceTransforms[Tail].GetRotation().Equals(Ref[Tail].GetRotation(), 0.001));
	}
	const FVector Paused = Habitat->MediumFish->GetComponentLocation();
	AtDistance(Paused, 10001.);
	TestFalse(TEXT("Empty habitat disables actor tick"), Habitat->IsActorTickEnabled());
	AtDistance(Paused, 10000.);
	TestTrue(TEXT("Wake preserves previous pose"), Paused.Equals(Habitat->MediumFish->GetComponentLocation()));
	AtDistance(Habitat->LargeFish->GetComponentLocation(), 0.);
	Habitat->Tick(1.7f);
	const int32 LargeTail = Habitat->LargeFish->GetBoneIndex(TEXT("tail"));
	const int32 LargeHead = Habitat->LargeFish->GetBoneIndex(TEXT("body_01"));
	if (TestTrue(TEXT("Large chain available"), LargeHead >= 0 && LargeTail >= 0))
	{
		const auto& Ref = Habitat->LargeFish->GetSkinnedAsset()->GetRefSkeleton().GetRefBonePose();
		const double TailAngle = Habitat->LargeFish->BoneSpaceTransforms[LargeTail].GetRotation().AngularDistance(Ref[LargeTail].GetRotation());
		const double HeadAngle = Habitat->LargeFish->BoneSpaceTransforms[LargeHead].GetRotation().AngularDistance(Ref[LargeHead].GetRotation());
		TestTrue(TEXT("Large tail bends more than head"), TailAngle > HeadAngle);
	}
	AtDistance(Habitat->GetActorTransform().TransformPosition(Habitat->FryCenter), 0.);
	Habitat->Tick(0.25f);
	TestTrue(TEXT("Only fry advance"), Habitat->bFryActive && !Habitat->bMediumActive && !Habitat->bLargeActive);
	const FVector A = Habitat->FryPosition(0, 0.);
	const FVector B = Habitat->FryPosition(1, 0.);
	const FVector C = Habitat->FryPosition(0, 3.);
	const FVector D = Habitat->FryPosition(1, 3.);
	TestTrue(TEXT("School splits at obstacle and reforms away from it"), FVector::Dist(A,B) > FVector::Dist(C,D) + 20.);
	TestTrue(TEXT("Rejoined fish retain spacing"), FVector::Dist(C,D) > 5.);
	const TArray<float> ShaderPose = Habitat->School->PerInstanceSMCustomData;
	AtDistance(Paused, 0.);
	Habitat->Tick(0.3f);
	TestTrue(TEXT("Dormant fry shader phase frozen"), ShaderPose == Habitat->School->PerInstanceSMCustomData);
	Habitat->bEnableFry = false;
	Habitat->bEnableMedium = false;
	Habitat->InitialPhaseSeconds = 2.5f;
	Habitat->InitializePose();
	AtDistance(Habitat->MediumFish->GetComponentLocation(), 0.);
	TestFalse(TEXT("Excluded medium never activates"), Habitat->bMediumActive);
	TestFalse(TEXT("Editor preview hides excluded groups"), Habitat->MediumFish->IsVisible() || Habitat->School->IsVisible());
	TestEqual(TEXT("Excluded school has no instances"), Habitat->School->GetInstanceCount(), 0);
	TestEqual(TEXT("Authored phase applies to included groups"), Habitat->LargeTime, 2.5);
	World->DestroyWorld(false);
	return true;
}
#endif
