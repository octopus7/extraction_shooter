#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AssetCompilingManager.h"
#include "Components/SplineComponent.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionPerInstanceRandom.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshAttributes.h"
#include "StaticMeshResources.h"
#include "WallCopingSplineActor.h"

namespace
{
	const TCHAR* MeshPath = TEXT("/Game/Environment/Architecture/WallCoping/SM_WallCoping_Straight.SM_WallCoping_Straight");
	const TCHAR* MaterialPath = TEXT("/Game/Environment/Architecture/WallCoping/M_WallCoping_WarmSandstone.M_WallCoping_WarmSandstone");
	const TCHAR* BlueprintPath = TEXT("/Game/Environment/Architecture/WallCoping/BP_WallCopingSpline.BP_WallCopingSpline");

	AWallCopingSplineActor* SpawnCopingActor(UWorld* World, UStaticMesh* Mesh)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWallCopingSplineActor* Actor = World
			? World->SpawnActor<AWallCopingSplineActor>(
				AWallCopingSplineActor::StaticClass(), FTransform::Identity, SpawnParameters)
			: nullptr;
		if (Actor)
		{
			Actor->bAutoRebuild = false;
			Actor->CopingMesh = Mesh;
			Actor->bUseMeshBoundsForModuleLength = true;
			Actor->Gap = 0.0;
			Actor->MaxYawJitterDegrees = 0.0;
			Actor->MaxWidthScaleVariation = 0.0;
			Actor->MaxHeightScaleVariation = 0.0;
		}
		return Actor;
	}

	void SetSpline(
		AWallCopingSplineActor* Actor,
		const TArray<FVector>& Points,
		const ESplinePointType::Type PointType)
	{
		USplineComponent* Spline = Actor->GetCopingSpline();
		Spline->ClearSplinePoints(false);
		for (const FVector& Point : Points)
		{
			Spline->AddSplinePoint(Point, ESplineCoordinateSpace::Local, false);
		}
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			Spline->SetSplinePointType(Index, PointType, false);
		}
		Spline->UpdateSpline();
	}

	bool ValidatePlacement(
		FAutomationTestBase& Test,
		AWallCopingSplineActor* Actor,
		const TCHAR* CaseName,
		const int32 ExpectedCount)
	{
		Actor->RebuildCoping();
		if (!Test.TestEqual(FString::Printf(TEXT("%s instance count"), CaseName), Actor->GetInstanceCount(), ExpectedCount))
		{
			return false;
		}

		USplineComponent* Spline = Actor->GetCopingSpline();
		const double Length = Spline->GetSplineLength();
		const double ModuleLength = Actor->GetResolvedModuleLength();
		const double OccupiedLength = ExpectedCount * ModuleLength + FMath::Max(0, ExpectedCount - 1) * Actor->Gap;
		const bool bShort = Length < ModuleLength;
		const double Leading = bShort || Actor->AlignmentPolicy == EWallCopingAlignmentPolicy::Centered
			? (Length - OccupiedLength) * 0.5
			: 0.0;

		for (int32 Index = 0; Index < ExpectedCount; ++Index)
		{
			FTransform Transform;
			if (!Test.TestTrue(FString::Printf(TEXT("%s transform %d"), CaseName, Index),
				Actor->GetInstanceTransform(Index, Transform, false)))
			{
				return false;
			}
			const double Distance = FMath::Clamp(
				Leading + ModuleLength * 0.5 + Index * (ModuleLength + Actor->Gap),
				0.0,
				Length);
			const FVector ExpectedLocation = Spline->GetLocationAtDistanceAlongSpline(
				Distance, ESplineCoordinateSpace::Local);
			Test.TestTrue(FString::Printf(TEXT("%s follows arc distance %d"), CaseName, Index),
				Transform.GetLocation().Equals(ExpectedLocation, 0.05));
			Test.TestTrue(FString::Printf(TEXT("%s preserves rigid X scale %d"), CaseName, Index),
				FMath::IsNearlyEqual(Transform.GetScale3D().X, 1.0, UE_SMALL_NUMBER));
			Test.TestTrue(FString::Printf(TEXT("%s remains horizontal %d"), CaseName, Index),
				FMath::IsNearlyZero(Transform.Rotator().Pitch, 0.01) &&
				FMath::IsNearlyZero(Transform.Rotator().Roll, 0.01));
		}
		return !Test.HasAnyErrors();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWallCopingGeneratedAssetsTest,
	"SplineWorldBuilder.WallCoping.GeneratedAssets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallCopingGeneratedAssetsTest::RunTest(const FString& Parameters)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	UMaterial* Material = LoadObject<UMaterial>(nullptr, MaterialPath);
	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintPath);
	TestNotNull(TEXT("Wall coping mesh exists"), Mesh);
	TestNotNull(TEXT("Warm sandstone material exists"), Material);
	TestNotNull(TEXT("Designer Blueprint exists"), Blueprint);
	if (!Mesh || !Material || !Blueprint)
	{
		return false;
	}
	FAssetCompilingManager::Get().FinishAllCompilation();

	const FBox Bounds = Mesh->GetBoundingBox();
	TestTrue(TEXT("Mesh length is 50 cm"), FMath::IsNearlyEqual(Bounds.GetSize().X, 50.0, 0.05));
	TestTrue(TEXT("Mesh width is 38 cm"), FMath::IsNearlyEqual(Bounds.GetSize().Y, 38.0, 0.05));
	TestTrue(TEXT("Mesh height is 14 cm"), FMath::IsNearlyEqual(Bounds.GetSize().Z, 14.0, 0.05));
	TestTrue(TEXT("Pivot is centered in X"), FMath::IsNearlyZero(Bounds.GetCenter().X, 0.01));
	TestTrue(TEXT("Pivot is centered in Y"), FMath::IsNearlyZero(Bounds.GetCenter().Y, 0.01));
	TestTrue(TEXT("Pivot lies on bottom plane"), FMath::IsNearlyZero(Bounds.Min.Z, 0.01));
	TestFalse(TEXT("Nanite is disabled for the small module"), Mesh->GetNaniteSettings().bEnabled);
	TestTrue(TEXT("Mesh uses warm sandstone material"),
		Mesh->GetStaticMaterials().Num() == 1 && Mesh->GetStaticMaterials()[0].MaterialInterface == Material);
	const FStaticMeshRenderData* RenderData = Mesh->GetRenderData();
	TestNotNull(TEXT("Mesh has render data"), RenderData);
	if (RenderData && RenderData->LODResources.Num() > 0)
	{
		const FStaticMeshLODResources& LOD = RenderData->LODResources[0];
		TestEqual(TEXT("Mesh uses the intended compact 96-triangle density"), LOD.GetNumTriangles(), 96u);
		TestTrue(TEXT("Mesh has authored UVs"), LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords() >= 1);
		const FVector3f FirstNormal = LOD.VertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(0);
		TestTrue(TEXT("Mesh has a normalized authored normal"), FMath::IsNearlyEqual(FirstNormal.Size(), 1.0f, 0.01f));
	}
	const FMeshDescription* MeshDescription = Mesh->GetMeshDescription(0);
	TestNotNull(TEXT("Mesh retains its source description"), MeshDescription);
	if (MeshDescription)
	{
		FStaticMeshConstAttributes Attributes(*MeshDescription);
		const TVertexAttributesConstRef<FVector3f> Positions = Attributes.GetVertexPositions();
		const TVertexInstanceAttributesConstRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
		bool bAllFacesUseClockwiseOutwardWinding = true;
		for (const FTriangleID TriangleId : MeshDescription->Triangles().GetElementIDs())
		{
			const TArrayView<const FVertexInstanceID> Instances =
				MeshDescription->GetTriangleVertexInstances(TriangleId);
			const FVector3f P0 = Positions[MeshDescription->GetVertexInstanceVertex(Instances[0])];
			const FVector3f P1 = Positions[MeshDescription->GetVertexInstanceVertex(Instances[1])];
			const FVector3f P2 = Positions[MeshDescription->GetVertexInstanceVertex(Instances[2])];
			const FVector3f GeometricCross = FVector3f::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
			const FVector3f AuthoredOutwardNormal =
				(Normals[Instances[0]] + Normals[Instances[1]] + Normals[Instances[2]]).GetSafeNormal();
			bAllFacesUseClockwiseOutwardWinding &=
				FVector3f::DotProduct(GeometricCross, AuthoredOutwardNormal) < -0.99f;
		}
		TestTrue(TEXT("Every face uses UE clockwise winding with an outward authored normal"),
			bAllFacesUseClockwiseOutwardWinding);
	}

	const UBodySetup* BodySetup = Mesh->GetBodySetup();
	TestNotNull(TEXT("Mesh has body setup"), BodySetup);
	TestEqual(TEXT("Mesh has one box collision"), BodySetup ? BodySetup->AggGeom.BoxElems.Num() : 0, 1);
	if (BodySetup && BodySetup->AggGeom.BoxElems.Num() == 1)
	{
		const FKBoxElem& Box = BodySetup->AggGeom.BoxElems[0];
		TestTrue(TEXT("Collision length matches mesh"), FMath::IsNearlyEqual(Box.X, 50.0, 0.05));
		TestTrue(TEXT("Collision width matches mesh"), FMath::IsNearlyEqual(Box.Y, 38.0, 0.05));
		TestTrue(TEXT("Collision height matches mesh"), FMath::IsNearlyEqual(Box.Z, 14.0, 0.05));
	}

	bool bHasPerInstanceRandom = false;
	for (UMaterialExpression* Expression : Material->GetExpressions())
	{
		bHasPerInstanceRandom |= Cast<UMaterialExpressionPerInstanceRandom>(Expression) != nullptr;
	}
	TestTrue(TEXT("Material varies sandstone color per HISM instance"), bHasPerInstanceRandom);
	TestTrue(TEXT("Blueprint derives from dedicated actor"),
		Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(AWallCopingSplineActor::StaticClass()));
	const AWallCopingSplineActor* Defaults = Blueprint->GeneratedClass
		? Cast<AWallCopingSplineActor>(Blueprint->GeneratedClass->GetDefaultObject())
		: nullptr;
	TestNotNull(TEXT("Blueprint has dedicated actor defaults"), Defaults);
	TestTrue(TEXT("Blueprint defaults to generated mesh"), Defaults && Defaults->CopingMesh == Mesh);
	TestTrue(TEXT("Blueprint defaults to generated material"), Defaults && Defaults->MaterialOverride == Material);
	TestTrue(TEXT("Blueprint defaults to horizontal blocks"), Defaults && !Defaults->bFollowSplinePitchAndRoll);
	TestTrue(TEXT("Blueprint defaults to centered fitting"),
		Defaults && Defaults->AlignmentPolicy == EWallCopingAlignmentPolicy::Centered);
	return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWallCopingPlacementTest,
	"SplineWorldBuilder.WallCoping.Placement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWallCopingPlacementTest::RunTest(const FString& Parameters)
{
	UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	TestNotNull(TEXT("Wall coping mesh exists"), Mesh);
	TestNotNull(TEXT("Editor world exists"), World);
	if (!Mesh || !World)
	{
		return false;
	}

	AWallCopingSplineActor* Actor = SpawnCopingActor(World, Mesh);
	TestNotNull(TEXT("Transient wall coping actor spawned"), Actor);
	if (!Actor)
	{
		return false;
	}

	SetSpline(Actor, { FVector::ZeroVector, FVector(30.0, 0.0, 0.0) }, ESplinePointType::Linear);
	ValidatePlacement(*this, Actor, TEXT("Short straight"), 1);

	SetSpline(Actor, { FVector::ZeroVector, FVector(260.0, 0.0, 0.0) }, ESplinePointType::Linear);
	Actor->AlignmentPolicy = EWallCopingAlignmentPolicy::Centered;
	ValidatePlacement(*this, Actor, TEXT("Long centered straight"), 5);
	Actor->AlignmentPolicy = EWallCopingAlignmentPolicy::StartAligned;
	ValidatePlacement(*this, Actor, TEXT("Long start-aligned straight"), 5);

	Actor->AlignmentPolicy = EWallCopingAlignmentPolicy::Centered;
	SetSpline(Actor,
		{ FVector::ZeroVector, FVector(150.0, 70.0, 0.0), FVector(300.0, 0.0, 0.0) },
		ESplinePointType::Curve);
	const int32 GentleCount = FMath::FloorToInt(Actor->GetCopingSpline()->GetSplineLength() / 50.0);
	ValidatePlacement(*this, Actor, TEXT("Gentle curve"), GentleCount);

	SetSpline(Actor,
		{ FVector::ZeroVector, FVector(100.0, 0.0, 0.0), FVector(100.0, 100.0, 0.0) },
		ESplinePointType::Linear);
	ValidatePlacement(*this, Actor, TEXT("Sharp 90-degree corner"), 4);

	SetSpline(Actor,
		{ FVector::ZeroVector, FVector(55.0, 75.0, 0.0), FVector(0.0, 150.0, 0.0) },
		ESplinePointType::Curve);
	const int32 TightCount = FMath::FloorToInt(Actor->GetCopingSpline()->GetSplineLength() / 50.0);
	ValidatePlacement(*this, Actor, TEXT("Tight curve"), TightCount);

	World->DestroyActor(Actor);
	return !HasAnyErrors();
}

#endif
