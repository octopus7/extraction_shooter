#include "Component/TunaSweeperPlayerVisionComponent.h"

#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/TunaSweeperVisionMaskWidget.h"

UTunaSweeperPlayerVisionComponent::UTunaSweeperPlayerVisionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UTunaSweeperPlayerVisionComponent::BeginPlay()
{
	Super::BeginPlay();
	TimeSinceLastMaskUpdate = VisionSettings.UpdateIntervalSeconds;
}

void UTunaSweeperPlayerVisionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (VisionMaskWidget)
	{
		VisionMaskWidget->RemoveFromParent();
		VisionMaskWidget = nullptr;
	}

	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperPlayerVisionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!ShouldUpdateVision())
	{
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskVisible(false);
		}
		return;
	}

	APlayerController* PlayerController = ResolveLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	EnsureOverlayWidget(PlayerController);

	const float UpdateInterval = FMath::Max(0.0f, VisionSettings.UpdateIntervalSeconds);
	TimeSinceLastMaskUpdate += FMath::Max(0.0f, DeltaTime);
	if (UpdateInterval > 0.0f && TimeSinceLastMaskUpdate < UpdateInterval)
	{
		return;
	}

	TimeSinceLastMaskUpdate = 0.0f;
	ForceRefreshVisionMask();
}

void UTunaSweeperPlayerVisionComponent::ForceRefreshVisionMask()
{
	APlayerController* PlayerController = ResolveLocalPlayerController();
	if (!PlayerController)
	{
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		return;
	}

	if (!EnsureMaskTexture(FIntPoint(ViewportX, ViewportY)))
	{
		return;
	}

	if (bRenderVisionOverlay)
	{
		EnsureOverlayWidget(PlayerController);
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskTexture(MaskTexture);
			VisionMaskWidget->SetMaskVisible(true);
		}
	}

	TArray<FVector2D> VisiblePolygonPoints;
	if (!BuildVisiblePolygonPoints(PlayerController, VisiblePolygonPoints))
	{
		return;
	}

	RasterizeVisiblePolygon(VisiblePolygonPoints);
	ApplyBlurToMask();
	RebuildTexturePixels();
	UploadMaskTexture();
}

APlayerController* UTunaSweeperPlayerVisionComponent::ResolveLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return nullptr;
	}

	return Cast<APlayerController>(OwnerPawn->GetController());
}

bool UTunaSweeperPlayerVisionComponent::ShouldUpdateVision() const
{
	return IsActive() && ResolveLocalPlayerController() && (bRenderVisionOverlay || IsVisionDebugEnabled());
}

bool UTunaSweeperPlayerVisionComponent::IsVisionDebugEnabled() const
{
	const UWorld* World = GetWorld();
	const UTunaSweeperGameInstance* TunaGameInstance = World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	return TunaGameInstance && TunaGameInstance->IsVisionDebugEnabled();
}

void UTunaSweeperPlayerVisionComponent::EnsureOverlayWidget(APlayerController* PlayerController)
{
	if (!bRenderVisionOverlay)
	{
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskVisible(false);
		}
		return;
	}

	if (VisionMaskWidget || !PlayerController || !PlayerController->IsLocalController())
	{
		return;
	}

	VisionMaskWidget = CreateWidget<UTunaSweeperVisionMaskWidget>(
		PlayerController,
		UTunaSweeperVisionMaskWidget::StaticClass());
	if (VisionMaskWidget)
	{
		VisionMaskWidget->SetMaskTexture(MaskTexture);
		VisionMaskWidget->SetMaskVisible(true);
		VisionMaskWidget->AddToViewport(OverlayZOrder);
	}
}

