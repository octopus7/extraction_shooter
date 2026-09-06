#if WITH_DEV_AUTOMATION_TESTS

#include "FoldingCanopyGarageDoorActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGarageDoorMotionIndicatorTest,
	"TunaSweeper.GarageDoor.MotionIndicators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGarageDoorMotionIndicatorTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("Test world"), World)) { return false; }
	// CreateWorld already initializes its WorldSettings; do not initialize twice.
	AFoldingCanopyGarageDoor* Door = World->SpawnActor<AFoldingCanopyGarageDoor>();
	if (!TestNotNull(TEXT("Door"), Door)) { World->DestroyWorld(false); return false; }
	Door->DispatchBeginPlay();
	TInlineComponentArray<UStaticMeshComponent*> Components(Door);
	UStaticMeshComponent* Green = nullptr;
	UStaticMeshComponent* Red = nullptr;
	for (UStaticMeshComponent* Component : Components)
	{
		if (Component->GetFName() == TEXT("OpeningIndicator")) { Green = Component; }
		if (Component->GetFName() == TEXT("ClosingIndicator")) { Red = Component; }
	}
	if (!TestNotNull(TEXT("Green lamp component"), Green) || !TestNotNull(TEXT("Red lamp component"), Red))
	{
		World->DestroyWorld(false);
		return false;
	}
	const auto Check = [this, Green, Red](const TCHAR* Label, bool bGreenOn, bool bRedOn)
	{
		const TArray<float>& G = Green->GetCustomPrimitiveData().Data;
		const TArray<float>& R = Red->GetCustomPrimitiveData().Data;
		TestTrue(FString(Label) + TEXT(" has complete material data"), G.Num() >= 4 && R.Num() >= 4);
		if (G.Num() >= 4 && R.Num() >= 4)
		{
			TestEqual(FString(Label) + TEXT(" green emissive"), G[0] > 0.0f, bGreenOn);
			TestEqual(FString(Label) + TEXT(" red emissive"), R[0] > 0.0f, bRedOn);
			TestTrue(FString(Label) + TEXT(" mutually exclusive"), G[0] == 0.0f || R[0] == 0.0f);
			TestTrue(TEXT("Green RGB"), G[2] > G[1] && G[2] > G[3]);
			TestTrue(TEXT("Red RGB"), R[1] > R[2] && R[1] > R[3]);
		}
	};
	Check(TEXT("Initially closed"), false, false);
	Door->OpenDoor(); Check(TEXT("Opening starts"), true, false);
	Door->Tick(1.0f); Check(TEXT("Opening in progress"), true, false);
	Door->CloseDoor(); Check(TEXT("Reverse to closing"), false, true);
	Door->OpenDoor(); Check(TEXT("Reverse to opening"), true, false);
	Door->SetDoorEnabled(false); Check(TEXT("Disabled midway"), false, false);
	const float PausedAlpha = Door->GetOpenAlpha();
	Door->Tick(1.0f);
	TestEqual(TEXT("Disabled door does not move"), Door->GetOpenAlpha(), PausedAlpha);
	Door->SetDoorEnabled(true); Check(TEXT("Resumed opening"), true, false);
	Door->Tick(10.0f); Check(TEXT("Fully open"), false, false);
	Door->CloseDoor(); Check(TEXT("Closing starts"), false, true);
	Door->Tick(10.0f); Check(TEXT("Fully closed"), false, false);
	Door->SetOpenAlpha(0.5f); Check(TEXT("Static partial pose"), false, false);
	Door->CloseDoor(); Check(TEXT("Close from partial"), false, true);
	Door->SetOpenAlpha(0.0f); Check(TEXT("Explicit closed endpoint"), false, false);
	Door->OpenDoor();
	Door->SetOpenAlpha(1.0f); Check(TEXT("Explicit open endpoint"), false, false);
	Door->SetDoorEnabled(false);
	Door->CloseDoor(); Check(TEXT("Disabled command ignored"), false, false);
	World->DestroyWorld(false);
	return true;
}

#endif
