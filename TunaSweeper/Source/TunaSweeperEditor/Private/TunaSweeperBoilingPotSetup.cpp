#include "TunaSweeperBoilingPotSetup.h"

#include "TunaSweeperEditorSetupShared.h"

#include "Environment/TunaSweeperBoilingPotActor.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "MeshDescriptionBuilder.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"

namespace TunaSweeperBoilingPotSetup
{
	const FString AssetPath = TEXT("/Game/Environment/Props/BoilingPot");
	const FString TextureName = TEXT("T_BoilingPot_Enamel");
	const FString MaterialName = TEXT("M_BoilingPot_Enamel");
	const FString BodyMeshName = TEXT("SM_BoilingPot_Body");
	const FString LidMeshName = TEXT("SM_BoilingPot_Lid");
	const FString SteamSystemName = TEXT("NS_BoilingPot_Steam");
	const FString BlueprintName = TEXT("BP_BoilingPot");
	const FName MaterialSlotName(TEXT("EnamelMetal"));
	constexpr int32 RadialSegments = 24;

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
			AddQuad(
				Builder,
				Group,
				MakeRadialPoint(BottomRadius, Angle0, BottomZ),
				MakeRadialPoint(BottomRadius, Angle1, BottomZ),
				MakeRadialPoint(TopRadius, Angle1, TopZ),
				MakeRadialPoint(TopRadius, Angle0, TopZ),
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
			AddQuad(
				Builder,
				Group,
				MakeRadialPoint(Radius, Angle0, BottomZ),
				MakeRadialPoint(Radius, Angle0, TopZ),
				MakeRadialPoint(Radius, Angle1, TopZ),
				MakeRadialPoint(Radius, Angle1, BottomZ),
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
		for (int32 Segment = 0; Segment < RadialSegments; ++Segment)
		{
			const int32 Next = (Segment + 1) % RadialSegments;
			const float Angle0 = UE_TWO_PI * static_cast<float>(Segment) / static_cast<float>(RadialSegments);
			const float Angle1 = UE_TWO_PI * static_cast<float>(Next) / static_cast<float>(RadialSegments);
			const FVector Point0 = MakeRadialPoint(Rings[0].Radius, Angle0, Rings[0].Z);
			const FVector Point1 = MakeRadialPoint(Rings[0].Radius, Angle1, Rings[0].Z);
			AddTriangle(
				Builder,
				Group,
				Center,
				Point0,
				Point1,
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
				AddQuad(Builder, Group, Inner0, Outer0, Outer1, Inner1, UV(Inner0), UV(Outer0), UV(Outer1), UV(Inner1));
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

	UNiagaraSystem* EnsureSteamSystem()
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

		System->Modify();
		System->SetWarmupTime(0.35f);
		System->SetWarmupTickDelta(1.0f / 30.0f);
		System->PostEditChange();
		System->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(System) ? System : nullptr;
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

		const FString ClatterSoundPath = TEXT("/Game/Audio/Imported/SW_BarrelHit.SW_BarrelHit");
		Blueprint->Modify();
		Defaults->Modify();
		Defaults->ConfigurePresentationDefaults(
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, BodyMeshName))),
			TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, LidMeshName))),
			TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(TunaSweeperEditorSetup::GetAssetObjectPath(AssetPath, SteamSystemName))),
			TSoftObjectPtr<USoundBase>(FSoftObjectPath(ClatterSoundPath)));
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		Blueprint->MarkPackageDirty();
		return TunaSweeperEditorSetup::SaveAsset(Blueprint) ? Blueprint : nullptr;
	}

	bool Run()
	{
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
		UNiagaraSystem* SteamSystem = EnsureSteamSystem();
		UBlueprint* Blueprint = EnsurePotBlueprint(BodyMesh, LidMesh, SteamSystem);
		const bool bSucceeded = Material && BodyMesh && LidMesh && SteamSystem && Blueprint;

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
