#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Components/SplineComponent.h"
#include "Editor.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "SplineWorldBuilderActor.h"
#include "SplineWorldBuilderJunctionActor.h"
#include "SplineWorldBuilderProfile.h"

namespace
{
	const TCHAR* ProfilePath = TEXT("/SplineWorldBuilder/Profiles/DA_SWB_TestStoneWall.DA_SWB_TestStoneWall");

	ASplineWorldBuilderActor* SpawnTestChain(
		UWorld* World,
		ASplineWorldJunctionActor* Junction,
		USplineWorldBuilderProfile* Profile,
		const FVector& Direction,
		const FVector& Origin)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ASplineWorldBuilderActor* Chain = World->SpawnActor<ASplineWorldBuilderActor>(
			ASplineWorldBuilderActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Origin),
			SpawnParameters);
		if (!Chain)
		{
			return nullptr;
		}

		Chain->Profile = Profile;
		Chain->StartJunction = Junction;
		Chain->bAutoRebuild = false;
		USplineComponent* Spline = Chain->GetBuilderSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		Spline->AddSplinePoint(Direction.GetSafeNormal() * 600.0, ESplineCoordinateSpace::Local, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		Chain->RebuildGenerated();
		return Chain;
	}

	bool TransformNearlyEqual(const FTransform& A, const FTransform& B)
	{
		return A.GetLocation().Equals(B.GetLocation(), 0.01) &&
			A.GetRotation().Equals(B.GetRotation(), 0.0001) &&
			A.GetScale3D().Equals(B.GetScale3D(), 0.0001);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSplineWorldBuilderGeneratedAssetsTest,
	"SplineWorldBuilder.Editor.GeneratedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSplineWorldBuilderGeneratedAssetsTest::RunTest(const FString& Parameters)
{
	UTexture2D* Texture = LoadObject<UTexture2D>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/T_SWB_StoneBlocks.T_SWB_StoneBlocks"));
	UMaterial* Material = LoadObject<UMaterial>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/M_SWB_TestStone.M_SWB_TestStone"));
	UStaticMesh* Straight = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/SM_SWB_TestStraight.SM_SWB_TestStraight"));
	UStaticMesh* End = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/SM_SWB_TestEnd.SM_SWB_TestEnd"));
	UStaticMesh* Corner = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/SM_SWB_TestCorner.SM_SWB_TestCorner"));
	UStaticMesh* Tee = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/SM_SWB_TestTJunction.SM_SWB_TestTJunction"));
	UStaticMesh* Cross = LoadObject<UStaticMesh>(
		nullptr,
		TEXT("/SplineWorldBuilder/Generated/Internal/SM_SWB_TestCrossJunction.SM_SWB_TestCrossJunction"));
	USplineWorldBuilderProfile* Profile = LoadObject<USplineWorldBuilderProfile>(nullptr, ProfilePath);

	TestNotNull(TEXT("ImageGen stone texture exists"), Texture);
	TestNotNull(TEXT("Stone material exists"), Material);
	TestNotNull(TEXT("Straight mesh exists"), Straight);
	TestNotNull(TEXT("End mesh exists"), End);
	TestNotNull(TEXT("Corner mesh exists"), Corner);
	TestNotNull(TEXT("T junction mesh exists"), Tee);
	TestNotNull(TEXT("Cross junction mesh exists"), Cross);
	TestNotNull(TEXT("Public test profile exists"), Profile);
	if (!Texture || !Material || !Straight || !End || !Corner || !Tee || !Cross || !Profile)
	{
		return false;
	}

	TestEqual(TEXT("ImageGen source width is preserved"), Texture->Source.GetSizeX(), static_cast<int64>(1254));
	TestEqual(TEXT("ImageGen source height is preserved"), Texture->Source.GetSizeY(), static_cast<int64>(1254));
	TestEqual(
		TEXT("Non-power-of-two source is padded during texture build"),
		Texture->PowerOfTwoMode.GetValue(),
		ETexturePowerOfTwoSetting::PadToPowerOfTwo);

	UMaterialExpressionTextureSampleParameter2D* StoneSample = nullptr;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		if (UMaterialExpressionTextureSampleParameter2D* Candidate =
			Cast<UMaterialExpressionTextureSampleParameter2D>(Expression))
		{
			if (Candidate->ParameterName == TEXT("StoneTexture"))
			{
				StoneSample = Candidate;
				break;
			}
		}
	}
	TestNotNull(TEXT("Material exposes the generated stone texture"), StoneSample);
	TestTrue(TEXT("Material sample references the generated texture"), StoneSample && StoneSample->Texture == Texture);

	for (UStaticMesh* Mesh : { Straight, End, Corner, Tee, Cross })
	{
		TestTrue(TEXT("Test mesh has renderable geometry"), Mesh->GetBounds().BoxExtent.SizeSquared() > 1.0);
		TestTrue(TEXT("Test mesh uses the generated stone material"),
			Mesh->GetStaticMaterials().Num() > 0 && Mesh->GetStaticMaterials()[0].MaterialInterface == Material);
	}

	TestTrue(TEXT("Profile references straight mesh"), Profile->StraightMesh == Straight);
	TestTrue(TEXT("Profile references end mesh"), Profile->EndMesh == End);
	TestTrue(TEXT("Profile references corner mesh"), Profile->CornerMesh == Corner);
	TestTrue(TEXT("Profile references T mesh"), Profile->TJunctionMesh == Tee);
	TestTrue(TEXT("Profile references cross mesh"), Profile->CrossJunctionMesh == Cross);
	TestTrue(TEXT("Profile references generated material"), Profile->MaterialOverride == Material);

	FAssetCompilingManager::Get().FinishAllCompilation();
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSplineWorldBuilderPlacementAndJunctionTest,
	"SplineWorldBuilder.Editor.PlacementAndJunctions",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FSplineWorldBuilderPlacementAndJunctionTest::RunTest(const FString& Parameters)
{
	USplineWorldBuilderProfile* Profile = LoadObject<USplineWorldBuilderProfile>(nullptr, ProfilePath);
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Test profile loads"), Profile);
	TestNotNull(TEXT("Editor world exists"), World);
	if (!Profile || !World)
	{
		return false;
	}

	const FVector Origin(1000000.0, 1000000.0, 1000000.0);
	FActorSpawnParameters JunctionSpawnParameters;
	JunctionSpawnParameters.ObjectFlags |= RF_Transient;
	JunctionSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASplineWorldJunctionActor* Junction = World->SpawnActor<ASplineWorldJunctionActor>(
		ASplineWorldJunctionActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, Origin),
		JunctionSpawnParameters);
	TestNotNull(TEXT("Junction actor spawns"), Junction);
	if (!Junction)
	{
		return false;
	}
	Junction->Profile = Profile;
	Junction->bAutoRebuild = false;

	TArray<ASplineWorldBuilderActor*> Chains;
	for (const FVector& Direction : {
		FVector::ForwardVector,
		-FVector::ForwardVector,
		FVector::RightVector,
		-FVector::RightVector })
	{
		Chains.Add(SpawnTestChain(World, Junction, Profile, Direction, Origin));
	}
	for (ASplineWorldBuilderActor* Chain : Chains)
	{
		TestNotNull(TEXT("Chain actor spawns"), Chain);
	}
	if (Chains.Contains(nullptr))
	{
		Junction->Destroy();
		return false;
	}

	TestEqual(TEXT("Junction trim and free end leave two 200 cm modules"), Chains[0]->GetStraightInstanceCount(), 2);
	TestEqual(TEXT("Free chain endpoint creates one end piece"), Chains[0]->GetEndInstanceCount(), 1);
	FTransform FirstBuildTransform;
	FTransform SecondBuildTransform;
	TestTrue(TEXT("First generated transform is readable"), Chains[0]->GetStraightInstanceTransform(0, FirstBuildTransform));
	Chains[0]->RebuildGenerated();
	TestTrue(TEXT("Second generated transform is readable"), Chains[0]->GetStraightInstanceTransform(0, SecondBuildTransform));
	TestTrue(TEXT("Repeated rebuild is deterministic"), TransformNearlyEqual(FirstBuildTransform, SecondBuildTransform));

	auto SetConnections = [Junction, &Chains](std::initializer_list<int32> Indices)
	{
		Junction->Connections.Reset();
		for (const int32 Index : Indices)
		{
			FSplineWorldJunctionConnection& Connection = Junction->Connections.AddDefaulted_GetRef();
			Connection.Chain = Chains[Index];
			Connection.Endpoint = ESplineWorldEndpoint::Start;
		}
		Junction->RebuildGenerated();
	};

	SetConnections({ 0 });
	TestEqual(TEXT("One arm resolves to End"), Junction->GetResolvedJunctionType(), ESplineWorldJunctionType::End);
	TestTrue(TEXT("End junction selects end mesh"), Junction->GetDisplayedJunctionMesh() == Profile->EndMesh);
	SetConnections({ 0, 1 });
	TestEqual(TEXT("Opposed arms resolve to Straight"), Junction->GetResolvedJunctionType(), ESplineWorldJunctionType::Straight);
	SetConnections({ 0, 2 });
	TestEqual(TEXT("Perpendicular arms resolve to Corner"), Junction->GetResolvedJunctionType(), ESplineWorldJunctionType::Corner);
	SetConnections({ 0, 1, 2 });
	TestEqual(TEXT("Three arms resolve to T"), Junction->GetResolvedJunctionType(), ESplineWorldJunctionType::Tee);
	TestTrue(TEXT("T junction selects T mesh"), Junction->GetDisplayedJunctionMesh() == Profile->TJunctionMesh);
	SetConnections({ 0, 1, 2, 3 });
	TestEqual(TEXT("Four arms resolve to Cross"), Junction->GetResolvedJunctionType(), ESplineWorldJunctionType::Cross);
	TestTrue(TEXT("Cross junction selects cross mesh"), Junction->GetDisplayedJunctionMesh() == Profile->CrossJunctionMesh);

	FActorSpawnParameters CornerSpawnParameters;
	CornerSpawnParameters.ObjectFlags |= RF_Transient;
	CornerSpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	ASplineWorldBuilderActor* CornerChain = World->SpawnActor<ASplineWorldBuilderActor>(
		ASplineWorldBuilderActor::StaticClass(),
		FTransform(FRotator::ZeroRotator, Origin + FVector(2000.0, 0.0, 0.0)),
		CornerSpawnParameters);
	TestNotNull(TEXT("Corner test chain spawns"), CornerChain);
	if (CornerChain)
	{
		CornerChain->Profile = Profile;
		CornerChain->bAutoRebuild = false;
		USplineComponent* CornerSpline = CornerChain->GetBuilderSpline();
		CornerSpline->ClearSplinePoints(false);
		CornerSpline->AddSplinePoint(FVector(0.0, 0.0, 0.0), ESplineCoordinateSpace::Local, false);
		CornerSpline->AddSplinePoint(FVector(400.0, 0.0, 0.0), ESplineCoordinateSpace::Local, false);
		CornerSpline->AddSplinePoint(FVector(400.0, 400.0, 0.0), ESplineCoordinateSpace::Local, false);
		for (int32 PointIndex = 0; PointIndex < 3; ++PointIndex)
		{
			CornerSpline->SetSplinePointType(PointIndex, ESplinePointType::Linear, false);
		}
		CornerSpline->UpdateSpline();
		CornerChain->RebuildGenerated();
		TestEqual(TEXT("A 90 degree chain creates one corner piece"), CornerChain->GetCornerInstanceCount(), 1);
		TestEqual(TEXT("An open chain creates two free end pieces"), CornerChain->GetEndInstanceCount(), 2);
		CornerChain->Destroy();
	}

	for (ASplineWorldBuilderActor* Chain : Chains)
	{
		Chain->Destroy();
	}
	Junction->Destroy();
	return !HasAnyErrors();
}

#endif