bool UTunaSweeperPlayerVisionComponent::EnsureMaskTexture(const FIntPoint& InViewportSize)
{
	const int32 DownsampleFactor = FMath::Clamp(VisionSettings.MaskDownsampleFactor, 1, 16);
	const FIntPoint NewMaskSize(
		FMath::Max(2, FMath::DivideAndRoundUp(InViewportSize.X, DownsampleFactor)),
		FMath::Max(2, FMath::DivideAndRoundUp(InViewportSize.Y, DownsampleFactor)));

	if (MaskTexture && MaskSize == NewMaskSize && ViewportSize == InViewportSize)
	{
		return true;
	}

	ViewportSize = InViewportSize;
	MaskSize = NewMaskSize;
	MaskTexture = UTexture2D::CreateTransient(MaskSize.X, MaskSize.Y, PF_B8G8R8A8);
	if (!MaskTexture)
	{
		return false;
	}

	MaskTexture->CompressionSettings = TC_EditorIcon;
	MaskTexture->MipGenSettings = TMGS_NoMipmaps;
	MaskTexture->LODGroup = TEXTUREGROUP_UI;
	MaskTexture->SRGB = false;
	MaskTexture->NeverStream = true;
	MaskTexture->Filter = TF_Bilinear;
	MaskTexture->AddressX = TA_Clamp;
	MaskTexture->AddressY = TA_Clamp;
	MaskTexture->UpdateResource();

	const int32 PixelCount = MaskSize.X * MaskSize.Y;
	VisibilityMask.SetNumUninitialized(PixelCount);
	BlurScratchMask.SetNumUninitialized(PixelCount);
	BlurredMask.SetNumUninitialized(PixelCount);
	TexturePixels.SetNumUninitialized(PixelCount * 4);

	if (VisionMaskWidget)
	{
		VisionMaskWidget->SetMaskTexture(MaskTexture);
	}

	return true;
}

bool UTunaSweeperPlayerVisionComponent::BuildVisiblePolygonPoints(
	APlayerController* PlayerController,
	TArray<FVector2D>& OutMaskPoints)
{
	const AActor* OwnerActor = GetOwner();
	const UWorld* World = GetWorld();
	if (!PlayerController || !OwnerActor || !World || MaskSize.X <= 0 || MaskSize.Y <= 0 ||
		ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return false;
	}

	const int32 RayCount = FMath::Clamp(VisionSettings.RayCount, 16, 720);
	const float AngleStepDegrees = 360.0f / static_cast<float>(RayCount);
	const float HalfFieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f) * 0.5f;
	const FVector TraceOrigin = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, VisionSettings.TraceHeight));
	FVector FacingDirection = OwnerActor->GetActorForwardVector();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.Normalize())
	{
		FacingDirection = FVector::ForwardVector;
	}

	const float FacingYaw = FacingDirection.Rotation().Yaw;
	const float MaskScaleX = static_cast<float>(MaskSize.X) / static_cast<float>(ViewportSize.X);
	const float MaskScaleY = static_cast<float>(MaskSize.Y) / static_cast<float>(ViewportSize.Y);
	const bool bDebugEnabled = IsVisionDebugEnabled();

	OutMaskPoints.Reset(RayCount);
	for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
	{
		const float AngleDegrees = static_cast<float>(RayIndex) * AngleStepDegrees;
		FVector Direction = FVector::ForwardVector.RotateAngleAxis(AngleDegrees, FVector::UpVector);
		Direction.Z = 0.0f;
		Direction.Normalize();

		const float DirectionYaw = Direction.Rotation().Yaw;
		const bool bInsideFieldOfView =
			FMath::Abs(FMath::FindDeltaAngleDegrees(FacingYaw, DirectionYaw)) <= HalfFieldOfViewDegrees;

		FHitResult Hit;
		const float VisibleDistance = TraceVisibleDistance(TraceOrigin, Direction, bInsideFieldOfView, Hit);
		if (bDebugEnabled)
		{
			DrawVisionDebug(TraceOrigin, Direction, VisibleDistance, bInsideFieldOfView, RayIndex, Hit);
		}

		FVector2D ScreenPoint;
		if (!PlayerController->ProjectWorldLocationToScreen(
			TraceOrigin + Direction * VisibleDistance,
			ScreenPoint,
			true))
		{
			continue;
		}

		OutMaskPoints.Add(FVector2D(ScreenPoint.X * MaskScaleX, ScreenPoint.Y * MaskScaleY));
	}

	if (bDebugEnabled)
	{
		DrawVisionDebugBounds(TraceOrigin, FacingDirection);
	}

	return OutMaskPoints.Num() >= 3;
}

