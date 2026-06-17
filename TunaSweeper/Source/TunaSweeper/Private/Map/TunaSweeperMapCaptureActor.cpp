#include "Map/TunaSweeperMapCaptureActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/SceneComponent.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "HAL/FileManager.h"
#include "IImageWrapper.h"
#include "IImageWrapperModule.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "RenderingThread.h"
#include "TextureResource.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperMapCapture, Log, All);

namespace TunaSweeperMapCapture
{
	constexpr float PreviewHeight = 40.0f;
	constexpr float CaptureHeight = 12000.0f;
	constexpr int32 MinLongSideResolution = 256;
	constexpr int32 MaxLongSideResolution = 8192;
}

ATunaSweeperMapCaptureActor::ATunaSweeperMapCaptureActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	BoundsPreviewComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsPreview"));
	BoundsPreviewComponent->SetupAttachment(SceneRoot);
	BoundsPreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsPreviewComponent->SetGenerateOverlapEvents(false);
	BoundsPreviewComponent->SetHiddenInGame(true);
	BoundsPreviewComponent->SetVisibility(true);
	BoundsPreviewComponent->ShapeColor = FColor(50, 210, 255, 255);

	SceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MapSceneCapture"));
	SceneCaptureComponent->SetupAttachment(SceneRoot);
	SceneCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, TunaSweeperMapCapture::CaptureHeight));
	SceneCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	SceneCaptureComponent->bCaptureEveryFrame = false;
	SceneCaptureComponent->bCaptureOnMovement = false;
	SceneCaptureComponent->bAlwaysPersistRenderingState = true;
	SceneCaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCaptureComponent->SetHiddenInGame(false);

	IncludedActorTags.Add(FName(TEXT("MapCapture")));
	UpdatePreviewComponents();
}

void ATunaSweeperMapCaptureActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdatePreviewComponents();
}

#if WITH_EDITOR
void ATunaSweeperMapCaptureActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	UpdatePreviewComponents();
}

bool ATunaSweeperMapCaptureActor::RunCaptureOpaqueRgbPngForEditor()
{
	return CaptureOpaqueRgbPngInternal();
}

bool ATunaSweeperMapCaptureActor::RunAutoDetectBoundsAndCaptureOpaqueRgbPngForEditor()
{
	if (!AutoDetectCaptureBoundsInternal())
	{
		return false;
	}

	const bool bOriginalAutoDetectBeforeCapture = bAutoDetectBoundsBeforeCapture;
	bAutoDetectBoundsBeforeCapture = false;
	const bool bCaptured = CaptureOpaqueRgbPngInternal();
	bAutoDetectBoundsBeforeCapture = bOriginalAutoDetectBeforeCapture;
	return bCaptured;
}

FString ATunaSweeperMapCaptureActor::GetLastWrittenRgbPngAbsolutePathForEditor() const
{
	return LastWrittenRgbPngAbsolutePath;
}

FString ATunaSweeperMapCaptureActor::ResolveImportDestinationPathForEditor() const
{
	return ImportDestinationPath.IsEmpty() ? TEXT("/Game/UI/Map") : ImportDestinationPath;
}

FString ATunaSweeperMapCaptureActor::ResolveImportAssetNameForEditor() const
{
	FString ResolvedName = ImportAssetNamePattern;
	if (ResolvedName.IsEmpty())
	{
		ResolvedName = TEXT("T_UIMap_{level}_RGB");
	}

	ResolvedName.ReplaceInline(TEXT("{level}"), *ResolveLevelName(), ESearchCase::IgnoreCase);
	return FPaths::GetBaseFilename(ResolvedName);
}
#endif

void ATunaSweeperMapCaptureActor::AutoDetectCaptureBounds()
{
	AutoDetectCaptureBoundsInternal();
}

void ATunaSweeperMapCaptureActor::CaptureOpaqueRgbPng()
{
	CaptureOpaqueRgbPngInternal();
}

void ATunaSweeperMapCaptureActor::AutoDetectBoundsAndCaptureOpaqueRgbPng()
{
	if (AutoDetectCaptureBoundsInternal())
	{
		const bool bOriginalAutoDetectBeforeCapture = bAutoDetectBoundsBeforeCapture;
		bAutoDetectBoundsBeforeCapture = false;
		CaptureOpaqueRgbPngInternal();
		bAutoDetectBoundsBeforeCapture = bOriginalAutoDetectBeforeCapture;
	}
}

