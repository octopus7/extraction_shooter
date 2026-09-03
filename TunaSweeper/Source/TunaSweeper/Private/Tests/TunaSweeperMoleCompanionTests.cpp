#if WITH_DEV_AUTOMATION_TESTS

#include "Character/TunaSweeperMoleCompanionActor.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTunaSweeperMoleCompanionVisualComponentsTest,
	"TunaSweeper.Character.Mole.VisualComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMoleCompanionVisualComponentsTest::RunTest(const FString& Parameters)
{
	UClass* MoleBlueprintClass = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Characters/Mole/BP_Mole.BP_Mole_C"));
	TestNotNull(TEXT("BP_Mole generated class exists"), MoleBlueprintClass);
	ATunaSweeperMoleCompanionActor* MoleDefaults = MoleBlueprintClass
		? Cast<ATunaSweeperMoleCompanionActor>(MoleBlueprintClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("BP_Mole class default object exists"), MoleDefaults);
	if (!MoleDefaults)
	{
		return false;
	}

	const UStaticMeshComponent* DummyMesh = Cast<UStaticMeshComponent>(
		MoleDefaults->GetDefaultSubobjectByName(TEXT("DummyMesh")));
	TestNotNull(TEXT("Mole has one dummy static mesh component"), DummyMesh);
	if (DummyMesh)
	{
		UStaticMesh* DefaultDummyMesh = DummyMesh->GetStaticMesh().Get();
		TestNotNull(TEXT("Dummy mesh has a default static mesh"), DefaultDummyMesh);
		if (DefaultDummyMesh)
		{
			TestEqual(
				TEXT("Dummy mesh uses SM_MoleDummy"),
				DefaultDummyMesh->GetPathName(),
				FString(TEXT("/Game/Characters/NPC/Mole/SM_MoleDummy.SM_MoleDummy")));
		}
	}

	const USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(
		MoleDefaults->GetDefaultSubobjectByName(TEXT("SkeletalMesh")));
	TestNotNull(TEXT("Mole reserves a skeletal mesh component"), SkeletalMesh);
	if (SkeletalMesh)
	{
		TestNull(TEXT("Reserved skeletal mesh starts empty"), SkeletalMesh->GetSkeletalMeshAsset());
	}

	TestNull(TEXT("Legacy BodyMesh component was removed"), MoleDefaults->GetDefaultSubobjectByName(TEXT("BodyMesh")));
	TestNull(TEXT("Legacy HeadMesh component was removed"), MoleDefaults->GetDefaultSubobjectByName(TEXT("HeadMesh")));
	TestNull(TEXT("Legacy SnoutMesh component was removed"), MoleDefaults->GetDefaultSubobjectByName(TEXT("SnoutMesh")));
	return true;
}

#endif
