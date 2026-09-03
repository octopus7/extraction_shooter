#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/TunaSweeperCrowbarWallRackActor.h"

#include "Components/StaticMeshComponent.h"
#include "Misc/AutomationTest.h"

namespace TunaSweeperCrowbarWallRackTests
{
	constexpr EAutomationTestFlags TestFlags =
		EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperCrowbarWallRackDefaultsTest,
	"TunaSweeper.Interaction.CrowbarWallRack.Defaults",
	TunaSweeperCrowbarWallRackTests::TestFlags)

bool FTunaSweeperCrowbarWallRackDefaultsTest::RunTest(const FString& Parameters)
{
	(void)Parameters;

	UClass* BlueprintClass = LoadClass<ATunaSweeperCrowbarWallRackActor>(
		nullptr,
		TEXT("/Game/Interaction/BP_CrowbarWallRack.BP_CrowbarWallRack_C"));
	TestNotNull(TEXT("BP_CrowbarWallRack exists"), BlueprintClass);

	const ATunaSweeperCrowbarWallRackActor* Defaults = BlueprintClass
		? Cast<ATunaSweeperCrowbarWallRackActor>(BlueprintClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Blueprint uses the crowbar wall rack parent"), Defaults);
	if (!Defaults)
	{
		return false;
	}

	const UStaticMeshComponent* PedestalMesh = Defaults->GetPedestalMeshComponent();
	const UStaticMeshComponent* CrowbarMesh = Defaults->GetCrowbarMeshComponent();
	TestNotNull(TEXT("PedestalMesh component exists"), PedestalMesh);
	TestNotNull(TEXT("CrowbarMesh component exists"), CrowbarMesh);
	TestNull(TEXT("PedestalMesh asset remains unassigned"), PedestalMesh ? PedestalMesh->GetStaticMesh() : nullptr);
	TestNull(TEXT("CrowbarMesh asset remains unassigned"), CrowbarMesh ? CrowbarMesh->GetStaticMesh() : nullptr);
	TestEqual(TEXT("Crowbar item ID is 6003"), Defaults->GetCrowbarItemId(), 6003);
	TestEqual(
		TEXT("Water-intake progress key matches the debris actor"),
		Defaults->GetWaterIntakeProgressObjectId(),
		FName(TEXT("demo.water_intake.blocked_screen")));

	TInlineComponentArray<UStaticMeshComponent*> MeshComponents;
	const_cast<ATunaSweeperCrowbarWallRackActor*>(Defaults)->GetComponents(MeshComponents);
	TestEqual(TEXT("Actor exposes exactly two static mesh components"), MeshComponents.Num(), 2);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