float UTunaSweeperPlayerVisionComponent::TraceVisibleDistance(
	const FVector& TraceOrigin,
	const FVector& Direction,
	bool bInsideFieldOfView,
	FHitResult& OutHit) const
{
	const float AlwaysVisibleRadius = FMath::Max(0.0f, VisionSettings.AlwaysVisibleRadius);
	const float TraceDistance = FMath::Max(AlwaysVisibleRadius, VisionSettings.SightDistance);
	const FVector TraceEnd = TraceOrigin + Direction * TraceDistance;

	UWorld* World = GetWorld();
	if (!World || TraceDistance <= 0.0f)
	{
		OutHit = FHitResult();
		return AlwaysVisibleRadius;
	}

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperPlayerVisionTrace), false);
	if (const AActor* OwnerActor = GetOwner())
	{
		QueryParams.AddIgnoredActor(OwnerActor);
	}

	const ECollisionChannel TraceChannel = static_cast<ECollisionChannel>(VisionSettings.TraceChannel.GetValue());
	const bool bHit = World->LineTraceSingleByChannel(OutHit, TraceOrigin, TraceEnd, TraceChannel, QueryParams);
	if (!bInsideFieldOfView)
	{
		return AlwaysVisibleRadius;
	}

	const float BlockedDistance = bHit && OutHit.bBlockingHit
		? FMath::Max(0.0f, OutHit.Distance)
		: TraceDistance;
	return FMath::Max(AlwaysVisibleRadius, BlockedDistance);
}

void UTunaSweeperPlayerVisionComponent::RasterizeVisiblePolygon(const TArray<FVector2D>& PolygonPoints)
{
	const int32 Width = MaskSize.X;
	const int32 Height = MaskSize.Y;
	const uint8 HiddenAlpha = static_cast<uint8>(FMath::Clamp(VisionSettings.HiddenMaskAlpha, 0, 255));
	VisibilityMask.Init(HiddenAlpha, Width * Height);

	if (PolygonPoints.Num() < 3 || Width <= 0 || Height <= 0)
	{
		return;
	}

	// The radial fan is converted to one screen-space polygon and filled once to avoid per-sector draw calls.
	for (int32 Y = 0; Y < Height; ++Y)
	{
		const float ScanY = static_cast<float>(Y) + 0.5f;
		ScanlineIntersections.Reset();

		for (int32 PointIndex = 0; PointIndex < PolygonPoints.Num(); ++PointIndex)
		{
			const FVector2D& A = PolygonPoints[PointIndex];
			const FVector2D& B = PolygonPoints[(PointIndex + 1) % PolygonPoints.Num()];
			if ((A.Y <= ScanY && B.Y > ScanY) || (B.Y <= ScanY && A.Y > ScanY))
			{
				const float Denominator = B.Y - A.Y;
				if (!FMath::IsNearlyZero(Denominator))
				{
					const float Alpha = (ScanY - A.Y) / Denominator;
					ScanlineIntersections.Add(A.X + Alpha * (B.X - A.X));
				}
			}
		}

		ScanlineIntersections.Sort();
		for (int32 IntersectionIndex = 0; IntersectionIndex + 1 < ScanlineIntersections.Num(); IntersectionIndex += 2)
		{
			const float Left = ScanlineIntersections[IntersectionIndex];
			const float Right = ScanlineIntersections[IntersectionIndex + 1];
			const int32 StartX = FMath::Clamp(FMath::CeilToInt(FMath::Min(Left, Right)), 0, Width - 1);
			const int32 EndX = FMath::Clamp(FMath::FloorToInt(FMath::Max(Left, Right)), 0, Width - 1);
			if (EndX < StartX)
			{
				continue;
			}

			uint8* Row = VisibilityMask.GetData() + Y * Width;
			for (int32 X = StartX; X <= EndX; ++X)
			{
				Row[X] = 0;
			}
		}
	}
}

void UTunaSweeperPlayerVisionComponent::ApplyBlurToMask()
{
	const int32 Width = MaskSize.X;
	const int32 Height = MaskSize.Y;
	const int32 PixelCount = Width * Height;
	const int32 Radius = FMath::Clamp(VisionSettings.BlurRadius, 0, 16);
	if (Radius <= 0 || PixelCount <= 0)
	{
		BlurredMask = VisibilityMask;
		return;
	}

	BlurScratchMask.SetNumUninitialized(PixelCount);
	BlurredMask.SetNumUninitialized(PixelCount);

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			int32 Sum = 0;
			int32 Count = 0;
			for (int32 SampleX = FMath::Max(0, X - Radius); SampleX <= FMath::Min(Width - 1, X + Radius); ++SampleX)
			{
				Sum += VisibilityMask[Y * Width + SampleX];
				++Count;
			}

			BlurScratchMask[Y * Width + X] = static_cast<uint8>(Sum / FMath::Max(1, Count));
		}
	}

	for (int32 Y = 0; Y < Height; ++Y)
	{
		for (int32 X = 0; X < Width; ++X)
		{
			int32 Sum = 0;
			int32 Count = 0;
			for (int32 SampleY = FMath::Max(0, Y - Radius); SampleY <= FMath::Min(Height - 1, Y + Radius); ++SampleY)
			{
				Sum += BlurScratchMask[SampleY * Width + X];
				++Count;
			}

			BlurredMask[Y * Width + X] = static_cast<uint8>(Sum / FMath::Max(1, Count));
		}
	}
}