FVector2D ATunaSweeperMapCaptureActor::WorldLocationToMapUV(const FVector& WorldLocation) const
{
	const FTransform CaptureTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), GetActorLocation());
	const FVector LocalPosition = CaptureTransform.InverseTransformPosition(WorldLocation);
	const FVector2D SafeSize(
		FMath::Max(1.0, CaptureWorldSize.X),
		FMath::Max(1.0, CaptureWorldSize.Y));

	return FVector2D(
		0.5 + LocalPosition.Y / SafeSize.Y,
		0.5 - LocalPosition.X / SafeSize.X);
}

FVector ATunaSweeperMapCaptureActor::MapUVToWorldLocation(const FVector2D& MapUV, float WorldZ) const
{
	const FVector LocalPosition(
		(0.5 - MapUV.Y) * CaptureWorldSize.X,
		(MapUV.X - 0.5) * CaptureWorldSize.Y,
		0.0f);
	const FTransform CaptureTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), GetActorLocation());
	FVector WorldPosition = CaptureTransform.TransformPosition(LocalPosition);
	WorldPosition.Z = WorldZ;
	return WorldPosition;
}

void ATunaSweeperMapCaptureActor::UpdatePreviewComponents()
{
	CaptureWorldSize.X = FMath::Max(100.0, CaptureWorldSize.X);
	CaptureWorldSize.Y = FMath::Max(100.0, CaptureWorldSize.Y);
	AutoDetectGridStepCm = FMath::Max(25.0f, AutoDetectGridStepCm);
	AutoDetectSearchExtent.X = FMath::Max(static_cast<double>(AutoDetectGridStepCm), AutoDetectSearchExtent.X);
	AutoDetectSearchExtent.Y = FMath::Max(static_cast<double>(AutoDetectGridStepCm), AutoDetectSearchExtent.Y);

	if (BoundsPreviewComponent)
	{
		BoundsPreviewComponent->SetBoxExtent(FVector(
			CaptureWorldSize.X * 0.5f,
			CaptureWorldSize.Y * 0.5f,
			TunaSweeperMapCapture::PreviewHeight));
		BoundsPreviewComponent->SetRelativeLocation(FVector::ZeroVector);
	}

	if (SceneCaptureComponent)
	{
		SceneCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, TunaSweeperMapCapture::CaptureHeight));
		SceneCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
		SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
		SceneCaptureComponent->OrthoWidth = FMath::Max(100.0, CaptureWorldSize.Y);
	}
}

bool ATunaSweeperMapCaptureActor::AutoDetectCaptureBoundsInternal()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture auto-detect failed: no world."));
		return false;
	}

	const double Step = FMath::Max(25.0, static_cast<double>(AutoDetectGridStepCm));
	const int32 StepCountX = FMath::Max(1, FMath::CeilToInt(AutoDetectSearchExtent.X * 2.0f / Step));
	const int32 StepCountY = FMath::Max(1, FMath::CeilToInt(AutoDetectSearchExtent.Y * 2.0f / Step));
	const FTransform ActorTransform(FRotator(0.0f, GetActorRotation().Yaw, 0.0f), GetActorLocation());
	const float ActorZ = GetActorLocation().Z;
	FBox2D HitBounds(ForceInit);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperMapCaptureAutoBounds), false);
	QueryParams.AddIgnoredActor(this);

	for (int32 XIndex = 0; XIndex <= StepCountX; ++XIndex)
	{
		const float LocalX = -AutoDetectSearchExtent.X + XIndex * Step;
		for (int32 YIndex = 0; YIndex <= StepCountY; ++YIndex)
		{
			const float LocalY = -AutoDetectSearchExtent.Y + YIndex * Step;
			const FVector LocalSample(LocalX, LocalY, 0.0f);
			const FVector WorldSample = ActorTransform.TransformPosition(LocalSample);
			const FVector TraceStart(WorldSample.X, WorldSample.Y, ActorZ + AutoDetectTraceStartHeight);
			const FVector TraceEnd(WorldSample.X, WorldSample.Y, ActorZ + AutoDetectTraceStartHeight - AutoDetectTraceDepth);

			FHitResult Hit;
			if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, AutoDetectTraceChannel, QueryParams))
			{
				continue;
			}

			if (!IsBoundsHitUsable(Hit))
			{
				continue;
			}

			HitBounds += FVector2D(LocalX, LocalY);
		}
	}

	if (!HitBounds.bIsValid)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture auto-detect found no valid geometry."));
		return false;
	}

	HitBounds.Min -= FVector2D(BoundsPaddingCm, BoundsPaddingCm);
	HitBounds.Max += FVector2D(BoundsPaddingCm, BoundsPaddingCm);

	const FVector2D LocalCenter = HitBounds.GetCenter();
	LastDetectedLocalMin = HitBounds.Min - LocalCenter;
	LastDetectedLocalMax = HitBounds.Max - LocalCenter;

	const FVector WorldCenter = ActorTransform.TransformPosition(FVector(LocalCenter.X, LocalCenter.Y, 0.0f));
	SetActorLocation(FVector(WorldCenter.X, WorldCenter.Y, ActorZ), false, nullptr, ETeleportType::TeleportPhysics);
	CaptureWorldSize = FVector2D(
		FMath::Max(100.0, HitBounds.GetSize().X),
		FMath::Max(100.0, HitBounds.GetSize().Y));

	UpdatePreviewComponents();
	Modify();

	UE_LOG(
		LogTunaSweeperMapCapture,
		Display,
		TEXT("Map capture bounds detected. Center=(%.1f, %.1f), Size=(%.1f, %.1f)."),
		GetActorLocation().X,
		GetActorLocation().Y,
		CaptureWorldSize.X,
		CaptureWorldSize.Y);
	return true;
}

