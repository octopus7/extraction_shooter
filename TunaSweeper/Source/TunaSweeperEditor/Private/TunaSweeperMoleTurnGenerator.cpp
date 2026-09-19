// One-off asset conversion; remove immediately after the generated assets are committed.
#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/BlendSpace.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperMoleTurnGenerator,
	"TunaSweeper.Generate.MoleDirectionalTurn",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperMoleTurnGenerator::RunTest(const FString& Parameters)
{
	const FString Folder = TEXT("/Game/Characters/NPC/Mole/");
	UAnimSequence* Idle = LoadObject<UAnimSequence>(nullptr, *(Folder + TEXT("A_Mole_Idle_Breathe")));
	if (!TestNotNull(TEXT("Existing idle"), Idle)) return false;
	TArray<FTransform> IdleRootKeys;
	Idle->GetDataModel()->GetBoneTrackTransforms(TEXT("root"), IdleRootKeys);
	if (!TestTrue(TEXT("Idle root track exists"), !IdleRootKeys.IsEmpty())) return false;
	auto Save = [this](UObject* Asset)
	{
		Asset->MarkPackageDirty();
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		const FString Filename = FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension());
		return TestTrue(TEXT("Saved directional animation asset"), UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, Args));
	};
	TArray<UAnimSequence*> Turns;
	for (const TCHAR* Direction : {TEXT("Left"), TEXT("Right")})
	{
		const FString SourceName = FString::Printf(TEXT("A_Mole_Turn_%s_90"), Direction);
		const FString TargetName = FString::Printf(TEXT("A_Mole_Turn_%s_InPlace"), Direction);
		UAnimSequence* Source = LoadObject<UAnimSequence>(nullptr, *(Folder + SourceName));
		if (!TestNotNull(TEXT("Authored directional turn exists"), Source)) return false;
		if (FPackageName::DoesPackageExist(Folder + TargetName))
		{
			AddError(TEXT("Refusing to overwrite an existing directional clip"));
			return false;
		}
		UPackage* Package = CreatePackage(*(Folder + TargetName));
		UAnimSequence* Clip = DuplicateObject<UAnimSequence>(Source, Package, *TargetName);
		Clip->SetFlags(RF_Public | RF_Standalone);
		IAnimationDataModel* Model = Clip->GetDataModel();
		TArray<FTransform> RootKeys;
		Model->GetBoneTrackTransforms(TEXT("root"), RootKeys);
		if (!TestTrue(TEXT("Root track exists"), !RootKeys.IsEmpty())) return false;
		TArray<FVector> Positions, Scales;
		TArray<FQuat> Rotations;
		// Keep neutral raw keys and the idle's reference-pose root lock. The lock
		// preserves the skeleton's 100x Blender-to-Unreal unit conversion at runtime.
		Positions.Init(IdleRootKeys[0].GetTranslation(), Model->GetNumberOfKeys());
		Rotations.Init(IdleRootKeys[0].GetRotation(), Model->GetNumberOfKeys());
		Scales.Init(IdleRootKeys[0].GetScale3D(), Model->GetNumberOfKeys());
		if (!TestTrue(TEXT("Removed baked heading from every frame"), Clip->GetController().SetBoneTrackKeys(TEXT("root"), Positions, Rotations, Scales, false))) return false;
		Clip->bEnableRootMotion = false;
		Clip->bForceRootLock = Idle->bForceRootLock;
		Clip->RootMotionRootLock = Idle->RootMotionRootLock;
		Clip->PostEditChange();
		FAssetRegistryModule::AssetCreated(Clip);
		if (!Save(Clip)) return false;
		Turns.Add(Clip);
	}
	UBlendSpace* Blend = LoadObject<UBlendSpace>(nullptr, *(Folder + TEXT("BS_Mole_IdleTurn")));
	if (!TestNotNull(TEXT("Existing blend"), Blend)) return false;
	while (Blend->GetNumberOfBlendSamples() > 0) Blend->DeleteSample(Blend->GetNumberOfBlendSamples() - 1);
	FBlendParameter& Axis = const_cast<FBlendParameter&>(Blend->GetBlendParameter(0));
	Axis.Min = -1.0f;
	Axis.Max = 1.0f;
	Axis.GridNum = 4;
	Blend->AddSample(Turns[0], FVector(-1, 0, 0));
	Blend->AddSample(Idle, FVector::ZeroVector);
	Blend->AddSample(Turns[1], FVector(1, 0, 0));
	Blend->ValidateSampleData();
	Blend->ResampleData();
	Blend->PostEditChange();
	return Save(Blend);
}
#endif
