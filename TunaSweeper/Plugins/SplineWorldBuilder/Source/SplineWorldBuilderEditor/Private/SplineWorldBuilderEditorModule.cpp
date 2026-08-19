#include "SplineWorldBuilderActor.h"
#include "SplineWorldBuilderJunctionActor.h"
#include "SplineWorldBuilderProfile.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Components/SplineComponent.h"
#include "Editor.h"
#include "Engine/Selection.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Framework/Commands/UIAction.h"
#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "MeshDescription.h"
#include "MeshDescriptionBuilder.h"
#include "Misc/PackageName.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "ScopedTransaction.h"
#include "StaticMeshAttributes.h"
#include "ToolMenus.h"
#include "UObject/MetaData.h"
#include "UObject/SavePackage.h"

#define LOCTEXT_NAMESPACE "SplineWorldBuilderEditor"

DEFINE_LOG_CATEGORY_STATIC(LogSplineWorldBuilderEditor, Log, All);

namespace SplineWorldBuilderEditor
{
	const FString InternalAssetPath = TEXT("/SplineWorldBuilder/Generated/Internal");
	const FString ProfileAssetPath = TEXT("/SplineWorldBuilder/Profiles");
	const FString TextureName = TEXT("T_SWB_StoneBlocks");
	const FString MaterialName = TEXT("M_SWB_TestStone");
	const FString StraightMeshName = TEXT("SM_SWB_TestStraight");
	const FString EndMeshName = TEXT("SM_SWB_TestEnd");
	const FString CornerMeshName = TEXT("SM_SWB_TestCorner");
	const FString TJunctionMeshName = TEXT("SM_SWB_TestTJunction");
	const FString CrossJunctionMeshName = TEXT("SM_SWB_TestCrossJunction");
	const FString ProfileName = TEXT("DA_SWB_TestStoneWall");
	const TCHAR* AssetVersionKey = TEXT("SplineWorldBuilderAssetVersion");
	const TCHAR* AssetVersion = TEXT("1");

	enum class ETestArm : uint8
	{
		PositiveX = 1 << 0,
		NegativeX = 1 << 1,
		PositiveY = 1 << 2,
		NegativeY = 1 << 3
	};

	struct FGeneratedAssets
	{
		TObjectPtr<UTexture2D> Texture;
		TObjectPtr<UMaterial> Material;
		TObjectPtr<UStaticMesh> StraightMesh;
		TObjectPtr<UStaticMesh> EndMesh;
		TObjectPtr<UStaticMesh> CornerMesh;
		TObjectPtr<UStaticMesh> TJunctionMesh;
		TObjectPtr<UStaticMesh> CrossJunctionMesh;
		TObjectPtr<USplineWorldBuilderProfile> Profile;

		bool IsComplete() const
		{
			return Texture && Material && StraightMesh && EndMesh && CornerMesh &&
				TJunctionMesh && CrossJunctionMesh && Profile;
		}
	};

	FString ObjectPath(const FString& AssetPath, const FString& AssetName)
	{
		return FString::Printf(TEXT("%s/%s.%s"), *AssetPath, *AssetName, *AssetName);
	}