bool ATunaSweeperMapCaptureActor::CaptureOpaqueRgbPngInternal()
{
	if (bAutoDetectBoundsBeforeCapture && !AutoDetectCaptureBoundsInternal())
	{
		return false;
	}

	if (!SceneCaptureComponent)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: no scene capture component."));
		return false;
	}

	const FIntPoint Resolution = ResolveCaptureResolution();
	if (Resolution.X <= 0 || Resolution.Y <= 0)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: invalid resolution."));
		return false;
	}

	UTextureRenderTarget2D* RenderTarget = NewObject<UTextureRenderTarget2D>(this, TEXT("MapCaptureRenderTarget"), RF_Transient);
	if (!RenderTarget)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: could not create render target."));
		return false;
	}

	RenderTarget->RenderTargetFormat = RTF_RGBA8;
	RenderTarget->ClearColor = FLinearColor::Black;
	RenderTarget->bAutoGenerateMips = false;
	RenderTarget->InitAutoFormat(Resolution.X, Resolution.Y);
	RenderTarget->UpdateResourceImmediate(true);

	UpdatePreviewComponents();

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: no world."));
		return false;
	}

	World->UpdateWorldComponents(true, false);
	FlushRenderingCommands();

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParameters.ObjectFlags |= RF_Transient;

	const FVector CaptureLocation = GetActorLocation() + FVector(0.0f, 0.0f, TunaSweeperMapCapture::CaptureHeight);
	const FRotator CaptureRotation(-90.0f, GetActorRotation().Yaw, 0.0f);
	ASceneCapture2D* CaptureActor = World->SpawnActor<ASceneCapture2D>(
		CaptureLocation,
		CaptureRotation,
		SpawnParameters);
	if (!CaptureActor || !CaptureActor->GetCaptureComponent2D())
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: could not spawn scene capture actor."));
		return false;
	}

	USceneCaptureComponent2D* CaptureComponent = CaptureActor->GetCaptureComponent2D();
	CaptureComponent->TextureTarget = RenderTarget;
	CaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	CaptureComponent->OrthoWidth = FMath::Max(100.0, CaptureWorldSize.Y);
	CaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	CaptureComponent->bCaptureEveryFrame = false;
	CaptureComponent->bCaptureOnMovement = false;
	CaptureComponent->bAlwaysPersistRenderingState = true;
	CaptureComponent->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	CaptureComponent->ShowFlags.SetTemporalAA(false);
	CaptureComponent->SetHiddenInGame(false);
	CaptureComponent->MarkRenderStateDirty();
	CaptureComponent->CaptureScene();
	FlushRenderingCommands();

	FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
	if (!RenderTargetResource)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: no render target resource."));
		CaptureComponent->TextureTarget = nullptr;
		CaptureActor->Destroy();
		return false;
	}

	TArray<FColor> Pixels;
	if (!RenderTargetResource->ReadPixels(Pixels) || Pixels.Num() != Resolution.X * Resolution.Y)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: could not read pixels."));
		CaptureComponent->TextureTarget = nullptr;
		CaptureActor->Destroy();
		return false;
	}

	CaptureComponent->TextureTarget = nullptr;
	CaptureActor->Destroy();

	for (FColor& Pixel : Pixels)
	{
		Pixel.A = 255;
	}

	const FString OutputPath = ResolveRgbOutputPath();
	const bool bSaved = WritePngFile(OutputPath, Pixels, Resolution.X, Resolution.Y);

	if (!bSaved)
	{
		UE_LOG(LogTunaSweeperMapCapture, Warning, TEXT("Map capture failed: could not save PNG to %s."), *OutputPath);
		return false;
	}

	LastCaptureResolution = Resolution;
	LastWrittenRgbPngAbsolutePath = OutputPath;
	Modify();

	UE_LOG(LogTunaSweeperMapCapture, Display, TEXT("Map capture saved RGB PNG: %s"), *OutputPath);
	return true;
}

