#include "TunaSweeperBoilingPotSetup.h"

#include "TunaSweeperEditorSetupShared.h"

#include "Environment/TunaSweeperBoilingPotActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "MeshDescriptionBuilder.h"
#include "NiagaraSystem.h"
#include "Stateless/NiagaraStatelessDistribution.h"
#include "UObject/UObjectHash.h"
#include "UObject/UnrealType.h"

namespace TunaSweeperBoilingPotSetup
{
	const FString AssetPath = TEXT("/Game/Environment/Props/BoilingPot");
	const FString TextureName = TEXT("T_BoilingPot_Enamel");
	const FString MaterialName = TEXT("M_BoilingPot_Enamel");
	const FString SteamOpacityTextureName = TEXT("T_BoilingPot_SteamOpacity");
	const FString SteamMaterialName = TEXT("M_BoilingPot_SteamWhite");
	const FString BodyMeshName = TEXT("SM_BoilingPot_Body");
	const FString LidMeshName = TEXT("SM_BoilingPot_Lid");
	const FString SteamSystemName = TEXT("NS_BoilingPot_Steam");
	const FString BlueprintName = TEXT("BP_BoilingPot");
	const FName MaterialSlotName(TEXT("EnamelMetal"));
	constexpr int32 RadialSegments = 64;

	FString GetTextureSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectContentDir(),
			TEXT("SourceArt"),
			TEXT("BoilingPot"),
			TEXT("T_BoilingPot_Enamel_Source.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	FString GetSteamOpacityTextureSourcePath()
	{
		FString SourcePath = FPaths::ConvertRelativePathToFull(FPaths::Combine(
			FPaths::ProjectContentDir(),
			TEXT("SourceArt"),
			TEXT("BoilingPot"),
			TEXT("T_BoilingPot_SteamOpacity_Source.png")));
		FPaths::CollapseRelativeDirectories(SourcePath);
		return SourcePath;
	}

	template <typename TExpression>
	TExpression* AddMaterialExpression(UMaterial* Material, int32 EditorX, int32 EditorY)
	{
		TExpression* Expression = NewObject<TExpression>(Material);
		Expression->Material = Material;
		Expression->MaterialExpressionEditorX = EditorX;
		Expression->MaterialExpressionEditorY = EditorY;
		Material->GetExpressionCollection().AddExpression(Expression);
		return Expression;
	}

	UMaterial* EnsurePotMaterial(UTexture2D* Texture)
	{
		if (!Texture)
		{
			return nullptr;
		}

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
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		if (!Data)
		{
			return nullptr;
		}

		UMaterialExpressionTextureCoordinate* TextureCoordinate =
			AddMaterialExpression<UMaterialExpressionTextureCoordinate>(Material, -620, -80);
		TextureCoordinate->CoordinateIndex = 0;

		UMaterialExpressionTextureSampleParameter2D* TextureSample =
			AddMaterialExpression<UMaterialExpressionTextureSampleParameter2D>(Material, -370, -100);
		TextureSample->ParameterName = TEXT("EnamelBaseColor");
		TextureSample->Texture = Texture;
		TextureSample->SamplerType = SAMPLERTYPE_Color;
		TextureSample->Coordinates.Connect(0, TextureCoordinate);
		TextureSample->AutoSetSampleType();

		UMaterialExpressionScalarParameter* Roughness =
			AddMaterialExpression<UMaterialExpressionScalarParameter>(Material, -350, 100);
		Roughness->ParameterName = TEXT("Roughness");
		Roughness->DefaultValue = 0.54f;

		UMaterialExpressionScalarParameter* Metallic =
			AddMaterialExpression<UMaterialExpressionScalarParameter>(Material, -350, 210);
		Metallic->ParameterName = TEXT("Metallic");
		Metallic->DefaultValue = 0.72f;

		Data->BaseColor.Connect(0, TextureSample);
		Data->Roughness.Connect(0, Roughness);
		Data->Metallic.Connect(0, Metallic);
		Data->Specular.UseConstant = true;
		Data->Specular.Constant = 0.45f;

		Material->PostEditChange();
		Material->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Material) ? Material : nullptr;
	}

