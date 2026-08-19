#include "StylizedWaterBodyActor.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "ProceduralMeshComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogStylizedWater, Log, All);

namespace StylizedWater
{
	const TCHAR* GeneratedMaterialInstancePath = TEXT("/StylizedWater/Generated/Internal/MI_StylizedWater_CalmAnime.MI_StylizedWater_CalmAnime");

	FName ShallowColorName(TEXT("ShallowColor"));
	FName MidColorName(TEXT("MidColor"));
	FName DeepColorName(TEXT("DeepColor"));
	FName FoamColorName(TEXT("FoamColor"));
	FName DepthColorRangeName(TEXT("DepthColorRangeCm"));
	FName MidColorPositionName(TEXT("MidColorPosition"));
	FName DepthGradientInfluenceName(TEXT("DepthGradientInfluence"));
	FName OpacityName(TEXT("Opacity"));
	FName RoughnessName(TEXT("Roughness"));
	FName DistortionStrengthName(TEXT("DistortionStrength"));
	FName EmissiveStrengthName(TEXT("EmissiveStrength"));
	FName WaterLevelOffsetName(TEXT("WaterLevelOffsetCm"));
	FName WaterlineSoftnessName(TEXT("WaterlineSoftnessCm"));
	FName ShoreRunupName(TEXT("ShoreRunupCm"));
	FName ShoreWavelengthName(TEXT("ShoreWavelengthCm"));
	FName ShoreWaveSpeedName(TEXT("ShoreWaveSpeed"));
	FName ShoreFoamDepthName(TEXT("ShoreFoamDepthCm"));
	FName ShoreFoamWidthName(TEXT("ShoreFoamWidth"));
	FName FoamIntensityName(TEXT("FoamIntensity"));
	FName FlowDirectionName(TEXT("FlowDirection"));
	FName FlowSpeedName(TEXT("FlowSpeed"));
	FName WaveWorldScaleName(TEXT("WaveWorldScale"));
	FName GeometryWaveAmplitudeName(TEXT("GeometryWaveAmplitudeCm"));
	FName DryRangeName(TEXT("DryRangeCm"));
	FName DepthRangeName(TEXT("DepthRangeCm"));
}

AStylizedWaterBodyActor::AStylizedWaterBodyActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	WaterSurface = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WaterSurface"));
	WaterSurface->SetupAttachment(SceneRoot);
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WaterSurface->SetGenerateOverlapEvents(false);
	WaterSurface->bUseAsyncCooking = true;
	WaterSurface->SetCastShadow(false);
	WaterSurface->SetCanEverAffectNavigation(false);

	ShoreOverlay = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("ShoreOverlay"));
	ShoreOverlay->SetupAttachment(SceneRoot);
	ShoreOverlay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreOverlay->SetGenerateOverlapEvents(false);
	ShoreOverlay->bUseAsyncCooking = true;
	ShoreOverlay->SetCastShadow(false);
	ShoreOverlay->SetCanEverAffectNavigation(false);

	TemplateMaterialInstance = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(StylizedWater::GeneratedMaterialInstancePath));
}

void AStylizedWaterBodyActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (HasAnyFlags(RF_ClassDefaultObject) || !WaterSurface || !ShoreOverlay)
	{
		return;
	}

	if (WaterSurface->GetNumSections() == 0 || !bShoreOverlayBakeInitialized)
	{
		BuildWaterMesh(bSampleTerrainOnRebuild);
	}
	else
	{
		EnsureDynamicMaterial();
		UpdateMaterialParameters();
	}
}

void AStylizedWaterBodyActor::BeginPlay()
{
	Super::BeginPlay();
	EnsureDynamicMaterial();
	UpdateMaterialParameters();
}

void AStylizedWaterBodyActor::RebuildAndBakeDepth()
{
	BuildWaterMesh(true);
}

void AStylizedWaterBodyActor::RebuildWithoutTerrainTrace()
{
	BuildWaterMesh(false);
}

void AStylizedWaterBodyActor::ApplyCalmLakePreset()
{
	ApplyPreset(EStylizedWaterPreset::CalmLake, true);
}

void AStylizedWaterBodyActor::ApplyGentleBeachPreset()
{
	ApplyPreset(EStylizedWaterPreset::GentleBeach, true);
}

void AStylizedWaterBodyActor::ApplyFlowingRiverPreset()
{
	ApplyPreset(EStylizedWaterPreset::FlowingRiver, true);
}