	bool SaveAsset(UObject* Asset)
	{
		if (!Asset || !Asset->GetOutermost())
		{
			return false;
		}

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset->GetOutermost()->GetName(),
			FPackageName::GetAssetPackageExtension());
		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		SaveArgs.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Asset->GetOutermost(), Asset, *Filename, SaveArgs);
	}

	bool HasCurrentAssetVersion(const UObject* Asset)
	{
		if (!Asset || !Asset->GetOutermost())
		{
			return false;
		}
		return Asset->GetOutermost()->GetMetaData().GetValue(Asset, AssetVersionKey) == AssetVersion;
	}

	void StampAssetVersion(UObject* Asset)
	{
		if (Asset && Asset->GetOutermost())
		{
			Asset->GetOutermost()->GetMetaData().SetValue(Asset, AssetVersionKey, AssetVersion);
		}
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

	UTexture2D* EnsureTexture(const bool bForceRebuild)
	{
		const FString TextureObjectPath = ObjectPath(InternalAssetPath, TextureName);
		UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TextureObjectPath);
		if (Texture && !bForceRebuild && HasCurrentAssetVersion(Texture))
		{
			return Texture;
		}

		const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SplineWorldBuilder"));
		const FString SourceFilename = Plugin.IsValid()
			? FPaths::Combine(Plugin->GetBaseDir(), TEXT("Resources/SourceArt/T_SplineWorldBuilder_StoneBlocks_ImageGen.png"))
			: FString();
		FImage SourceImage;
		if (SourceFilename.IsEmpty() || !FImageUtils::LoadImage(*SourceFilename, SourceImage))
		{
			UE_LOG(LogSplineWorldBuilderEditor, Error, TEXT("Failed to load ImageGen source texture: %s"), *SourceFilename);
			return nullptr;
		}

		if (!Texture)
		{
			UPackage* Package = CreatePackage(*(InternalAssetPath / TextureName));
			Texture = Package
				? Cast<UTexture2D>(FImageUtils::CreateTexture(
					ETextureClass::TwoD,
					SourceImage,
					Package,
					TextureName,
					RF_Public | RF_Standalone | RF_Transactional,
					false))
				: nullptr;
			if (Texture)
			{
				FAssetRegistryModule::AssetCreated(Texture);
			}
		}
		else
		{
			Texture->Modify();
			Texture->Source.Init(SourceImage);
		}

		if (!Texture)
		{
			return nullptr;
		}

		Texture->SRGB = true;
		Texture->CompressionSettings = TC_Default;
		Texture->MipGenSettings = TMGS_FromTextureGroup;
		Texture->PowerOfTwoMode = ETexturePowerOfTwoSetting::PadToPowerOfTwo;
		Texture->AddressX = TA_Wrap;
		Texture->AddressY = TA_Wrap;
		Texture->LODGroup = TEXTUREGROUP_World;
		StampAssetVersion(Texture);
		Texture->PostEditChange();
		Texture->MarkPackageDirty();
		return SaveAsset(Texture) ? Texture : nullptr;
	}

	UMaterial* EnsureMaterial(UTexture2D* Texture, const bool bForceRebuild)
	{
		if (!Texture)
		{
			return nullptr;
		}

		const FString MaterialObjectPath = ObjectPath(InternalAssetPath, MaterialName);
		UMaterial* Material = LoadObject<UMaterial>(nullptr, *MaterialObjectPath);
		if (Material && !bForceRebuild && HasCurrentAssetVersion(Material))
		{
			return Material;
		}
		if (!Material)
		{
			UPackage* Package = CreatePackage(*(InternalAssetPath / MaterialName));
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
		Material->bUsedWithSplineMeshes = true;
		Material->SetShadingModel(MSM_DefaultLit);

		UMaterialEditorOnlyData* EditorData = Material->GetEditorOnlyData();
		if (!EditorData)
		{
			return nullptr;
		}
		UMaterialExpressionTextureSampleParameter2D* Sample =
			AddExpression<UMaterialExpressionTextureSampleParameter2D>(Material, -320, -40);
		Sample->ParameterName = TEXT("StoneTexture");
		Sample->Texture = Texture;
		Sample->SamplerType = SAMPLERTYPE_Color;
		UMaterialExpressionScalarParameter* Roughness =
			AddExpression<UMaterialExpressionScalarParameter>(Material, -320, 100);
		Roughness->ParameterName = TEXT("Roughness");
		Roughness->DefaultValue = 0.82f;

		EditorData->BaseColor.Connect(0, Sample);
		EditorData->Roughness.Connect(0, Roughness);
		EditorData->Metallic.UseConstant = true;
		EditorData->Metallic.Constant = 0.0f;
		EditorData->Specular.UseConstant = true;
		EditorData->Specular.Constant = 0.22f;

		StampAssetVersion(Material);
		Material->PostEditChange();
		Material->MarkPackageDirty();
		return SaveAsset(Material) ? Material : nullptr;
	}

	void AddQuad(
		FMeshDescriptionBuilder& Builder,
		const FPolygonGroupID Group,
		const FVector& P0,
		const FVector& P1,
		const FVector& P2,
		const FVector& P3,
		const FVector& Normal,
		const FVector& Tangent,
		const FVector2D& UVExtent)
	{
		const FVector Positions[4] = { P0, P1, P2, P3 };
		const FVector2D UVs[4] = {
			FVector2D(0.0, 0.0),
			FVector2D(UVExtent.X, 0.0),
			FVector2D(UVExtent.X, UVExtent.Y),
			FVector2D(0.0, UVExtent.Y)
		};
		FVertexInstanceID Instances[4];
		for (int32 Index = 0; Index < 4; ++Index)
		{
			const FVertexID Vertex = Builder.AppendVertex(Positions[Index]);
			Instances[Index] = Builder.AppendInstance(Vertex);
			Builder.SetInstanceTangentSpace(Instances[Index], Normal, Tangent, 1.0f);
			Builder.SetInstanceUV(Instances[Index], UVs[Index], 0);
			Builder.SetInstanceColor(Instances[Index], FVector4f(1.0f, 1.0f, 1.0f, 1.0f));
		}
		Builder.AppendTriangle(Instances[0], Instances[1], Instances[2], Group);
		Builder.AppendTriangle(Instances[0], Instances[2], Instances[3], Group);
	}

	void AddBox(FMeshDescriptionBuilder& Builder, const FPolygonGroupID Group, const FVector& Min, const FVector& Max)
	{
		const double XYTextureScale = 200.0;
		const double X = (Max.X - Min.X) / XYTextureScale;
		const double Y = (Max.Y - Min.Y) / XYTextureScale;
		const double Z = (Max.Z - Min.Z) / XYTextureScale;

		AddQuad(Builder, Group,
			FVector(Max.X, Min.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z),
			FVector(Max.X, Max.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z),
			FVector::ForwardVector, FVector::RightVector, FVector2D(Y, Z));
		AddQuad(Builder, Group,
			FVector(Min.X, Max.Y, Min.Z), FVector(Min.X, Min.Y, Min.Z),
			FVector(Min.X, Min.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z),
			-FVector::ForwardVector, -FVector::RightVector, FVector2D(Y, Z));
		AddQuad(Builder, Group,
			FVector(Max.X, Max.Y, Min.Z), FVector(Min.X, Max.Y, Min.Z),
			FVector(Min.X, Max.Y, Max.Z), FVector(Max.X, Max.Y, Max.Z),
			FVector::RightVector, -FVector::ForwardVector, FVector2D(X, Z));
		AddQuad(Builder, Group,
			FVector(Min.X, Min.Y, Min.Z), FVector(Max.X, Min.Y, Min.Z),
			FVector(Max.X, Min.Y, Max.Z), FVector(Min.X, Min.Y, Max.Z),
			-FVector::RightVector, FVector::ForwardVector, FVector2D(X, Z));
		AddQuad(Builder, Group,
			FVector(Min.X, Min.Y, Max.Z), FVector(Max.X, Min.Y, Max.Z),
			FVector(Max.X, Max.Y, Max.Z), FVector(Min.X, Max.Y, Max.Z),
			FVector::UpVector, FVector::ForwardVector, FVector2D(X, Y));
		AddQuad(Builder, Group,
			FVector(Min.X, Max.Y, Min.Z), FVector(Max.X, Max.Y, Min.Z),
			FVector(Max.X, Min.Y, Min.Z), FVector(Min.X, Min.Y, Min.Z),
			-FVector::UpVector, FVector::ForwardVector, FVector2D(X, Y));
	}

	UStaticMesh* EnsureTestMesh(
		const FString& MeshName,
		const uint8 Arms,
		UMaterialInterface* Material,
		const bool bForceRebuild)
	{
		const FString MeshObjectPath = ObjectPath(InternalAssetPath, MeshName);
		UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshObjectPath);
		if (Mesh && !bForceRebuild && HasCurrentAssetVersion(Mesh))
		{
			return Mesh;
		}
		if (!Mesh)
		{
			UPackage* Package = CreatePackage(*(InternalAssetPath / MeshName));
			Mesh = Package
				? NewObject<UStaticMesh>(Package, *MeshName, RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (Mesh)
			{
				FAssetRegistryModule::AssetCreated(Mesh);
			}
		}
		if (!Mesh || !Material)
		{
			return nullptr;
		}

		FMeshDescription MeshDescription;
		FStaticMeshAttributes Attributes(MeshDescription);
		Attributes.Register();
		FMeshDescriptionBuilder Builder;
		Builder.SetMeshDescription(&MeshDescription);
		Builder.SetNumUVLayers(1);
		const FPolygonGroupID Group = Builder.AppendPolygonGroup(TEXT("Stone"));

		const FVector CenterMin(-25.0, -25.0, 0.0);
		const FVector CenterMax(25.0, 25.0, 140.0);
		AddBox(Builder, Group, CenterMin, CenterMax);
		if ((Arms & static_cast<uint8>(ETestArm::PositiveX)) != 0)
		{
			AddBox(Builder, Group, FVector(25.0, -25.0, 0.0), FVector(100.0, 25.0, 140.0));
		}
		if ((Arms & static_cast<uint8>(ETestArm::NegativeX)) != 0)
		{
			AddBox(Builder, Group, FVector(-100.0, -25.0, 0.0), FVector(-25.0, 25.0, 140.0));
		}
		if ((Arms & static_cast<uint8>(ETestArm::PositiveY)) != 0)
		{
			AddBox(Builder, Group, FVector(-25.0, 25.0, 0.0), FVector(25.0, 100.0, 140.0));
		}
		if ((Arms & static_cast<uint8>(ETestArm::NegativeY)) != 0)
		{
			AddBox(Builder, Group, FVector(-25.0, -100.0, 0.0), FVector(25.0, -25.0, 140.0));
		}

		Mesh->Modify();
		Mesh->GetStaticMaterials().Reset();
		Mesh->GetStaticMaterials().Add(FStaticMaterial(Material, TEXT("Stone")));
		UStaticMesh::FBuildMeshDescriptionsParams BuildParams;
		BuildParams.bBuildSimpleCollision = true;
		BuildParams.bCommitMeshDescription = true;
		BuildParams.bMarkPackageDirty = true;
		BuildParams.bAllowCpuAccess = true;
		const TArray<const FMeshDescription*> MeshDescriptions = { &MeshDescription };
		if (!Mesh->BuildFromMeshDescriptions(MeshDescriptions, BuildParams))
		{
			UE_LOG(LogSplineWorldBuilderEditor, Error, TEXT("Failed to build test mesh %s"), *MeshName);
			return nullptr;
		}

		StampAssetVersion(Mesh);
		Mesh->PostEditChange();
		Mesh->MarkPackageDirty();
		return SaveAsset(Mesh) ? Mesh : nullptr;
	}

	USplineWorldBuilderProfile* EnsureProfile(const FGeneratedAssets& Assets, const bool bForceRebuild)
	{
		const FString ProfileObjectPath = ObjectPath(ProfileAssetPath, ProfileName);
		USplineWorldBuilderProfile* Profile = LoadObject<USplineWorldBuilderProfile>(nullptr, *ProfileObjectPath);
		if (Profile && !bForceRebuild && HasCurrentAssetVersion(Profile))
		{
			return Profile;
		}
		if (!Profile)
		{
			UPackage* Package = CreatePackage(*(ProfileAssetPath / ProfileName));
			Profile = Package
				? NewObject<USplineWorldBuilderProfile>(Package, *ProfileName, RF_Public | RF_Standalone | RF_Transactional)
				: nullptr;
			if (Profile)
			{
				FAssetRegistryModule::AssetCreated(Profile);
			}
		}
		if (!Profile)
		{
			return nullptr;
		}

		Profile->Modify();
		Profile->BuilderKind = ESplineWorldBuilderKind::Wall;
		Profile->StraightMesh = Assets.StraightMesh;
		Profile->EndMesh = Assets.EndMesh;
		Profile->CornerMesh = Assets.CornerMesh;
		Profile->TJunctionMesh = Assets.TJunctionMesh;
		Profile->CrossJunctionMesh = Assets.CrossJunctionMesh;
		Profile->MaterialOverride = Assets.Material;
		Profile->ModuleLength = 200.0;
		Profile->JunctionTrimDistance = 100.0;
		Profile->EndTrimDistance = 100.0;
		Profile->CornerTrimDistance = 100.0;
		Profile->CornerAngleThreshold = 20.0;
		Profile->MinimumLengthScale = 0.80;
		Profile->MaximumLengthScale = 1.20;
		StampAssetVersion(Profile);
		Profile->PostEditChange();
		Profile->MarkPackageDirty();
		return SaveAsset(Profile) ? Profile : nullptr;
	}

	FGeneratedAssets EnsurePluginAssets(const bool bForceRebuild)
	{
		FGeneratedAssets Assets;
		Assets.Texture = EnsureTexture(bForceRebuild);
		Assets.Material = EnsureMaterial(Assets.Texture, bForceRebuild);
		const uint8 PositiveX = static_cast<uint8>(ETestArm::PositiveX);
		const uint8 NegativeX = static_cast<uint8>(ETestArm::NegativeX);
		const uint8 PositiveY = static_cast<uint8>(ETestArm::PositiveY);
		const uint8 NegativeY = static_cast<uint8>(ETestArm::NegativeY);
		Assets.StraightMesh = EnsureTestMesh(StraightMeshName, PositiveX | NegativeX, Assets.Material, bForceRebuild);
		Assets.EndMesh = EnsureTestMesh(EndMeshName, PositiveX, Assets.Material, bForceRebuild);
		Assets.CornerMesh = EnsureTestMesh(CornerMeshName, PositiveX | PositiveY, Assets.Material, bForceRebuild);
		Assets.TJunctionMesh = EnsureTestMesh(TJunctionMeshName, PositiveX | NegativeX | PositiveY, Assets.Material, bForceRebuild);
		Assets.CrossJunctionMesh = EnsureTestMesh(
			CrossJunctionMeshName,
			PositiveX | NegativeX | PositiveY | NegativeY,
			Assets.Material,
			bForceRebuild);
		Assets.Profile = EnsureProfile(Assets, bForceRebuild);

		if (Assets.IsComplete())
		{
			UE_LOG(LogSplineWorldBuilderEditor, Display, TEXT("SplineWorldBuilder generated test assets are ready."));
		}
		else
		{
			UE_LOG(LogSplineWorldBuilderEditor, Error, TEXT("SplineWorldBuilder generated test assets are incomplete."));
		}
		return Assets;
	}

	FVector ChooseSpawnLocation()
	{
		if (GEditor)
		{
			if (USelection* Selection = GEditor->GetSelectedActors())
			{
				if (AActor* Actor = Selection->GetTop<AActor>())
				{
					return Actor->GetActorLocation() + FVector(0.0, 0.0, 20.0);
				}
			}
		}
		return FVector::ZeroVector;
	}

	ASplineWorldBuilderActor* SpawnChain(
		UWorld* World,
		ASplineWorldJunctionActor* Junction,
		USplineWorldBuilderProfile* Profile,
		const FVector& Direction,
		const FString& Label)
	{
		if (!World || !Junction || !Profile)
		{
			return nullptr;
		}

		ASplineWorldBuilderActor* Chain = World->SpawnActor<ASplineWorldBuilderActor>(
			ASplineWorldBuilderActor::StaticClass(),
			Junction->GetActorTransform());
		if (!Chain)
		{
			return nullptr;
		}
		Chain->SetFlags(RF_Transactional);
		Chain->SetActorLabel(Label);
		Chain->SetFolderPath(TEXT("SplineWorldBuilder/TestJunctions"));
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

		FSplineWorldJunctionConnection& Connection = Junction->Connections.AddDefaulted_GetRef();
		Connection.Chain = Chain;
		Connection.Endpoint = ESplineWorldEndpoint::Start;
		return Chain;
	}

	ASplineWorldJunctionActor* SpawnJunctionExample(
		UWorld* World,
		USplineWorldBuilderProfile* Profile,
		const FVector& Location,
		const FString& Label,
		const TArray<FVector>& Directions)
	{
		ASplineWorldJunctionActor* Junction = World->SpawnActor<ASplineWorldJunctionActor>(
			ASplineWorldJunctionActor::StaticClass(),
			FTransform(FRotator::ZeroRotator, Location));
		if (!Junction)
		{
			return nullptr;
		}
		Junction->SetFlags(RF_Transactional);
		Junction->SetActorLabel(Label);
		Junction->SetFolderPath(TEXT("SplineWorldBuilder/TestJunctions"));
		Junction->Profile = Profile;
		Junction->bAutoRebuild = false;

		for (int32 Index = 0; Index < Directions.Num(); ++Index)
		{
			SpawnChain(World, Junction, Profile, Directions[Index], FString::Printf(TEXT("%s_Arm_%d"), *Label, Index + 1));
		}
		Junction->RebuildGenerated();
		return Junction;
	}
}