void UTunaSweeperPlayerVisionComponent::RebuildTexturePixels()
{
	const int32 PixelCount = MaskSize.X * MaskSize.Y;
	TexturePixels.SetNumUninitialized(PixelCount * 4);
	for (int32 PixelIndex = 0; PixelIndex < PixelCount; ++PixelIndex)
	{
		const uint8 Alpha = BlurredMask.IsValidIndex(PixelIndex) ? BlurredMask[PixelIndex] : 0;
		const int32 ByteIndex = PixelIndex * 4;
		TexturePixels[ByteIndex + 0] = 0;
		TexturePixels[ByteIndex + 1] = 0;
		TexturePixels[ByteIndex + 2] = 0;
		TexturePixels[ByteIndex + 3] = Alpha;
	}
}

void UTunaSweeperPlayerVisionComponent::UploadMaskTexture()
{
	if (!MaskTexture || MaskSize.X <= 0 || MaskSize.Y <= 0 || TexturePixels.IsEmpty())
	{
		return;
	}

	const int32 DataSize = TexturePixels.Num();
	uint8* UpdateData = static_cast<uint8*>(FMemory::Malloc(DataSize));
	FMemory::Memcpy(UpdateData, TexturePixels.GetData(), DataSize);

	FUpdateTextureRegion2D* UpdateRegion = new FUpdateTextureRegion2D(0, 0, 0, 0, MaskSize.X, MaskSize.Y);
	MaskTexture->UpdateTextureRegions(
		0,
		1,
		UpdateRegion,
		MaskSize.X * 4,
		4,
		UpdateData,
		[](uint8* InData, const FUpdateTextureRegion2D* InRegions)
		{
			FMemory::Free(InData);
			delete InRegions;
		});
}

void UTunaSweeperPlayerVisionComponent::DrawVisionDebug(
	const FVector& TraceOrigin,
	const FVector& Direction,
	float VisibleDistance,
	bool bInsideFieldOfView,
	int32 RayIndex,
	const FHitResult& Hit) const
{
	UWorld* World = GetWorld();
	if (!World || DebugRayStride <= 0 || RayIndex % DebugRayStride != 0)
	{
		return;
	}

	const FColor RayColor = bInsideFieldOfView ? FColor::Green : FColor::Cyan;
	DrawDebugLine(
		World,
		TraceOrigin,
		TraceOrigin + Direction * VisibleDistance,
		RayColor,
		false,
		DebugDrawLifeTime,
		0,
		bInsideFieldOfView ? 1.5f : 0.75f);

	if (Hit.bBlockingHit)
	{
		DrawDebugPoint(World, Hit.ImpactPoint, 6.0f, FColor::Red, false, DebugDrawLifeTime, 0);
	}
}

void UTunaSweeperPlayerVisionComponent::DrawVisionDebugBounds(
	const FVector& TraceOrigin,
	const FVector& FacingDirection) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float AlwaysVisibleRadius = FMath::Max(0.0f, VisionSettings.AlwaysVisibleRadius);
	const float SightDistance = FMath::Max(AlwaysVisibleRadius, VisionSettings.SightDistance);
	const float HalfFieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f) * 0.5f;
	const FVector LeftBoundary = FacingDirection.RotateAngleAxis(-HalfFieldOfViewDegrees, FVector::UpVector).GetSafeNormal();
	const FVector RightBoundary = FacingDirection.RotateAngleAxis(HalfFieldOfViewDegrees, FVector::UpVector).GetSafeNormal();

	DrawDebugCircle(
		World,
		TraceOrigin,
		AlwaysVisibleRadius,
		64,
		FColor::Cyan,
		false,
		DebugDrawLifeTime,
		0,
		1.5f,
		FVector::ForwardVector,
		FVector::RightVector,
		false);
	DrawDebugLine(World, TraceOrigin, TraceOrigin + LeftBoundary * SightDistance, FColor::Green, false, DebugDrawLifeTime, 0, 2.0f);
	DrawDebugLine(World, TraceOrigin, TraceOrigin + RightBoundary * SightDistance, FColor::Green, false, DebugDrawLifeTime, 0, 2.0f);
}