void AStylizedWaterBodyActor::ApplyPreset(const EStylizedWaterPreset InPreset, const bool bRebuild)
{
	DepthGradientInfluence = 1.0f;
	switch (InPreset)
	{
	case EStylizedWaterPreset::CalmLake:
		SurfaceSize = FVector2D(5000.0, 5000.0);
		GridResolution = FIntPoint(48, 48);
		ShallowColor = FLinearColor(0.17f, 0.72f, 0.77f, 1.0f);
		MidColor = FLinearColor(0.035f, 0.43f, 0.63f, 1.0f);
		DeepColor = FLinearColor(0.014f, 0.16f, 0.34f, 1.0f);
		Opacity = 0.84f;
		Roughness = 0.13f;
		DistortionStrength = 0.12f;
		ShoreRunup = 12.0f;
		ShoreWavelength = 360.0f;
		ShoreWaveSpeed = 0.09f;
		ShoreFoamDepth = 90.0f;
		ShoreFoamWidth = 0.09f;
		FoamIntensity = 0.42f;
		FlowSpeed = 0.06f;
		GeometryWaveAmplitude = 1.5f;
		break;

	case EStylizedWaterPreset::GentleBeach:
		SurfaceSize = FVector2D(6000.0, 4200.0);
		GridResolution = FIntPoint(72, 52);
		ShallowColor = FLinearColor(0.24f, 0.78f, 0.76f, 1.0f);
		MidColor = FLinearColor(0.045f, 0.49f, 0.68f, 1.0f);
		DeepColor = FLinearColor(0.012f, 0.18f, 0.38f, 1.0f);
		Opacity = 0.80f;
		Roughness = 0.16f;
		DistortionStrength = 0.13f;
		ShoreRunup = 38.0f;
		ShoreWavelength = 260.0f;
		ShoreWaveSpeed = 0.16f;
		ShoreFoamDepth = 150.0f;
		ShoreFoamWidth = 0.14f;
		FoamIntensity = 0.95f;
		FlowSpeed = 0.08f;
		GeometryWaveAmplitude = 4.0f;
		break;

	case EStylizedWaterPreset::FlowingRiver:
		SurfaceSize = FVector2D(7000.0, 1800.0);
		GridResolution = FIntPoint(96, 28);
		ShallowColor = FLinearColor(0.13f, 0.67f, 0.70f, 1.0f);
		MidColor = FLinearColor(0.025f, 0.38f, 0.52f, 1.0f);
		DeepColor = FLinearColor(0.012f, 0.16f, 0.28f, 1.0f);
		Opacity = 0.86f;
		Roughness = 0.11f;
		DistortionStrength = 0.18f;
		ShoreRunup = 10.0f;
		ShoreWavelength = 220.0f;
		ShoreWaveSpeed = 0.12f;
		ShoreFoamDepth = 75.0f;
		ShoreFoamWidth = 0.08f;
		FoamIntensity = 0.46f;
		FlowSpeed = 0.32f;
		WaveWorldScale = 0.0048f;
		GeometryWaveAmplitude = 2.0f;
		FlowDirection = FVector2D(1.0, 0.0);
		break;
	}

	if (bRebuild)
	{
		BuildWaterMesh(bSampleTerrainOnRebuild);
	}
	else
	{
		UpdateMaterialParameters();
	}
}

void AStylizedWaterBodyActor::SetTemplateMaterialInstance(UMaterialInterface* InMaterial)
{
	TemplateMaterialInstance = InMaterial;
	DynamicMaterial = nullptr;
	EnsureDynamicMaterial();
	UpdateMaterialParameters();
}

float AStylizedWaterBodyActor::SampleSignedDepthAtWorldPosition(const FVector& SurfaceWorldPosition, FHitResult& OutHit) const
{
	OutHit = FHitResult();
	const UWorld* World = GetWorld();
	if (!World)
	{
		return MaximumDepth;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(StylizedWaterDepthBake), true, this);
	QueryParams.AddIgnoredActor(this);

	FVector TraceStart = SurfaceWorldPosition;
	TraceStart.Z += FMath::Max(TraceHeight, MaximumDryHeight + 10.0f);
	FVector TraceEnd = SurfaceWorldPosition;
	TraceEnd.Z -= MaximumDepth;

	if (!World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, TerrainTraceChannel, QueryParams))
	{
		return MaximumDepth;
	}

	return FMath::Clamp(SurfaceWorldPosition.Z - OutHit.ImpactPoint.Z, -MaximumDryHeight, MaximumDepth);
}

