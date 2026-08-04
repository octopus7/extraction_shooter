#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "StaticMeshQualityProfile.h"

#include "Engine/StaticMesh.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FStaticMeshQualityProfileStrictPairTest,
	"StaticMeshQualitySwitcher.Profile.StrictOneToOne",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStaticMeshQualityProfileStrictPairTest::RunTest(const FString& Parameters)
{
	UStaticMeshQualityProfile* Profile = NewObject<UStaticMeshQualityProfile>();
	UStaticMesh* OriginalA = NewObject<UStaticMesh>();
	UStaticMesh* LowA = NewObject<UStaticMesh>();
	UStaticMesh* OriginalB = NewObject<UStaticMesh>();
	UStaticMesh* LowB = NewObject<UStaticMesh>();

	FStaticMeshQualityPair PairA;
	PairA.OriginalMesh = OriginalA;
	PairA.LowMesh = LowA;
	FStaticMeshQualityPair PairB;
	PairB.OriginalMesh = OriginalB;
	PairB.LowMesh = LowB;
	Profile->MeshPairs = {PairA, PairB};

	TArray<FText> Errors;
	TestTrue(TEXT("Distinct Original and Low meshes form a valid profile"), Profile->ValidateProfile(Errors));
	TestEqual(TEXT("A valid profile has no errors"), Errors.Num(), 0);

	Profile->MeshPairs[1].LowMesh = LowA;
	TestFalse(TEXT("A Low mesh cannot be shared by different pairs"), Profile->ValidateProfile(Errors));
	TestTrue(TEXT("Shared Low mesh reports an error"), !Errors.IsEmpty());

	Profile->MeshPairs[1].LowMesh = OriginalA;
	TestFalse(TEXT("A mesh cannot be reused across Original and Low roles"), Profile->ValidateProfile(Errors));

	Profile->MeshPairs[1].LowMesh.Reset();
	TestFalse(TEXT("A pair cannot omit its dedicated Low mesh"), Profile->ValidateProfile(Errors));

	return true;
}

#endif
