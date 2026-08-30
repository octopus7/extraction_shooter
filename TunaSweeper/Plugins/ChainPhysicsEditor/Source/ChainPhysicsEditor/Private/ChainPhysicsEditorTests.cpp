#if WITH_DEV_AUTOMATION_TESTS

#include "ChainPhysicsSetup.h"

#include "Animation/AnimBlueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Misc/AutomationTest.h"
#include "PhysicsEngine/PhysicsAsset.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChainPhysicsLunaDetectionTest,
	"TunaSweeper.ChainPhysicsEditor.LunaMk2ExistingSetup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChainPhysicsLunaDetectionTest::RunTest(const FString& Parameters)
{
	USkeletalMesh* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Characters/Player/LunaMk2/SKM_LunaMk2.SKM_LunaMk2"));
	TestNotNull(TEXT("Luna Mk2 skeletal mesh loads"), Mesh);
	if (!Mesh)
	{
		return false;
	}

	ChainPhysicsSetup::FAnalysisResult Analysis;
	TestTrue(TEXT("Mesh analysis succeeds"), ChainPhysicsSetup::AnalyzeSkeletalMesh(Mesh, Analysis));
	const TSet<FName> ExpectedRoots = { FName(TEXT("sidetail_L_01")), FName(TEXT("sidetail_R_01")) };
	TSet<FName> FoundRoots;
	for (const ChainPhysicsSetup::FChainCandidate& Chain : Analysis.Chains)
	{
		if (ExpectedRoots.Contains(Chain.RootBone))
		{
			FoundRoots.Add(Chain.RootBone);
			TestEqual(FString::Printf(TEXT("%s has six bones"), *Chain.RootBone.ToString()), Chain.BoneNames.Num(), 6);
			TestEqual(FString::Printf(TEXT("%s is complete"), *Chain.RootBone.ToString()), Chain.SetupState, ChainPhysicsSetup::ESetupState::Complete);
			TestEqual(FString::Printf(TEXT("%s uses the expected PA"), *Chain.RootBone.ToString()),
				Chain.DetectedPhysicsAsset.IsValid() ? Chain.DetectedPhysicsAsset->GetName() : FString(), FString(TEXT("PA_LunaMk2_SideTail")));
		}
	}
	TestEqual(TEXT("Both side-tail roots are detected"), FoundRoots.Num(), 2);

	bool bFoundAnimBlueprint = false;
	for (const ChainPhysicsSetup::FAnimBlueprintCandidate& Candidate : Analysis.AnimBlueprints)
	{
		if (Candidate.AnimBlueprint.IsValid() && Candidate.AnimBlueprint->GetName() == TEXT("ABP_LunaMk2"))
		{
			bFoundAnimBlueprint = true;
			TestTrue(TEXT("ABP_LunaMk2 has a connected Rigid Body node"), Candidate.bHasConnectedRigidBody);
			TestEqual(TEXT("ABP_LunaMk2 uses the expected PA"),
				Candidate.DetectedPhysicsAsset.IsValid() ? Candidate.DetectedPhysicsAsset->GetName() : FString(), FString(TEXT("PA_LunaMk2_SideTail")));
		}
	}
	TestTrue(TEXT("ABP_LunaMk2 is compatible"), bFoundAnimBlueprint);

	for (ChainPhysicsSetup::FChainCandidate& Chain : Analysis.Chains)
	{
		Chain.bSelected = ExpectedRoots.Contains(Chain.RootBone);
	}
	for (ChainPhysicsSetup::FAnimBlueprintCandidate& Candidate : Analysis.AnimBlueprints)
	{
		Candidate.bSelected = Candidate.AnimBlueprint.IsValid() && Candidate.AnimBlueprint->GetName() == TEXT("ABP_LunaMk2");
	}
	const ChainPhysicsSetup::FSetupResult IdempotentResult = ChainPhysicsSetup::SetupSelectedChains(Analysis, ChainPhysicsSetup::FSetupOptions());
	TestTrue(TEXT("Repairing the detected setup succeeds"), IdempotentResult.bSucceeded);
	TestFalse(TEXT("Repairing the detected setup does not modify assets"), IdempotentResult.bModified);
	return true;
}

#endif