bool ATunaSweeperMapCaptureActor::IsBoundsHitUsable(const FHitResult& Hit) const
{
	AActor* HitActor = Hit.GetActor();
	if (!HitActor || HitActor == this)
	{
		return false;
	}

	if (bRequireIncludedActorTag)
	{
		bool bHasIncludedTag = false;
		for (const FName& IncludedTag : IncludedActorTags)
		{
			if (!IncludedTag.IsNone() && HitActor->ActorHasTag(IncludedTag))
			{
				bHasIncludedTag = true;
				break;
			}
		}

		if (!bHasIncludedTag)
		{
			return false;
		}
	}

	const UPrimitiveComponent* HitComponent = Hit.GetComponent();
	if (bIgnoreMovableComponentsForBounds && HitComponent && HitComponent->Mobility != EComponentMobility::Static)
	{
		return false;
	}

	return true;
}

FIntPoint ATunaSweeperMapCaptureActor::ResolveCaptureResolution() const
{
	const int32 ClampedLongSide = FMath::Clamp(
		LongSideResolution,
		TunaSweeperMapCapture::MinLongSideResolution,
		TunaSweeperMapCapture::MaxLongSideResolution);
	const double SizeX = FMath::Max(1.0, CaptureWorldSize.X);
	const double SizeY = FMath::Max(1.0, CaptureWorldSize.Y);

	if (SizeX >= SizeY)
	{
		return FIntPoint(
			FMath::Max(1, FMath::RoundToInt(ClampedLongSide * SizeY / SizeX)),
			ClampedLongSide);
	}

	return FIntPoint(
		ClampedLongSide,
		FMath::Max(1, FMath::RoundToInt(ClampedLongSide * SizeX / SizeY)));
}

FString ATunaSweeperMapCaptureActor::ResolveRgbOutputPath() const
{
	FString ResolvedPath = RgbPngOutputPath;
	if (ResolvedPath.IsEmpty())
	{
		ResolvedPath = TEXT("Saved/MapCaptures/{level}_Map_RGB.png");
	}

	ResolvedPath.ReplaceInline(TEXT("{level}"), *ResolveLevelName(), ESearchCase::IgnoreCase);
	return FPaths::ConvertRelativePathToFull(
		FPaths::IsRelative(ResolvedPath)
			? FPaths::Combine(FPaths::ProjectDir(), ResolvedPath)
			: ResolvedPath);
}

FString ATunaSweeperMapCaptureActor::ResolveLevelName() const
{
	FString LevelName = TEXT("Level");
	if (const UWorld* World = GetWorld())
	{
		if (const UPackage* Package = World->GetOutermost())
		{
			LevelName = FPackageName::GetShortName(Package->GetName());
		}
	}

	return LevelName;
}

bool ATunaSweeperMapCaptureActor::WritePngFile(const FString& AbsolutePath, const TArray<FColor>& Pixels, int32 Width, int32 Height) const
{
	if (Pixels.Num() != Width * Height || Width <= 0 || Height <= 0)
	{
		return false;
	}

	IFileManager::Get().MakeDirectory(*FPaths::GetPath(AbsolutePath), true);

	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	const TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	if (!ImageWrapper.IsValid())
	{
		return false;
	}

	if (!ImageWrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Width, Height, ERGBFormat::BGRA, 8))
	{
		return false;
	}

	const TArray64<uint8>& PngData64 = ImageWrapper->GetCompressed(100);
	if (PngData64.Num() <= 0 || PngData64.Num() > MAX_int32)
	{
		return false;
	}

	TArray<uint8> PngData;
	PngData.Append(PngData64.GetData(), static_cast<int32>(PngData64.Num()));
	return FFileHelper::SaveArrayToFile(PngData, *AbsolutePath);
}
