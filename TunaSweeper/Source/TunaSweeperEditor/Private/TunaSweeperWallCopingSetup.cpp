#include "TunaSweeperWallCopingSetup.h"

#include "TunaSweeperEditorSetupShared.h"

#include "Dom/JsonObject.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionPerInstanceRandom.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "MeshDescriptionBuilder.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/BoxElem.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "WallCopingSplineActor.h"

namespace TunaSweeperWallCopingSetup
{
	const FString AssetPath = TEXT("/Game/Environment/Architecture/WallCoping");
	const FString MeshName = TEXT("SM_WallCoping_Straight");
	const FString MaterialName = TEXT("M_WallCoping_WarmSandstone");
	const FString BlueprintName = TEXT("BP_WallCopingSpline");

	struct FParameters
	{
		double Length = 50.0;
		double Width = 38.0;
		double Height = 14.0;
		double EdgeBevel = 3.0;
		double EndBevel = 3.5;
		double HandmadeOffset = 0.3;
		double UVTile = 50.0;
	};

	struct FRing
	{
		double X = 0.0;
		double HalfWidth = 0.0;
		double MinZ = 0.0;
		double MaxZ = 0.0;
		double CornerBevel = 0.0;
		double YOffset = 0.0;
	};

	double ReadNumber(const TSharedPtr<FJsonObject>& Object, const TCHAR* Field, const double DefaultValue)
	{
		double Value = DefaultValue;
		return Object.IsValid() && Object->TryGetNumberField(Field, Value) ? Value : DefaultValue;
	}

	bool LoadParameters(FParameters& OutParameters)
	{
		const FString SourcePath = FPaths::Combine(
			FPaths::ProjectDir(),
			TEXT("SourceArt/Environment/WallCoping/wall_coping_parameters.json"));
		FString JsonText;
		if (!FFileHelper::LoadFileToString(JsonText, *SourcePath))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to read wall coping parameters: %s"), *SourcePath);
			return false;
		}