void AStylizedWaterBodyActor::BuildWaterMesh(const bool bTraceTerrain)
{
	if (!WaterSurface || !ShoreOverlay || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	const int32 ResolutionX = FMath::Clamp(GridResolution.X, 2, 256);
	const int32 ResolutionY = FMath::Clamp(GridResolution.Y, 2, 256);
	GridResolution = FIntPoint(ResolutionX, ResolutionY);
	SurfaceSize.X = FMath::Max(SurfaceSize.X, 100.0);
	SurfaceSize.Y = FMath::Max(SurfaceSize.Y, 100.0);
	MaximumDryHeight = FMath::Max(MaximumDryHeight, 10.0f);
	MaximumDepth = FMath::Max(MaximumDepth, 10.0f);

	const int32 VertexCount = (ResolutionX + 1) * (ResolutionY + 1);
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> ShoreVertices;
	TArray<int32> ShoreTriangles;
	TArray<FVector> Normals;
	TArray<FVector> ShoreNormals;
	TArray<FVector2D> UVs;
	TArray<FLinearColor> VertexColors;
	TArray<FProcMeshTangent> Tangents;
	TArray<FProcMeshTangent> ShoreTangents;
	TArray<bool> TerrainHitMask;
	TArray<float> SignedDepths;
	Vertices.Reserve(VertexCount);
	ShoreVertices.Reserve(VertexCount);
	Normals.Reserve(VertexCount);
	ShoreNormals.Reserve(VertexCount);
	UVs.Reserve(VertexCount);
	VertexColors.Reserve(VertexCount);
	Tangents.Reserve(VertexCount);
	ShoreTangents.Reserve(VertexCount);
	TerrainHitMask.Reserve(VertexCount);
	SignedDepths.Reserve(VertexCount);
	Triangles.Reserve(ResolutionX * ResolutionY * 6);
	ShoreTriangles.Reserve(ResolutionX * ResolutionY * 6);

	const FTransform ActorTransform = GetActorTransform();
	const float TotalDepthRange = MaximumDryHeight + MaximumDepth;
	int32 HitSampleCount = 0;
	float MinimumHitDepth = MaximumDepth;
	float MaximumHitDepth = -MaximumDryHeight;
	for (int32 Y = 0; Y <= ResolutionY; ++Y)
	{
		const float V = static_cast<float>(Y) / static_cast<float>(ResolutionY);
		for (int32 X = 0; X <= ResolutionX; ++X)
		{
			const float U = static_cast<float>(X) / static_cast<float>(ResolutionX);
			const FVector LocalPosition(
				FMath::Lerp(-0.5 * SurfaceSize.X, 0.5 * SurfaceSize.X, U),
				FMath::Lerp(-0.5 * SurfaceSize.Y, 0.5 * SurfaceSize.Y, V),
				0.0);
			const FVector WorldPosition = ActorTransform.TransformPosition(LocalPosition);
			FHitResult TerrainHit;
			const float SignedDepth = bTraceTerrain ? SampleSignedDepthAtWorldPosition(WorldPosition, TerrainHit) : MaximumDepth;
			const bool bHitTerrain = TerrainHit.bBlockingHit;
			if (bHitTerrain)
			{
				++HitSampleCount;
				MinimumHitDepth = FMath::Min(MinimumHitDepth, SignedDepth);
				MaximumHitDepth = FMath::Max(MaximumHitDepth, SignedDepth);
			}
			const float EncodedDepth = FMath::Clamp((SignedDepth + MaximumDryHeight) / TotalDepthRange, 0.0f, 1.0f);
			const float BorderDistance = FMath::Clamp(2.0f * FMath::Min(FMath::Min(U, 1.0f - U), FMath::Min(V, 1.0f - V)), 0.0f, 1.0f);

			Vertices.Add(LocalPosition);
			if (bHitTerrain)
			{
				const FVector SafeImpactNormal = TerrainHit.ImpactNormal.GetSafeNormal(SMALL_NUMBER, FVector::UpVector);
				const FVector OverlayWorldPosition = TerrainHit.ImpactPoint + SafeImpactNormal * ShoreOverlayOffset;
				ShoreVertices.Add(ActorTransform.InverseTransformPosition(OverlayWorldPosition));
				ShoreNormals.Add(ActorTransform.InverseTransformVectorNoScale(SafeImpactNormal).GetSafeNormal(SMALL_NUMBER, FVector::UpVector));
			}
			else
			{
				ShoreVertices.Add(LocalPosition - FVector(0.0, 0.0, MaximumDepth));
				ShoreNormals.Add(FVector::UpVector);
			}
			Normals.Add(FVector::UpVector);
			UVs.Add(FVector2D(U, V));
			VertexColors.Add(FLinearColor(EncodedDepth, BorderDistance, U, V));
			Tangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			ShoreTangents.Add(FProcMeshTangent(FVector::ForwardVector, false));
			TerrainHitMask.Add(bHitTerrain);
			SignedDepths.Add(SignedDepth);
		}
	}
	if (!bTraceTerrain)
	{
		LastDepthBakeResult = FString::Printf(TEXT("Terrain trace skipped; uniform %.1f cm depth"), MaximumDepth);
	}
	else if (HitSampleCount == 0)
	{
		LastDepthBakeResult = FString::Printf(TEXT("0 / %d samples hit the selected trace channel"), VertexCount);
		UE_LOG(LogStylizedWater, Warning, TEXT("StylizedWater: %s has no terrain depth hits. Check its Z position and Terrain Trace Channel."), *GetName());
	}
	else
	{
		LastDepthBakeResult = FString::Printf(
			TEXT("%d / %d hits; %.1f to %.1f cm (range %.1f cm)"),
			HitSampleCount,
			VertexCount,
			MinimumHitDepth,
			MaximumHitDepth,
			MaximumHitDepth - MinimumHitDepth);
	}

	for (int32 Y = 0; Y < ResolutionY; ++Y)
	{
		for (int32 X = 0; X < ResolutionX; ++X)
		{
			const int32 I00 = Y * (ResolutionX + 1) + X;
			const int32 I10 = I00 + 1;
			const int32 I01 = I00 + ResolutionX + 1;
			const int32 I11 = I01 + 1;

			Triangles.Add(I00);
			Triangles.Add(I11);
			Triangles.Add(I10);
			Triangles.Add(I00);
			Triangles.Add(I01);
			Triangles.Add(I11);

			const bool bCellHasTerrain =
				TerrainHitMask[I00] && TerrainHitMask[I10] && TerrainHitMask[I01] && TerrainHitMask[I11];
			const bool bCellTouchesShore =
				SignedDepths[I00] <= ShoreFoamDepth || SignedDepths[I10] <= ShoreFoamDepth ||
				SignedDepths[I01] <= ShoreFoamDepth || SignedDepths[I11] <= ShoreFoamDepth;
			if (bEnableTerrainShoreOverlay && bTraceTerrain && bCellHasTerrain && bCellTouchesShore)
			{
				ShoreTriangles.Add(I00);
				ShoreTriangles.Add(I11);
				ShoreTriangles.Add(I10);
				ShoreTriangles.Add(I00);
				ShoreTriangles.Add(I01);
				ShoreTriangles.Add(I11);
			}
		}
	}
	if (bTraceTerrain)
	{
		LastDepthBakeResult += bEnableTerrainShoreOverlay
			? FString::Printf(TEXT("; shore overlay %d triangles"), ShoreTriangles.Num() / 3)
			: TEXT("; shore overlay disabled");
	}

	WaterSurface->ClearAllMeshSections();
	WaterSurface->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, false);
	WaterSurface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShoreOverlay->ClearAllMeshSections();
	if (!ShoreTriangles.IsEmpty())
	{
		ShoreOverlay->CreateMeshSection_LinearColor(0, ShoreVertices, ShoreTriangles, ShoreNormals, UVs, VertexColors, ShoreTangents, false);
		ShoreOverlay->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	bShoreOverlayBakeInitialized = true;
	DynamicMaterial = nullptr;
	EnsureDynamicMaterial();
	UpdateMaterialParameters();

#if WITH_EDITOR
	WaterSurface->MarkRenderStateDirty();
	ShoreOverlay->MarkRenderStateDirty();
	MarkPackageDirty();
#endif
}

void AStylizedWaterBodyActor::EnsureDynamicMaterial()
{
	if (!WaterSurface || !ShoreOverlay || DynamicMaterial || HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	UMaterialInterface* ParentMaterial = TemplateMaterialInstance.LoadSynchronous();
	if (!ParentMaterial)
	{
		ParentMaterial = LoadObject<UMaterialInterface>(nullptr, StylizedWater::GeneratedMaterialInstancePath);
	}
	if (!ParentMaterial)
	{
		return;
	}

	DynamicMaterial = UMaterialInstanceDynamic::Create(ParentMaterial, this);
	WaterSurface->SetMaterial(0, DynamicMaterial);
	ShoreOverlay->SetMaterial(0, DynamicMaterial);
}

void AStylizedWaterBodyActor::UpdateMaterialParameters()
{
	EnsureDynamicMaterial();
	if (!DynamicMaterial)
	{
		return;
	}

	FVector2D SafeFlowDirection = FlowDirection.GetSafeNormal();
	if (SafeFlowDirection.IsNearlyZero())
	{
		SafeFlowDirection = FVector2D::UnitX();
	}

	DynamicMaterial->SetVectorParameterValue(StylizedWater::ShallowColorName, ShallowColor);
	DynamicMaterial->SetVectorParameterValue(StylizedWater::MidColorName, MidColor);
	DynamicMaterial->SetVectorParameterValue(StylizedWater::DeepColorName, DeepColor);
	DynamicMaterial->SetVectorParameterValue(StylizedWater::FoamColorName, FoamColor);
	DynamicMaterial->SetVectorParameterValue(StylizedWater::FlowDirectionName, FLinearColor(SafeFlowDirection.X, SafeFlowDirection.Y, 0.0f, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::DepthColorRangeName, FMath::Max(DepthColorRange, 10.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::MidColorPositionName, FMath::Clamp(MidColorPosition, 0.05f, 0.95f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::DepthGradientInfluenceName, FMath::Clamp(DepthGradientInfluence, 0.0f, 1.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::OpacityName, FMath::Clamp(Opacity, 0.0f, 1.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::RoughnessName, FMath::Clamp(Roughness, 0.0f, 1.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::DistortionStrengthName, FMath::Max(DistortionStrength, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::EmissiveStrengthName, FMath::Max(EmissiveStrength, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::WaterLevelOffsetName, WaterLevelOffset);
	DynamicMaterial->SetScalarParameterValue(StylizedWater::WaterlineSoftnessName, FMath::Max(WaterlineSoftness, 0.1f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::ShoreRunupName, FMath::Max(ShoreRunup, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::ShoreWavelengthName, FMath::Max(ShoreWavelength, 20.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::ShoreWaveSpeedName, FMath::Max(ShoreWaveSpeed, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::ShoreFoamDepthName, FMath::Max(ShoreFoamDepth, 10.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::ShoreFoamWidthName, FMath::Clamp(ShoreFoamWidth, 0.01f, 0.49f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::FoamIntensityName, FMath::Max(FoamIntensity, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::FlowSpeedName, FMath::Max(FlowSpeed, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::WaveWorldScaleName, FMath::Max(WaveWorldScale, 0.00001f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::GeometryWaveAmplitudeName, FMath::Max(GeometryWaveAmplitude, 0.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::DryRangeName, FMath::Max(MaximumDryHeight, 10.0f));
	DynamicMaterial->SetScalarParameterValue(StylizedWater::DepthRangeName, FMath::Max(MaximumDepth, 10.0f));
}

#if WITH_EDITOR
void AStylizedWaterBodyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();
	const FName MemberPropertyName = PropertyChangedEvent.GetMemberPropertyName();
	const FName ChangedPropertyName = MemberPropertyName.IsNone() ? PropertyName : MemberPropertyName;
	const bool bGeometryChanged =
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, SurfaceSize) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, GridResolution) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, MaximumDryHeight) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, MaximumDepth) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, TraceHeight) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, TerrainTraceChannel) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, bEnableTerrainShoreOverlay) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, ShoreOverlayOffset) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, ShoreFoamDepth) ||
		ChangedPropertyName == GET_MEMBER_NAME_CHECKED(AStylizedWaterBodyActor, bSampleTerrainOnRebuild);

	if (bGeometryChanged)
	{
		BuildWaterMesh(bSampleTerrainOnRebuild);
	}
	else
	{
		UpdateMaterialParameters();
	}
}

void AStylizedWaterBodyActor::PostEditMove(const bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished && !HasAnyFlags(RF_ClassDefaultObject))
	{
		BuildWaterMesh(bSampleTerrainOnRebuild);
	}
}
#endif