class FSplineWorldBuilderEditorModule final : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		EditorInitializedHandle = FEditorDelegates::OnEditorInitialized.AddRaw(
			this,
			&FSplineWorldBuilderEditorModule::OnEditorInitialized);
		UToolMenus::RegisterStartupCallback(
			FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FSplineWorldBuilderEditorModule::RegisterMenus));
	}

	virtual void ShutdownModule() override
	{
		if (EditorInitializedHandle.IsValid())
		{
			FEditorDelegates::OnEditorInitialized.Remove(EditorInitializedHandle);
			EditorInitializedHandle.Reset();
		}
		if (UToolMenus::IsToolMenuUIEnabled())
		{
			UToolMenus::UnRegisterStartupCallback(this);
			UToolMenus::UnregisterOwner(this);
		}
	}

private:
	void OnEditorInitialized(double)
	{
		const bool bForceRebuild = FParse::Param(FCommandLine::Get(), TEXT("SplineWorldBuilderRebuildAssets"));
		SplineWorldBuilderEditor::EnsurePluginAssets(bForceRebuild);
		if (FParse::Param(FCommandLine::Get(), TEXT("SplineWorldBuilderAssetGenerationQuit")))
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}

	void RegisterMenus()
	{
		FToolMenuOwnerScoped OwnerScoped(this);
		UToolMenu* MainMenu = UToolMenus::Get()->ExtendMenu(TEXT("LevelEditor.MainMenu"));
		FToolMenuSection& MainSection = MainMenu->FindOrAddSection(NAME_None);
		if (!MainSection.FindEntry(TEXT("TunaSweeper")))
		{
			FToolMenuEntry& TunaSweeperEntry = MainSection.AddSubMenu(
				TEXT("TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenu", "TunaSweeper"),
				LOCTEXT("TunaSweeperTopMenuTooltip", "Open TunaSweeper editor tools."),
				FNewToolMenuChoice());
			TunaSweeperEntry.InsertPosition = FToolMenuInsert(TEXT("Tools"), EToolMenuInsertType::After);
		}

		UToolMenu* TunaSweeperMenu = UToolMenus::Get()->RegisterMenu(
			TEXT("LevelEditor.MainMenu.TunaSweeper"), NAME_None, EMultiBoxType::Menu, false);
		FToolMenuSection& Section = TunaSweeperMenu->FindOrAddSection(
			TEXT("WorldBuilding"), LOCTEXT("WorldBuildingSection", "World Building"));
		Section.AddMenuEntry(
			TEXT("AddSplineWorldBuilderJunctionTests"),
			LOCTEXT("AddJunctionTests", "Spline World Builder: Add Junction Test Set"),
			LOCTEXT("AddJunctionTestsTooltip", "Add End, Straight, Corner, T, and Cross test networks using generated stone assets."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(this, &FSplineWorldBuilderEditorModule::AddJunctionTestSet)));
		Section.AddMenuEntry(
			TEXT("RebuildSplineWorldBuilderAssets"),
			LOCTEXT("RebuildAssets", "Spline World Builder: Rebuild Test Assets"),
			LOCTEXT("RebuildAssetsTooltip", "Rebuild the ImageGen texture, material, test junction meshes, and public test profile."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				SplineWorldBuilderEditor::EnsurePluginAssets(true);
			})));
	}

	void AddJunctionTestSet()
	{
		if (!GEditor)
		{
			return;
		}
		SplineWorldBuilderEditor::FGeneratedAssets Assets = SplineWorldBuilderEditor::EnsurePluginAssets(false);
		UWorld* World = GEditor->GetEditorWorldContext().World();
		if (!Assets.IsComplete() || !World)
		{
			return;
		}

		const FScopedTransaction Transaction(LOCTEXT("AddJunctionTestSetTransaction", "Add Spline Junction Test Set"));
		World->Modify();
		const FVector Origin = SplineWorldBuilderEditor::ChooseSpawnLocation();
		TArray<ASplineWorldJunctionActor*> Junctions;
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Assets.Profile, Origin, TEXT("SWB_End"), { FVector::ForwardVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Assets.Profile, Origin + FVector(900.0, 0.0, 0.0), TEXT("SWB_Straight"),
			{ FVector::ForwardVector, -FVector::ForwardVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Assets.Profile, Origin + FVector(0.0, 900.0, 0.0), TEXT("SWB_Corner"),
			{ FVector::ForwardVector, FVector::RightVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Assets.Profile, Origin + FVector(900.0, 900.0, 0.0), TEXT("SWB_T"),
			{ FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector }));
		Junctions.Add(SplineWorldBuilderEditor::SpawnJunctionExample(
			World, Assets.Profile, Origin + FVector(1800.0, 900.0, 0.0), TEXT("SWB_Cross"),
			{ FVector::ForwardVector, -FVector::ForwardVector, FVector::RightVector, -FVector::RightVector }));

		GEditor->SelectNone(false, true, false);
		for (ASplineWorldJunctionActor* Junction : Junctions)
		{
			if (Junction)
			{
				GEditor->SelectActor(Junction, true, false, true);
			}
		}
		GEditor->NoteSelectionChange();
	}

	FDelegateHandle EditorInitializedHandle;
};

IMPLEMENT_MODULE(FSplineWorldBuilderEditorModule, SplineWorldBuilderEditor)

#undef LOCTEXT_NAMESPACE