		TSharedPtr<FJsonObject> Root;
		const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonText);
		if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Invalid wall coping parameter JSON: %s"), *SourcePath);
			return false;
		}

		OutParameters.Length = ReadNumber(Root, TEXT("length_cm"), OutParameters.Length);
		OutParameters.Width = ReadNumber(Root, TEXT("width_cm"), OutParameters.Width);
		OutParameters.Height = ReadNumber(Root, TEXT("height_cm"), OutParameters.Height);
		OutParameters.EdgeBevel = ReadNumber(Root, TEXT("edge_bevel_cm"), OutParameters.EdgeBevel);
		OutParameters.EndBevel = ReadNumber(Root, TEXT("end_bevel_cm"), OutParameters.EndBevel);
		OutParameters.HandmadeOffset = ReadNumber(Root, TEXT("handmade_offset_cm"), OutParameters.HandmadeOffset);
		OutParameters.UVTile = ReadNumber(Root, TEXT("uv_tile_cm"), OutParameters.UVTile);

		const double MaxBevel = FMath::Min(OutParameters.Width * 0.24, OutParameters.Height * 0.42);
		OutParameters.Length = FMath::Max(OutParameters.Length, 10.0);
		OutParameters.Width = FMath::Max(OutParameters.Width, 10.0);
		OutParameters.Height = FMath::Max(OutParameters.Height, 4.0);
		OutParameters.EdgeBevel = FMath::Clamp(OutParameters.EdgeBevel, 0.5, MaxBevel);
		OutParameters.EndBevel = FMath::Clamp(OutParameters.EndBevel, 0.5, OutParameters.Length * 0.2);
		OutParameters.HandmadeOffset = FMath::Clamp(OutParameters.HandmadeOffset, 0.0, OutParameters.Height * 0.08);
		OutParameters.UVTile = FMath::Max(OutParameters.UVTile, 1.0);
		return true;
	}

	template <typename TExpression>
	TExpression* AddExpression(UMaterial* Material, const int32 X, const int32 Y)
	{
		TExpression* Expression = NewObject<TExpression>(Material);
		Expression->Material = Material;
		Expression->MaterialExpressionEditorX = X;
		Expression->MaterialExpressionEditorY = Y;
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterial* EnsureMaterial()
	{
		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, MaterialName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UPackage* Package = CreatePackage(*(AssetPath / MaterialName));
			Material = Package
				? NewObject<UMaterial>(Package, *MaterialName, RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (Material)
			{
				FAssetRegistryModule::AssetCreated(Material);
			}
		}
		if (!Material)
		{
			return nullptr;
		}

		Material->Modify();
		Material->GetExpressionCollection().Empty();
		Material->BlendMode = BLEND_Opaque;
		Material->TwoSided = false;
		Material->bUsedWithInstancedStaticMeshes = true;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		if (!Data)
		{
			return nullptr;
		}

		UMaterialExpressionVectorParameter* DarkSand = AddExpression<UMaterialExpressionVectorParameter>(Material, -520, -100);
		DarkSand->ParameterName = TEXT("DarkSandColor");
		DarkSand->DefaultValue = FLinearColor(0.42f, 0.205f, 0.075f, 1.0f);
		UMaterialExpressionVectorParameter* LightSand = AddExpression<UMaterialExpressionVectorParameter>(Material, -520, 30);
		LightSand->ParameterName = TEXT("LightSandColor");
		LightSand->DefaultValue = FLinearColor(0.72f, 0.43f, 0.17f, 1.0f);
		UMaterialExpressionPerInstanceRandom* InstanceRandom = AddExpression<UMaterialExpressionPerInstanceRandom>(Material, -520, 160);
		UMaterialExpressionLinearInterpolate* ColorVariation = AddExpression<UMaterialExpressionLinearInterpolate>(Material, -210, -30);
		ColorVariation->A.Connect(0, DarkSand);
		ColorVariation->B.Connect(0, LightSand);
		ColorVariation->Alpha.Connect(0, InstanceRandom);
		UMaterialExpressionScalarParameter* Roughness = AddExpression<UMaterialExpressionScalarParameter>(Material, -210, 130);
		Roughness->ParameterName = TEXT("Roughness");
		Roughness->DefaultValue = 0.82f;

		Data->BaseColor.Connect(0, ColorVariation);
		Data->Roughness.Connect(0, Roughness);
		Data->Metallic.UseConstant = true;
		Data->Metallic.Constant = 0.0f;
		Data->Specular.UseConstant = true;
		Data->Specular.Constant = 0.24f;

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Material) ? Material : nullptr;
	}

	TArray<FVector> MakeRingPoints(const FRing& Ring)
	{
		const double B = FMath::Min(Ring.CornerBevel, FMath::Min(Ring.HalfWidth, (Ring.MaxZ - Ring.MinZ) * 0.5));
		return {
			FVector(Ring.X, Ring.YOffset - Ring.HalfWidth + B, Ring.MinZ),
			FVector(Ring.X, Ring.YOffset + Ring.HalfWidth - B, Ring.MinZ),
			FVector(Ring.X, Ring.YOffset + Ring.HalfWidth, Ring.MinZ + B),
			FVector(Ring.X, Ring.YOffset + Ring.HalfWidth, Ring.MaxZ - B),
			FVector(Ring.X, Ring.YOffset + Ring.HalfWidth - B, Ring.MaxZ),
			FVector(Ring.X, Ring.YOffset - Ring.HalfWidth + B, Ring.MaxZ),
			FVector(Ring.X, Ring.YOffset - Ring.HalfWidth, Ring.MaxZ - B),
			FVector(Ring.X, Ring.YOffset - Ring.HalfWidth, Ring.MinZ + B)
		};
	}

	void AddTriangle(
		FMeshDescriptionBuilder& Builder,
		const FPolygonGroupID Group,
		const FVector& P0,
		const FVector& P1,
		const FVector& P2,
		const FVector2D& UV0,
		const FVector2D& UV1,
		const FVector2D& UV2)
	{
		const FVector Normal = FVector::CrossProduct(P1 - P0, P2 - P0).GetSafeNormal();
		const FVector Tangent = (P1 - P0).GetSafeNormal();
		const FVector Positions[3] = { P0, P1, P2 };
		const FVector2D UVs[3] = { UV0, UV1, UV2 };
		FVertexInstanceID Instances[3];
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVertexID Vertex = Builder.AppendVertex(Positions[Index]);
			Instances[Index] = Builder.AppendInstance(Vertex);
			Builder.SetInstanceTangentSpace(Instances[Index], Normal, Tangent, 1.0f);
			Builder.SetInstanceUV(Instances[Index], UVs[Index], 0);
			Builder.SetInstanceColor(Instances[Index], FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
		}
		// UE uses clockwise front-face winding. Keep the authored tangent-space normal
		// pointing outward while reversing the index order used for rasterization.
		Builder.AppendTriangle(Instances[0], Instances[2], Instances[1], Group);
	}

	void AddQuad(
		FMeshDescriptionBuilder& Builder,
		const FPolygonGroupID Group,
		const FVector& P0,
		const FVector& P1,
		const FVector& P2,
		const FVector& P3,
		const FVector2D& UV0,
		const FVector2D& UV1,
		const FVector2D& UV2,
		const FVector2D& UV3)
	{
		AddTriangle(Builder, Group, P0, P1, P2, UV0, UV1, UV2);
		AddTriangle(Builder, Group, P0, P2, P3, UV0, UV2, UV3);
	}

	void BuildMeshDescription(FMeshDescription& MeshDescription, const FParameters& Parameters)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		FMeshDescriptionBuilder Builder;
		Builder.SetMeshDescription(&MeshDescription);
		Builder.SetNumUVLayers(1);
		const FPolygonGroupID Group = Builder.AppendPolygonGroup(TEXT("WarmSandstone"));

		const double HalfLength = Parameters.Length * 0.5;
		const double HalfWidth = Parameters.Width * 0.5;
		const double EndInset = FMath::Min(Parameters.EdgeBevel * 0.82, HalfWidth * 0.25);
		const double InnerX = HalfLength - Parameters.EndBevel;
		const double H = Parameters.HandmadeOffset;
		const TArray<FRing> Rings = {
			{ -HalfLength, HalfWidth - EndInset, Parameters.EdgeBevel * 0.72, Parameters.Height - Parameters.EdgeBevel * 0.72, Parameters.EdgeBevel * 0.48, 0.0 },
			{ -InnerX, HalfWidth, 0.0, Parameters.Height, Parameters.EdgeBevel, 0.0 },
			{ -Parameters.Length * 0.18, HalfWidth - H, 0.0, Parameters.Height, Parameters.EdgeBevel, H },
			{ Parameters.Length * 0.18, HalfWidth - H * 0.65, 0.0, Parameters.Height - H * 0.45, Parameters.EdgeBevel, -H * 0.65 },
			{ InnerX, HalfWidth, 0.0, Parameters.Height, Parameters.EdgeBevel, 0.0 },
			{ HalfLength, HalfWidth - EndInset, Parameters.EdgeBevel * 0.72, Parameters.Height - Parameters.EdgeBevel * 0.72, Parameters.EdgeBevel * 0.48, 0.0 }
		};

		TArray<TArray<FVector>> Points;
		for (const FRing& Ring : Rings)
		{
			Points.Add(MakeRingPoints(Ring));
		}

		TArray<double> PerimeterDistances;
		PerimeterDistances.Add(0.0);
		for (int32 SideIndex = 0; SideIndex < 8; ++SideIndex)
		{
			PerimeterDistances.Add(PerimeterDistances.Last() +
				FVector::Distance(Points[1][SideIndex], Points[1][(SideIndex + 1) % 8]));
		}

		for (int32 RingIndex = 0; RingIndex + 1 < Points.Num(); ++RingIndex)
		{
			const double U0 = (Rings[RingIndex].X + HalfLength) / Parameters.UVTile;
			const double U1 = (Rings[RingIndex + 1].X + HalfLength) / Parameters.UVTile;
			for (int32 SideIndex = 0; SideIndex < 8; ++SideIndex)
			{
				const int32 NextSide = (SideIndex + 1) % 8;
				const double V0 = PerimeterDistances[SideIndex] / Parameters.UVTile;
				const double V1 = PerimeterDistances[SideIndex + 1] / Parameters.UVTile;
				AddQuad(
					Builder, Group,
					Points[RingIndex][SideIndex], Points[RingIndex][NextSide],
					Points[RingIndex + 1][NextSide], Points[RingIndex + 1][SideIndex],
					FVector2D(U0, V0), FVector2D(U0, V1), FVector2D(U1, V1), FVector2D(U1, V0));
			}
		}

		const FVector LeftCenter(-HalfLength, 0.0, Parameters.Height * 0.5);
		const FVector RightCenter(HalfLength, 0.0, Parameters.Height * 0.5);
		for (int32 SideIndex = 0; SideIndex < 8; ++SideIndex)
		{
			const int32 NextSide = (SideIndex + 1) % 8;
			auto CapUV = [&Parameters](const FVector& P)
			{
				return FVector2D(P.Y / Parameters.UVTile + 0.5, P.Z / Parameters.UVTile);
			};
			AddTriangle(Builder, Group,
				LeftCenter, Points[0][NextSide], Points[0][SideIndex],
				CapUV(LeftCenter), CapUV(Points[0][NextSide]), CapUV(Points[0][SideIndex]));
			AddTriangle(Builder, Group,
				RightCenter, Points.Last()[SideIndex], Points.Last()[NextSide],
				CapUV(RightCenter), CapUV(Points.Last()[SideIndex]), CapUV(Points.Last()[NextSide]));
		}
	}

	UStaticMesh* EnsureMesh(UMaterialInterface* Material, const FParameters& Parameters)
	{
		if (!Material)
		{
			return nullptr;
		}
		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, MeshName);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!Mesh)
		{
			UPackage* Package = CreatePackage(*(AssetPath / MeshName));
			Mesh = Package
				? NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (Mesh)
			{
				FAssetRegistryModule::AssetCreated(Mesh);
			}
		}
		if (!Mesh)
		{
			return nullptr;
		}

		FMeshDescription MeshDescription;
		BuildMeshDescription(MeshDescription, Parameters);
		Mesh->Modify();
		Mesh->GetStaticMaterials().Reset();
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, TEXT("WarmSandstone")));

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = true;
		BuildParams.bUseHashAsGuid = true;
		const TArray<const FMeshDescription*> MeshDescriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build %s."), *ObjectPath);
			return nullptr;
		}

		Mesh->CreateBodySetup();
		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			BodySetup->Modify();
			BodySetup->AggGeom.EmptyElements();
			FKBoxElem Box;
			Box.Center = FVector(0.0, 0.0, Parameters.Height * 0.5);
			Box.X = Parameters.Length;
			Box.Y = Parameters.Width;
			Box.Z = Parameters.Height;
			BodySetup->AggGeom.BoxElems.Add(Box);
			BodySetup->CollisionTraceFlag = CTF_UseDefault;
			BodySetup->bNeverNeedsCookedCollisionData = false;
			BodySetup->InvalidatePhysicsData();
			BodySetup->CreatePhysicsMeshes();
		}

		if (Mesh->GetNumSourceModels() > 0)
		{
			FMeshBuildSettings& Settings = Mesh->GetSourceModel(0).BuildSettings;
			Settings.bRecomputeNormals = false;
			Settings.bRecomputeTangents = false;
			Settings.bGenerateLightmapUVs = false;
		}
		Mesh->GetNaniteSettings().bEnabled = false;
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Mesh) ? Mesh : nullptr;
	}

	UBlueprint* EnsureCopingBlueprint(UStaticMesh* Mesh, UMaterialInterface* Material, const FParameters& Parameters)
	{
		UBlueprint* Blueprint = TunaSweeperEditorSetup::EnsureBlueprint(
			AssetPath,
			BlueprintName,
			AWallCopingSplineActor::StaticClass());
		if (!Blueprint || !Mesh || !Material)
		{
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		AWallCopingSplineActor* Defaults = Blueprint->GeneratedClass
			? Cast<AWallCopingSplineActor>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			return nullptr;
		}

		Blueprint->Modify();
		Defaults->Modify();
		Defaults->CopingMesh = Mesh;
		Defaults->MaterialOverride = Material;
		Defaults->bUseMeshBoundsForModuleLength = true;
		Defaults->ModuleLength = Parameters.Length;
		Defaults->WidthScale = 1.0;
		Defaults->HeightScale = 1.0;
		Defaults->Gap = 0.0;
		Defaults->AlignmentPolicy = EWallCopingAlignmentPolicy::Centered;
		Defaults->bFollowSplinePitchAndRoll = false;
		Defaults->Seed = 1337;
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	void LogExistingWallBounds()
	{
		const TArray<FString> Names = {
			TEXT("SM_Agit_D_000"), TEXT("SM_Agit_D_001"), TEXT("SM_Agit_D_002"),
			TEXT("SM_Agit_D_003"), TEXT("SM_Agit_D_004"), TEXT("SM_Agit_D_005"), TEXT("SM_Agit_D_006")
		};
		for (const FString& Name : Names)
		{
			const FString ObjectPath = FString::Printf(
				TEXT("/Game/Environment/Bunker/Agit/%s.%s"), *Name, *Name);
			if (const UStaticMesh* ExistingMesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath))
			{
				UE_LOG(LogTunaSweeperEditor, Display, TEXT("Wall-coping survey %s bounds=%s"),
					*Name, *ExistingMesh->GetBoundingBox().GetSize().ToString());
			}
		}
	}

	bool Run()
	{
		FParameters Parameters;
		if (!LoadParameters(Parameters))
		{
			return false;
		}

		LogExistingWallBounds();
		UMaterial* Material = EnsureMaterial();
		UStaticMesh* Mesh = EnsureMesh(Material, Parameters);
		UBlueprint* Blueprint = EnsureCopingBlueprint(Mesh, Material, Parameters);
		const bool bSucceeded = Material && Mesh && Blueprint;
		UE_LOG(LogTunaSweeperEditor, Display,
			TEXT("Wall coping setup %s: dimensions %.1f x %.1f x %.1f cm, bottom-center pivot."),
			bSucceeded ? TEXT("complete") : TEXT("failed"),
			Parameters.Length, Parameters.Width, Parameters.Height);
		return bSucceeded;
	}
}
