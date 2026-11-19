#if WITH_DEV_AUTOMATION_TESTS
#include "BeachDetailMotion.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBeachDetailMotionTest, "TunaSweeper.Environment.CoastalFish.BeachDetailMotion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBeachDetailMotionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("World"), World)) return false;
	auto* Detail = World->SpawnActor<ABeachDetailMotion>();
	if (!TestNotNull(TEXT("Beach detail"), Detail))
	{
		World->DestroyWorld(false);
		return false;
	}
	Detail->RegisterAllActorTickFunctions(true,false);
	Detail->InitializeDetail();
	const FVector Near = Detail->GetActorLocation() + FVector(9900,0,0);
	const FVector Far = Detail->GetActorLocation() + FVector(10100,0,0);
	Detail->ApplyPlayerLocations({Near});
	TestTrue(TEXT("Near buoy ticks without screen visibility"),Detail->IsActorTickEnabled());
	const FTransform Initial = Detail->Body->GetRelativeTransform();
	Detail->Tick(0.73f);
	const FVector Drift = Detail->Body->GetRelativeLocation();
	TestFalse(TEXT("Floating ring moves and tilts"),Initial.Equals(Detail->Body->GetRelativeTransform()));
	TestTrue(TEXT("Buoy remains close to waterline"),FMath::Abs(Drift.Z) <= 2.3 && Drift.Size2D() < 4.);
	TestTrue(TEXT("Ripple follows horizontal drift"), FVector::Dist2D(Drift,Detail->Ripple->GetRelativeLocation()) < 0.01);
	TestTrue(TEXT("Ripple stays on the water plane"),Detail->Ripple->GetRelativeRotation().IsNearlyZero() && FMath::IsNearlyEqual(Detail->Ripple->GetRelativeLocation().Z,0.7,0.01));
	const float RippleTime = Detail->Ripple->GetCustomPrimitiveData().Data[1];
	Detail->ApplyPlayerLocations({Far});
	TestFalse(TEXT("Distant buoy tick off"),Detail->IsActorTickEnabled());
	Detail->ApplyPlayerLocations({Near});
	TestEqual(TEXT("Ripple wake resumes saved phase"),Detail->Ripple->GetCustomPrimitiveData().Data[1],RippleTime);
	Detail->Tick(0.2f);
	TestTrue(TEXT("Ripple animation resumes"),Detail->Ripple->GetCustomPrimitiveData().Data[1] > RippleTime);

	Detail->bIsBurrow = true;
	Detail->InitializeDetail();
	Detail->ApplyPlayerLocations({Near});
	TestFalse(TEXT("Hole has no idle tick"),Detail->IsActorTickEnabled());
	TestFalse(TEXT("Hole hides the water ripple"),Detail->Ripple->IsVisible());
	TestEqual(TEXT("No idle grain instances"),Detail->Pellets->GetInstanceCount(),0);
	TestTrue(TEXT("Hole waits for independently scheduled event"),World->GetTimerManager().IsTimerActive(Detail->EventTimer));
	Detail->ApplyPlayerLocations({Far});
	TestTrue(TEXT("Distant idle hole pauses its one-shot timer"),World->GetTimerManager().IsTimerPaused(Detail->EventTimer));
	Detail->ApplyPlayerLocations({Near});
	Detail->StartBurrowEvent();
	TestTrue(TEXT("Short sand emission wakes tick"),Detail->IsActorTickEnabled() && Detail->Pellets->GetInstanceCount() >= 3);
	Detail->Tick(0.18f);
	FTransform Grain;
	Detail->Pellets->GetInstanceTransform(0,Grain);
	TestTrue(TEXT("Pellet rises out of the entrance"),Grain.GetTranslation().Z > 3.);
	Detail->ApplyPlayerLocations({Far});
	TestFalse(TEXT("Far event suspends and hides grains"),Detail->IsActorTickEnabled() || Detail->Pellets->IsVisible());
	const double Paused = Detail->EventAge;
	Detail->ApplyPlayerLocations({Near});
	TestEqual(TEXT("Event resumes at stored age"),Detail->EventAge,Paused);
	Detail->Tick(1.8f);
	TestFalse(TEXT("After one-shot the hole stops ticking"),Detail->IsActorTickEnabled());
	TestEqual(TEXT("Grains cleaned up"),Detail->Pellets->GetInstanceCount(),0);
	TestTrue(TEXT("Future event only scheduled once"),World->GetTimerManager().IsTimerActive(Detail->EventTimer));
	World->DestroyWorld(false);
	return true;
}
#endif