	FVector MakeRadialPoint(float Radius, float Angle, float Z)
	{
		return FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, Z);
	}

	void AddTriangle(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC)
	{
		FVector Normal = FVector::CrossProduct(B - A, C - A).GetSafeNormal();
		if (Normal.IsNearlyZero())
		{
			Normal = FVector::UpVector;
		}
		FVector Tangent = (B - A).GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = FVector::ForwardVector;
		}

		const FVector Positions[3] = { A, B, C };
		const FVector2D UVs[3] = { UVA, UVB, UVC };
		FVertexInstanceID Instances[3];
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVertexID Vertex = Builder.AppendVertex(Positions[Index]);
			Instances[Index] = Builder.AppendInstance(Vertex);
			Builder.SetInstanceTangentSpace(Instances[Index], Normal, Tangent, 1.0f);
			Builder.SetInstanceUV(Instances[Index], UVs[Index], 0);
			Builder.SetInstanceColor(Instances[Index], FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
		}
		Builder.AppendTriangle(Instances[0], Instances[2], Instances[1], Group);
	}

	void AddSmoothTriangle(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& NormalA,
		const FVector& NormalB,
		const FVector& NormalC,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC)
	{
		const FVector Positions[3] = { A, B, C };
		const FVector Normals[3] = {
			NormalA.GetSafeNormal(),
			NormalB.GetSafeNormal(),
			NormalC.GetSafeNormal()
		};
		const FVector2D UVs[3] = { UVA, UVB, UVC };
		FVertexInstanceID Instances[3];
		for (int32 Index = 0; Index < 3; ++Index)
		{
			const FVertexID Vertex = Builder.AppendVertex(Positions[Index]);
			Instances[Index] = Builder.AppendInstance(Vertex);
			FVector Tangent = B - A;
			Tangent -= Normals[Index] * FVector::DotProduct(Tangent, Normals[Index]);
			if (!Tangent.Normalize())
			{
				Tangent = FVector::CrossProduct(FVector::UpVector, Normals[Index]).GetSafeNormal();
			}
			if (Tangent.IsNearlyZero())
			{
				Tangent = FVector::ForwardVector;
			}
			Builder.SetInstanceTangentSpace(Instances[Index], Normals[Index], Tangent, 1.0f);
			Builder.SetInstanceUV(Instances[Index], UVs[Index], 0);
			Builder.SetInstanceColor(Instances[Index], FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
		}
		Builder.AppendTriangle(Instances[0], Instances[2], Instances[1], Group);
	}

	void AddQuad(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FVector2D& UVA = FVector2D(0.0, 0.0),
		const FVector2D& UVB = FVector2D(1.0, 0.0),
		const FVector2D& UVC = FVector2D(1.0, 1.0),
		const FVector2D& UVD = FVector2D(0.0, 1.0))
	{
		AddTriangle(Builder, Group, A, B, C, UVA, UVB, UVC);
		AddTriangle(Builder, Group, A, C, D, UVA, UVC, UVD);
	}

	void AddSmoothQuad(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		const FVector& A,
		const FVector& B,
		const FVector& C,
		const FVector& D,
		const FVector& NormalA,
		const FVector& NormalB,
		const FVector& NormalC,
		const FVector& NormalD,
		const FVector2D& UVA,
		const FVector2D& UVB,
		const FVector2D& UVC,
		const FVector2D& UVD)
	{
		AddSmoothTriangle(Builder, Group, A, B, C, NormalA, NormalB, NormalC, UVA, UVB, UVC);
		AddSmoothTriangle(Builder, Group, A, C, D, NormalA, NormalC, NormalD, UVA, UVC, UVD);
	}

	void AddBox(FMeshDescriptionBuilder& Builder, FPolygonGroupID Group, const FVector& Minimum, const FVector& Maximum)
	{
		const FVector A(Minimum.X, Minimum.Y, Minimum.Z);
		const FVector B(Maximum.X, Minimum.Y, Minimum.Z);
		const FVector C(Maximum.X, Maximum.Y, Minimum.Z);
		const FVector D(Minimum.X, Maximum.Y, Minimum.Z);
		const FVector E(Minimum.X, Minimum.Y, Maximum.Z);
		const FVector F(Maximum.X, Minimum.Y, Maximum.Z);
		const FVector G(Maximum.X, Maximum.Y, Maximum.Z);
		const FVector H(Minimum.X, Maximum.Y, Maximum.Z);

		AddQuad(Builder, Group, A, D, C, B);
		AddQuad(Builder, Group, E, F, G, H);
		AddQuad(Builder, Group, A, B, F, E);
		AddQuad(Builder, Group, D, H, G, C);
		AddQuad(Builder, Group, A, E, H, D);
		AddQuad(Builder, Group, B, C, G, F);
	}

	void AddOuterRingSurface(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		float BottomRadius,
		float BottomZ,
		float TopRadius,
		float TopZ,
		float V0,
		float V1)
	{
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			const float U0 = static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float U1 = static_cast<float>(Segment + 1) / static_cast<float>(RadialSegments);
			const float DeltaRadius = TopRadius - BottomRadius;
			const float DeltaZ = TopZ - BottomZ;
			const FVector Normal0 = FVector(FMath::Cos(Angle0) * DeltaZ, FMath::Sin(Angle0) * DeltaZ, -DeltaRadius).GetSafeNormal();
			const FVector Normal1 = FVector(FMath::Cos(Angle1) * DeltaZ, FMath::Sin(Angle1) * DeltaZ, -DeltaRadius).GetSafeNormal();
			AddSmoothQuad(
				Builder,
				Group,
				MakeRadialPoint(BottomRadius, Angle0, BottomZ),
				MakeRadialPoint(BottomRadius, Angle1, BottomZ),
				MakeRadialPoint(TopRadius, Angle1, TopZ),
				MakeRadialPoint(TopRadius, Angle0, TopZ),
				Normal0,
				Normal1,
				Normal1,
				Normal0,
				FVector2D(U0, V0),
				FVector2D(U1, V0),
				FVector2D(U1, V1),
				FVector2D(U0, V1));
		}
	}

	void AddInnerRingSurface(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		float Radius,
		float BottomZ,
		float TopZ)
	{
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			const float U0 = static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float U1 = static_cast<float>(Segment + 1) / static_cast<float>(RadialSegments);
			const FVector Normal0(-FMath::Cos(Angle0), -FMath::Sin(Angle0), 0.0f);
			const FVector Normal1(-FMath::Cos(Angle1), -FMath::Sin(Angle1), 0.0f);
			AddSmoothQuad(
				Builder,
				Group,
				MakeRadialPoint(Radius, Angle0, BottomZ),
				MakeRadialPoint(Radius, Angle0, TopZ),
				MakeRadialPoint(Radius, Angle1, TopZ),
				MakeRadialPoint(Radius, Angle1, BottomZ),
				Normal0,
				Normal0,
				Normal1,
				Normal1,
				FVector2D(U0, 0.0),
				FVector2D(U0, 1.0),
				FVector2D(U1, 1.0),
				FVector2D(U1, 0.0));
		}
	}

	void AddTopAnnulus(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		float InnerRadius,
		float OuterRadius,
		float Z)
	{
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			auto UV = [OuterRadius](const FVector& Position)
			{
				return FVector2D(0.5 + Position.X / (OuterRadius * 2.0), 0.5 + Position.Y / (OuterRadius * 2.0));
			};
			const FVector Inner0 = MakeRadialPoint(InnerRadius, Angle0, Z);
			const FVector Outer0 = MakeRadialPoint(OuterRadius, Angle0, Z);
			const FVector Outer1 = MakeRadialPoint(OuterRadius, Angle1, Z);
			const FVector Inner1 = MakeRadialPoint(InnerRadius, Angle1, Z);
			AddQuad(Builder, Group, Inner0, Outer0, Outer1, Inner1, UV(Inner0), UV(Outer0), UV(Outer1), UV(Inner1));
		}
	}

	void AddDisc(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		float Radius,
		float Z,
		bool bFaceUp)
	{
		const FVector Center(0.0f, 0.0f, Z);
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			const FVector Point0 = MakeRadialPoint(Radius, Angle0, Z);
			const FVector Point1 = MakeRadialPoint(Radius, Angle1, Z);
			auto UV = [Radius](const FVector& Position)
			{
				return FVector2D(0.5 + Position.X / (Radius * 2.0), 0.5 + Position.Y / (Radius * 2.0));
			};
			if (bFaceUp)
			{
				AddTriangle(Builder, Group, Center, Point0, Point1, FVector2D(0.5, 0.5), UV(Point0), UV(Point1));
			}
			else
			{
				AddTriangle(Builder, Group, Center, Point1, Point0, FVector2D(0.5, 0.5), UV(Point1), UV(Point0));
			}
		}
	}

	void BuildBodyMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		FMeshDescriptionBuilder Builder;
		Builder.SetMeshDescription(&MeshDescription);
		Builder.SetNumUVLayers(1);
		const FPolygonGroupID Group = Builder.AppendPolygonGroup(MaterialSlotName);

		AddOuterRingSurface(Builder, Group, 39.0f, 0.0f, 42.0f, 5.0f, 0.0f, 0.1f);
		AddOuterRingSurface(Builder, Group, 42.0f, 5.0f, 42.0f, 49.5f, 0.1f, 0.9f);
		AddOuterRingSurface(Builder, Group, 42.0f, 49.5f, 43.0f, 52.0f, 0.9f, 1.0f);
		AddInnerRingSurface(Builder, Group, 36.5f, 6.0f, 52.0f);
		AddTopAnnulus(Builder, Group, 36.5f, 43.0f, 52.0f);
		AddDisc(Builder, Group, 39.0f, 0.0f, false);
		AddDisc(Builder, Group, 36.5f, 6.0f, true);

		AddBox(Builder, Group, FVector(-17.0f, 38.0f, 29.0f), FVector(-10.0f, 60.0f, 36.0f));
		AddBox(Builder, Group, FVector(10.0f, 38.0f, 29.0f), FVector(17.0f, 60.0f, 36.0f));
		AddBox(Builder, Group, FVector(-17.0f, 56.0f, 29.0f), FVector(17.0f, 63.0f, 36.0f));
		AddBox(Builder, Group, FVector(-17.0f, -60.0f, 29.0f), FVector(-10.0f, -38.0f, 36.0f));
		AddBox(Builder, Group, FVector(10.0f, -60.0f, 29.0f), FVector(17.0f, -38.0f, 36.0f));
		AddBox(Builder, Group, FVector(-17.0f, -63.0f, 29.0f), FVector(17.0f, -56.0f, 36.0f));
	}

	struct FLidRing
	{
		float Radius;
		float Z;
	};

	void AddLidDomeSurface(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		const TArray<FLidRing>& Rings)
	{
		const FVector Center(0.0f, 0.0f, 9.5f);
		auto GetRingNormal = [&Rings, &Center](int32 RingIndex, float Angle)
		{
			const float PreviousRadius = RingIndex > 0 ? Rings[RingIndex - 1].Radius : 0.0f;
			const float PreviousZ = RingIndex > 0 ? Rings[RingIndex - 1].Z : Center.Z;
			const int32 NextRingIndex = FMath::Min(RingIndex + 1, Rings.Num() - 1);
			const float DeltaRadius = Rings[NextRingIndex].Radius - PreviousRadius;
			const float DeltaZ = Rings[NextRingIndex].Z - PreviousZ;
			return FVector(
				FMath::Cos(Angle) * -DeltaZ,
				FMath::Sin(Angle) * -DeltaZ,
				DeltaRadius).GetSafeNormal();
		};
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			const FVector Point0 = MakeRadialPoint(Rings[0].Radius, Angle0, Rings[0].Z);
			const FVector Point1 = MakeRadialPoint(Rings[0].Radius, Angle1, Rings[0].Z);
			AddSmoothTriangle(
				Builder,
				Group,
				Center,
				Point0,
				Point1,
				FVector::UpVector,
				GetRingNormal(0, Angle0),
				GetRingNormal(0, Angle1),
				FVector2D(0.5, 0.5),
				FVector2D(0.5 + Point0.X / 88.0, 0.5 + Point0.Y / 88.0),
				FVector2D(0.5 + Point1.X / 88.0, 0.5 + Point1.Y / 88.0));
		}

		for (int32 RingIndex = 0; RingIndex + 1 < Rings.Num(); ++RingIndex)
		{
			for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
			{
				const int32 Next = (Segment + 1) % RadialSegments;
				const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
				const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
				const FVector Inner0 = MakeRadialPoint(Rings[RingIndex].Radius, Angle0, Rings[RingIndex].Z);
				const FVector Outer0 = MakeRadialPoint(Rings[RingIndex + 1].Radius, Angle0, Rings[RingIndex + 1].Z);
				const FVector Outer1 = MakeRadialPoint(Rings[RingIndex + 1].Radius, Angle1, Rings[RingIndex + 1].Z);
				const FVector Inner1 = MakeRadialPoint(Rings[RingIndex].Radius, Angle1, Rings[RingIndex].Z);
				auto UV = [](const FVector& Position)
				{
					return FVector2D(0.5 + Position.X / 88.0, 0.5 + Position.Y / 88.0);
				};
				AddSmoothQuad(
					Builder,
					Group,
					Inner0,
					Outer0,
					Outer1,
					Inner1,
					GetRingNormal(RingIndex, Angle0),
					GetRingNormal(RingIndex + 1, Angle0),
					GetRingNormal(RingIndex + 1, Angle1),
					GetRingNormal(RingIndex, Angle1),
					UV(Inner0),
					UV(Outer0),
					UV(Outer1),
					UV(Inner1));
			}
		}
	}

	void AddCylinder(
		FMeshDescriptionBuilder& Builder,
		FPolygonGroupID Group,
		float BottomRadius,
		float BottomZ,
		float TopRadius,
		float TopZ,
		bool bAddTop)
	{
		AddOuterRingSurface(Builder, Group, BottomRadius, BottomZ, TopRadius, TopZ, 0.0f, 1.0f);
		if (bAddTop)
		{
			AddDisc(Builder, Group, TopRadius, TopZ, true);
		}
	}

	void BuildLidMeshDescription(FMeshDescription& MeshDescription)
	{
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		FMeshDescriptionBuilder Builder;
		Builder.SetMeshDescription(&MeshDescription);
		Builder.SetNumUVLayers(1);
		const FPolygonGroupID Group = Builder.AppendPolygonGroup(MaterialSlotName);

		const TArray<FLidRing> Rings = {
			{ 10.0f, 9.0f },
			{ 24.0f, 7.2f },
			{ 37.0f, 3.8f },
			{ 44.0f, 0.6f }
		};
		AddLidDomeSurface(Builder, Group, Rings);
		AddOuterRingSurface(Builder, Group, 44.0f, -1.5f, 44.0f, 0.6f, 0.0f, 1.0f);
		AddDisc(Builder, Group, 44.0f, -1.5f, false);
		AddCylinder(Builder, Group, 8.0f, 8.8f, 8.0f, 12.0f, false);
		AddCylinder(Builder, Group, 8.0f, 12.0f, 6.0f, 19.0f, true);
	}

	UStaticMesh* EnsureStaticMesh(
		const FString& AssetName,
		UMaterialInterface* Material,
		TFunctionRef<void(FMeshDescription&)> BuildMeshDescription)
	{
		if (!Material)
		{
			return nullptr;
		}

		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, AssetName);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *ObjectPath);
		if (!Mesh)
		{
			UPackage* Package = CreatePackage(*(AssetPath / AssetName));
			Mesh = Package
				? NewObject<UStaticMesh>(Package, *AssetName, RF_Public | RF_Standalone | RF_Transactional)
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
		BuildMeshDescription(MeshDescription);
		Mesh->Modify();
		Mesh->GetStaticMaterials().Reset();
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, MaterialSlotName));

		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = false;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bFastBuild = true;
		BuildParams.bMarkPackageDirty = true;
		BuildParams.bUseHashAsGuid = true;
		const TArray<const FMeshDescription*> MeshDescriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams))
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to build boiling-pot mesh %s."), *ObjectPath);
			return nullptr;
		}

		if (Mesh->GetNumSourceModels() > 0)
		{
			FMeshBuildSettings& Settings = Mesh->GetSourceModel(0).BuildSettings;
			Settings.bRecomputeNormals = false;
			Settings.bRecomputeTangents = false;
			Settings.bGenerateLightmapUVs = false;
			Settings.bUseMikkTSpace = false;
		}
		Mesh->GetNaniteSettings().bEnabled = false;
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();

		UE_LOG(
			LogTunaSweeperEditor,
			Display,
			TEXT("Built %s bounds=%s"),
			*ObjectPath,
			*Mesh->GetBounds().BoxExtent.ToString());
		return TunaSweeperEditorSetup::SaveAsset(Mesh) ? Mesh : nullptr;
	}

	UTexture2D* EnsureSteamOpacityTexture()
	{
		UTexture2D* Texture = nullptr;
		const FString SourcePath = GetSteamOpacityTextureSourcePath();
		if (!TunaSweeperEditorSetup::ImportWorldTexture(
				SourcePath,
				AssetPath,
				SteamOpacityTextureName,
				&Texture) || !Texture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import boiling-pot steam opacity texture %s."), *SourcePath);
			return nullptr;
		}

		Texture->Modify();
		Texture->SRGB = false;
		Texture->CompressionSettings = TC_Masks;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->LODGroup = TEXTUREGROUP_Effects;
		Texture->AddressX = TA_Clamp;
		Texture->AddressY = TA_Clamp;
		Texture->PostEditChange();
		Texture->UpdateResource();
		Texture->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Texture) ? Texture : nullptr;
	}

	UMaterial* EnsureSteamWhiteMaterial(UTexture2D* SmokeTexture)
	{
		if (!SmokeTexture)
		{
			return nullptr;
		}

		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, SteamMaterialName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *ObjectPath);
		if (!Material)
		{
			UPackage* Package = CreatePackage(*(AssetPath / SteamMaterialName));
			Material = Package
				? NewObject<UMaterial>(Package, *SteamMaterialName, RF_Public | RF_Standalone | RF_Transactional)
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
		Material->MaterialDomain = MD_Surface;
		Material->BlendMode = BLEND_Translucent;
		Material->SetShadingModel(MSM_Unlit);
		Material->TwoSided = true;
		Material->bUsedWithNiagaraSprites = true;

		UMaterialEditorOnlyData* Data = Material->GetEditorOnlyData();
		if (!Data)
		{
			return nullptr;
		}

		UMaterialExpressionTextureSampleParameter2D* TextureSample =
			AddMaterialExpression<UMaterialExpressionTextureSampleParameter2D>(Material, -620, 0);
		TextureSample->ParameterName = TEXT("SteamOpacityTexture");
		TextureSample->Texture = SmokeTexture;
		TextureSample->AutoSetSampleType();
		UMaterialExpressionScalarParameter* Opacity =
			AddMaterialExpression<UMaterialExpressionScalarParameter>(Material, -380, 260);
		Opacity->ParameterName = TEXT("SteamOpacity");
		Opacity->DefaultValue = 0.72f;
		UMaterialExpressionParticleColor* ParticleColor =
			AddMaterialExpression<UMaterialExpressionParticleColor>(Material, -620, 150);
		UMaterialExpressionMultiply* TextureAndParticleOpacity =
			AddMaterialExpression<UMaterialExpressionMultiply>(Material, -350, 20);
		TextureAndParticleOpacity->A.Connect(1, TextureSample);
		TextureAndParticleOpacity->B.Connect(4, ParticleColor);
		UMaterialExpressionParticleRelativeTime* RelativeTime =
			AddMaterialExpression<UMaterialExpressionParticleRelativeTime>(Material, -620, 370);
		UMaterialExpressionOneMinus* LifetimeFade =
			AddMaterialExpression<UMaterialExpressionOneMinus>(Material, -380, 370);
		LifetimeFade->Input.Connect(0, RelativeTime);
		UMaterialExpressionMultiply* FadedTextureOpacity =
			AddMaterialExpression<UMaterialExpressionMultiply>(Material, -100, 50);
		FadedTextureOpacity->A.Connect(0, TextureAndParticleOpacity);
		FadedTextureOpacity->B.Connect(0, LifetimeFade);
		UMaterialExpressionMultiply* FinalOpacity =
			AddMaterialExpression<UMaterialExpressionMultiply>(Material, 140, 50);
		FinalOpacity->A.Connect(0, FadedTextureOpacity);
		FinalOpacity->B.Connect(0, Opacity);
		UMaterialExpressionConstant3Vector* WhiteColor =
			AddMaterialExpression<UMaterialExpressionConstant3Vector>(Material, -100, -130);
		WhiteColor->Constant = FLinearColor(0.88f, 0.95f, 1.0f, 1.0f);

		Data->EmissiveColor.Connect(0, WhiteColor);
		Data->Opacity.Connect(0, FinalOpacity);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Material) ? Material : nullptr;
	}

	int32 ConfigureStatelessSteamModules(UNiagaraSystem* System)
	{
		TArray<UObject*> NestedObjects;
		GetObjectsWithOuter(System, NestedObjects, true);
		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			UObject* StatelessEmitter = reinterpret_cast<UObject*>(EmitterHandle.GetStatelessEmitter());
			if (!StatelessEmitter)
			{
				continue;
			}
			StatelessEmitter->Modify();
			if (FStructProperty* FixedBoundsProperty =
				FindFProperty<FStructProperty>(StatelessEmitter->GetClass(), TEXT("FixedBounds")))
			{
				FBox* FixedBounds = FixedBoundsProperty->ContainerPtrToValuePtr<FBox>(StatelessEmitter);
				*FixedBounds = FBox(FVector(-180.0f, -180.0f, -40.0f), FVector(180.0f, 180.0f, 240.0f));
			}
			StatelessEmitter->PostEditChange();

			FArrayProperty* ModulesProperty =
				FindFProperty<FArrayProperty>(StatelessEmitter->GetClass(), TEXT("Modules"));
			FObjectPropertyBase* ModuleObjectProperty =
				ModulesProperty ? CastField<FObjectPropertyBase>(ModulesProperty->Inner) : nullptr;
			if (!ModulesProperty || !ModuleObjectProperty)
			{
				continue;
			}

			FScriptArrayHelper Modules(ModulesProperty, ModulesProperty->ContainerPtrToValuePtr<void>(StatelessEmitter));
			for (int32 ModuleIndex = 0; ModuleIndex < Modules.Num(); ++ModuleIndex)
			{
				NestedObjects.AddUnique(ModuleObjectProperty->GetObjectPropertyValue(Modules.GetRawPtr(ModuleIndex)));
			}
		}
		int32 UpdatedModuleCount = 0;

		for (UObject* Object : NestedObjects)
		{
			if (!Object)
			{
				continue;
			}

			const FName ClassName = Object->GetClass()->GetFName();
			if (ClassName == TEXT("NiagaraStatelessModule_InitializeParticle"))
			{
				Object->Modify();
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("LifetimeDistribution")))
				{
					FNiagaraDistributionRangeFloat* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeFloat>(Object);
					Distribution->InitRange(0.65f, 1.15f);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("SpriteSizeDistribution")))
				{
					FNiagaraDistributionRangeVector2* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeVector2>(Object);
					Distribution->Mode = ENiagaraDistributionMode::NonUniformRange;
					Distribution->Min = FVector2f(7.0f, 10.0f);
					Distribution->Max = FVector2f(13.0f, 18.0f);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("ColorDistribution")))
				{
					FNiagaraDistributionColor* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionColor>(Object);
					Distribution->InitConstant(FLinearColor(0.88f, 0.95f, 1.0f, 0.65f));
				}
				Object->PostEditChange();
				++UpdatedModuleCount;
			}
			else if (ClassName == TEXT("NiagaraStatelessModule_ShapeLocation"))
			{
				Object->Modify();
				if (FEnumProperty* Property = FindFProperty<FEnumProperty>(Object->GetClass(), TEXT("ShapePrimitive")))
				{
					void* ValueAddress = Property->ContainerPtrToValuePtr<void>(Object);
					Property->GetUnderlyingProperty()->SetIntPropertyValue(ValueAddress, int64(3)); // ENSM_ShapePrimitive::Ring
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("RingRadius")))
				{
					FNiagaraDistributionRangeFloat* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeFloat>(Object);
					// The Niagara component is scaled to 25%, so 152 cm becomes a 38 cm world-space ring.
					Distribution->InitConstant(152.0f);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("DiscCoverage")))
				{
					FNiagaraDistributionRangeFloat* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeFloat>(Object);
					Distribution->InitConstant(0.28f);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("RingUDistribution")))
				{
					FNiagaraDistributionRangeFloat* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeFloat>(Object);
					Distribution->InitConstant(0.0f);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("ShapeRotation")))
				{
					FNiagaraDistributionRangeRotator* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeRotator>(Object);
					Distribution->InitConstant(FRotator3f::ZeroRotator);
				}
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("ShapeScale")))
				{
					FNiagaraDistributionRangeVector3* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionRangeVector3>(Object);
					Distribution->InitConstant(FVector3f::OneVector);
				}
				Object->PostEditChange();
				++UpdatedModuleCount;
			}
			else if (ClassName == TEXT("NiagaraStatelessModule_ScaleSpriteSize"))
			{
				Object->Modify();
				if (FStructProperty* Property = FindFProperty<FStructProperty>(Object->GetClass(), TEXT("ScaleDistribution")))
				{
					FNiagaraDistributionVector2* Distribution =
						Property->ContainerPtrToValuePtr<FNiagaraDistributionVector2>(Object);
					Distribution->InitConstant(FVector2f(1.0f, 1.0f));
				}
				Object->PostEditChange();
				++UpdatedModuleCount;
			}
			else if (ClassName == TEXT("NiagaraStatelessModule_SubUVAnimation"))
			{
				Object->Modify();
				if (FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), TEXT("NumFrames")))
				{
					Property->SetPropertyValue_InContainer(Object, 1);
				}
				Object->PostEditChange();
				++UpdatedModuleCount;
			}
		}

		return UpdatedModuleCount;
	}

	bool ConfigureSteamSystem(UNiagaraSystem* System, UMaterialInterface* SteamMaterial)
	{
		if (!System || !SteamMaterial)
		{
			return false;
		}

		bool bConfiguredSpriteRenderer = false;
		System->Modify();
		const int32 UpdatedStatelessModuleCount = ConfigureStatelessSteamModules(System);
		for (FNiagaraEmitterHandle& EmitterHandle : System->GetEmitterHandles())
		{
			EmitterHandle.SetDebugShowBounds(false);
			EmitterHandle.ForEachEnabledRendererWithIndex(
				[SteamMaterial, &bConfiguredSpriteRenderer](const UNiagaraRendererProperties* Renderer, int32)
				{
					UNiagaraSpriteRendererProperties* SpriteRenderer =
						Cast<UNiagaraSpriteRendererProperties>(const_cast<UNiagaraRendererProperties*>(Renderer));
					if (SpriteRenderer)
					{
						SpriteRenderer->Modify();
						SpriteRenderer->Material = SteamMaterial;
						SpriteRenderer->SubImageSize = FVector2D(1.0, 1.0);
						SpriteRenderer->bSubImageBlend = false;
						bConfiguredSpriteRenderer = true;
					}
				});
			if (FVersionedNiagaraEmitterData* EmitterData = EmitterHandle.GetEmitterData())
			{
				EmitterData->bLocalSpace = true;
				EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
				EmitterData->FixedBounds = FBox(FVector(-80.0f, -80.0f, -10.0f), FVector(80.0f, 80.0f, 220.0f));
			}
		}

		System->SetWarmupTime(0.35f);
		System->SetWarmupTickDelta(1.0f / 30.0f);
		System->InvalidateCachedData();
		System->RequestCompile(true);
		System->PollForCompilationComplete(true);
		System->PostEditChange();
		System->MarkPackageDirty();
		UE_LOG(
			LogTunaSweeperEditor,
			Display,
			TEXT("Configured boiling-pot steam: %d stateless modules, 38 cm world-space annular emission at 25%% component scale, textured white sprites."),
			UpdatedStatelessModuleCount);
		return bConfiguredSpriteRenderer && TunaSweeperEditorSetup::SaveAsset(System);
	}

	UNiagaraSystem* EnsureSteamSystem(UMaterialInterface* SteamMaterial)
	{
		const FString ObjectPath = TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, SteamSystemName);
		UNiagaraSystem* System = LoadObject<UNiagaraSystem>(nullptr, *ObjectPath);
		if (!System)
		{
			const TCHAR* SourcePath = TEXT("/Game/Effects/NS_ExplosiveBarrel_SmokeLight.NS_ExplosiveBarrel_SmokeLight");
			UNiagaraSystem* SourceSystem = LoadObject<UNiagaraSystem>(nullptr, SourcePath);
			if (!SourceSystem)
			{
				UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to load boiling-pot steam source %s."), SourcePath);
				return nullptr;
			}

			FAssetToolsModule& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
			System = Cast<UNiagaraSystem>(AssetTools.Get().DuplicateAsset(SteamSystemName, AssetPath, SourceSystem));
		}
		if (!System)
		{
			return nullptr;
		}

		return ConfigureSteamSystem(System, SteamMaterial) ? System : nullptr;
	}

	UBlueprint* EnsurePotBlueprint(UStaticMesh* BodyMesh, UStaticMesh* LidMesh, UNiagaraSystem* SteamSystem)
	{
		if (!BodyMesh || !LidMesh || !SteamSystem)
		{
			return nullptr;
		}

		UBlueprint* Blueprint = TunaSweeperEditorSetup::EnsureBlueprint(
			AssetPath,
			BlueprintName,
			ATunaSweeperBoilingPotActor::StaticClass());
		if (!Blueprint)
		{
			return nullptr;
		}

		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		ATunaSweeperBoilingPotActor* Defaults = Blueprint->GeneratedClass
			? Cast<ATunaSweeperBoilingPotActor>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			return nullptr;
		}

		Blueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigurePresentationDefaults(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, BodyMeshName))),
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, LidMeshName))),
			TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, SteamSystemName))));
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	bool Run()
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBoilingPotMeshesOnly")))
		{
			UMaterial* Material = LoadObject<UMaterial>(
				nullptr,
				*TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, MaterialName));
			UNiagaraSystem* ExistingSteamSystem = LoadObject<UNiagaraSystem>(
				nullptr,
				*TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, SteamSystemName));
			UStaticMesh* BodyMesh = EnsureStaticMesh(BodyMeshName, Material, BuildBodyMeshDescription);
			UStaticMesh* LidMesh = EnsureStaticMesh(LidMeshName, Material, BuildLidMeshDescription);
			return Material && ExistingSteamSystem && BodyMesh && LidMesh &&
				EnsurePotBlueprint(BodyMesh, LidMesh, ExistingSteamSystem);
		}

		UTexture2D* SteamOpacityTexture = EnsureSteamOpacityTexture();
		UMaterial* SteamMaterial = EnsureSteamWhiteMaterial(SteamOpacityTexture);
		UNiagaraSystem* SteamSystem = EnsureSteamSystem(SteamMaterial);
		if (FParse::Param(FCommandLine::Get(), TEXT("TunaSweeperBoilingPotSteamOnly")))
		{
			UStaticMesh* BodyMesh = LoadObject<UStaticMesh>(
				nullptr,
				*TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, BodyMeshName));
			UStaticMesh* LidMesh = LoadObject<UStaticMesh>(
				nullptr,
				*TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, LidMeshName));
			return SteamMaterial && SteamSystem && EnsurePotBlueprint(BodyMesh, LidMesh, SteamSystem);
		}

		UTexture2D* Texture = nullptr;
		const FString SourcePath = GetTextureSourcePath();
		if (!TunaSweeperEditorSetup::ImportWorldTexture(SourcePath, AssetPath, TextureName, &Texture) || !Texture)
		{
			UE_LOG(LogTunaSweeperEditor, Error, TEXT("Failed to import boiling-pot texture %s."), *SourcePath);
			return false;
		}

		Texture->Modify();
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->MarkPackageDirty();
		TunaSweeperEditorSetup::SaveAsset(Texture);

		UMaterial* Material = EnsurePotMaterial(Texture);
		UStaticMesh* BodyMesh = EnsureStaticMesh(BodyMeshName, Material, BuildBodyMeshDescription);
		UStaticMesh* LidMesh = EnsureStaticMesh(LidMeshName, Material, BuildLidMeshDescription);
		UBlueprint* Blueprint = EnsurePotBlueprint(BodyMesh, LidMesh, SteamSystem);
		const bool bSucceeded = Material && SteamOpacityTexture && SteamMaterial && BodyMesh && LidMesh && SteamSystem && Blueprint;

		if (bSucceeded)
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Display,
				TEXT("Boiling-pot setup completed: texture=%s material=%s body=%s lid=%s steam=%s blueprint=%s"),
				*GetNameSafe(Texture),
				*GetNameSafe(Material),
				*GetNameSafe(BodyMesh),
				*GetNameSafe(LidMesh),
				*GetNameSafe(SteamSystem),
				*GetNameSafe(Blueprint));
		}
		else
		{
			UE_LOG(
				LogTunaSweeperEditor,
				Error,
				TEXT("Boiling-pot setup failed: texture=%s material=%s body=%s lid=%s steam=%s blueprint=%s"),
				*GetNameSafe(Texture),
				*GetNameSafe(Material),
				*GetNameSafe(BodyMesh),
				*GetNameSafe(LidMesh),
				*GetNameSafe(SteamSystem),
				*GetNameSafe(Blueprint));
		}
		return bSucceeded;
	}
}
