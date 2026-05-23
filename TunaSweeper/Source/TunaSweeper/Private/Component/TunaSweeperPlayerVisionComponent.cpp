#include "Component/TunaSweeperPlayerVisionComponent.h"

#include "Blueprint/UserWidget.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/TunaSweeperVisionMaskWidget.h"

DEFINE_LOG_CATEGORY_STATIC(LogTunaSweeperPlayerVision, Log, All);

namespace TunaSweeperVision
{
	constexpr float FramedDefaultSightDistance = 810.0f;
	constexpr float CompactDefaultSightDistance = 450.0f;
	constexpr float RecentDefaultSightDistance = 900.0f;
	constexpr float LegacyDefaultSightDistance = 1800.0f;
	constexpr float FramedDefaultAlwaysVisibleRadius = 100.0f;
	constexpr float LegacyDefaultAlwaysVisibleRadius = 200.0f;
	constexpr int32 FramedMaskDownsampleFactor = 2;
	constexpr int32 LegacyMaskDownsampleFactor = 4;
	constexpr int32 FramedHiddenMaskAlpha = 77;
	constexpr int32 LegacyHiddenMaskAlpha = 220;
	constexpr float MaskFeatherPixels = 18.0f;

	struct FMaskBoundaryPoint
	{
		FVector2D Visible = FVector2D::ZeroVector;
		FVector2D Feather = FVector2D::ZeroVector;
		FVector2D Border = FVector2D::ZeroVector;
		bool bValid = false;
	};

	FVector2D FindViewportRayBorderPoint(const FVector2D& Origin, const FVector2D& Direction, const FIntPoint& InViewportSize)
	{
		float BestT = TNumericLimits<float>::Max();

		auto TestT = [&BestT](float T)
		{
			if (T >= 0.0f)
			{
				BestT = FMath::Min(BestT, T);
			}
		};

		if (!FMath::IsNearlyZero(Direction.X))
		{
			TestT((0.0f - Origin.X) / Direction.X);
			TestT((static_cast<float>(InViewportSize.X) - Origin.X) / Direction.X);
		}
		if (!FMath::IsNearlyZero(Direction.Y))
		{
			TestT((0.0f - Origin.Y) / Direction.Y);
			TestT((static_cast<float>(InViewportSize.Y) - Origin.Y) / Direction.Y);
		}

		if (BestT == TNumericLimits<float>::Max())
		{
			return Origin;
		}

		const FVector2D BorderPoint = Origin + Direction * BestT;
		return FVector2D(
			FMath::Clamp(BorderPoint.X, 0.0f, static_cast<float>(InViewportSize.X)),
			FMath::Clamp(BorderPoint.Y, 0.0f, static_cast<float>(InViewportSize.Y)));
	}

	float SampleVisibleDistanceForRelativeAngle(
		float RelativeAngleDegrees,
		float HalfFieldOfViewDegrees,
		float RayAngleStepDegrees,
		float AlwaysVisibleRadius,
		const TArray<float>& RayDistances)
	{
		if (FMath::Abs(RelativeAngleDegrees) > HalfFieldOfViewDegrees || RayDistances.IsEmpty())
		{
			return AlwaysVisibleRadius;
		}

		const float RayPosition =
			(RelativeAngleDegrees + HalfFieldOfViewDegrees) / FMath::Max(0.1f, RayAngleStepDegrees);
		const int32 RayIndexA = FMath::Clamp(FMath::FloorToInt(RayPosition), 0, RayDistances.Num() - 1);
		const int32 RayIndexB = FMath::Clamp(RayIndexA + 1, 0, RayDistances.Num() - 1);
		const float RayAlpha = FMath::Frac(RayPosition);
		return FMath::Max(AlwaysVisibleRadius, FMath::Lerp(RayDistances[RayIndexA], RayDistances[RayIndexB], RayAlpha));
	}

	FMaskBoundaryPoint BuildMaskBoundaryPoint(
		APlayerController* PlayerController,
		const FVector& TraceOrigin,
		const FVector2D& PlayerScreenPosition,
		float WorldYawDegrees,
		float VisibleDistance,
		const FIntPoint& InViewportSize)
	{
		FMaskBoundaryPoint BoundaryPoint;
		if (!PlayerController || InViewportSize.X <= 0 || InViewportSize.Y <= 0)
		{
			return BoundaryPoint;
		}

		const float WorldYawRadians = FMath::DegreesToRadians(WorldYawDegrees);
		const FVector WorldDirection(FMath::Cos(WorldYawRadians), FMath::Sin(WorldYawRadians), 0.0f);
		const FVector BoundaryWorldPoint = TraceOrigin + WorldDirection * FMath::Max(0.0f, VisibleDistance);

		FVector2D BoundaryScreenPosition;
		if (!PlayerController->ProjectWorldLocationToScreen(BoundaryWorldPoint, BoundaryScreenPosition, true))
		{
			return BoundaryPoint;
		}

		FVector2D ScreenDirection = BoundaryScreenPosition - PlayerScreenPosition;
		if (!ScreenDirection.Normalize())
		{
			return BoundaryPoint;
		}

		const FVector2D BorderPoint = FindViewportRayBorderPoint(PlayerScreenPosition, ScreenDirection, InViewportSize);
		const float BoundaryDistance = FVector2D::Distance(PlayerScreenPosition, BoundaryScreenPosition);
		const float BorderDistance = FVector2D::Distance(PlayerScreenPosition, BorderPoint);
		const float VisibleScreenDistance = FMath::Min(BoundaryDistance, BorderDistance);
		const float FeatherScreenDistance = FMath::Min(BorderDistance, VisibleScreenDistance + MaskFeatherPixels);

		BoundaryPoint.Visible = PlayerScreenPosition + ScreenDirection * VisibleScreenDistance;
		BoundaryPoint.Feather = PlayerScreenPosition + ScreenDirection * FeatherScreenDistance;
		BoundaryPoint.Border = BorderPoint;
		BoundaryPoint.bValid = true;
		return BoundaryPoint;
	}

	void AddMaskQuad(
		const FVector2D& A,
		const FVector2D& B,
		const FVector2D& C,
		const FVector2D& D,
		const FColor& ColorA,
		const FColor& ColorB,
		const FColor& ColorC,
		const FColor& ColorD,
		TArray<FTunaSweeperVisionMaskVertex>& OutVertices,
		TArray<SlateIndex>& OutIndices)
	{
		const int32 BaseIndex = OutVertices.Num();
		const int64 MaxSlateIndex = static_cast<int64>(TNumericLimits<SlateIndex>::Max());
		if (static_cast<int64>(BaseIndex) > MaxSlateIndex - 4)
		{
			return;
		}

		OutVertices.Add({ A, ColorA });
		OutVertices.Add({ B, ColorB });
		OutVertices.Add({ C, ColorC });
		OutVertices.Add({ D, ColorD });

		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 0));
		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 1));
		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 2));
		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 0));
		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 2));
		OutIndices.Add(static_cast<SlateIndex>(BaseIndex + 3));
	}

	int32 BuildHiddenMaskMeshFromView(
		APlayerController* PlayerController,
		const FTunaSweeperPlayerVisionSettings& VisionSettings,
		const TArray<float>& RayDistances,
		const FVector& TraceOrigin,
		float FacingYawDegrees,
		float RayAngleStepDegrees,
		const FIntPoint& InViewportSize,
		TArray<FTunaSweeperVisionMaskVertex>& OutVertices,
		TArray<SlateIndex>& OutIndices)
	{
		OutVertices.Reset();
		OutIndices.Reset();
		if (!PlayerController || RayDistances.IsEmpty() || InViewportSize.X <= 0 || InViewportSize.Y <= 0)
		{
			return 0;
		}

		FVector2D PlayerScreenPosition;
		if (!PlayerController->ProjectWorldLocationToScreen(TraceOrigin, PlayerScreenPosition, true))
		{
			return 0;
		}

		const float AlwaysVisibleRadius = FMath::Max(0.0f, VisionSettings.AlwaysVisibleRadius);
		const float HalfFieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f) * 0.5f;
		const int32 BoundarySegmentCount = FMath::Clamp(
			FMath::CeilToInt(360.0f / FMath::Max(1.0f, RayAngleStepDegrees)),
			72,
			360);
		const float BoundaryAngleStepDegrees = 360.0f / static_cast<float>(BoundarySegmentCount);
		const uint8 HiddenAlpha = static_cast<uint8>(FMath::Clamp(VisionSettings.HiddenMaskAlpha, 0, 255));
		const FColor MaskColor(0, 0, 0, HiddenAlpha);
		const FColor TransparentColor(0, 0, 0, 0);

		TArray<FMaskBoundaryPoint> BoundaryPoints;
		BoundaryPoints.SetNum(BoundarySegmentCount);
		for (int32 BoundaryIndex = 0; BoundaryIndex < BoundarySegmentCount; ++BoundaryIndex)
		{
			const float RelativeAngleDegrees =
				-180.0f + static_cast<float>(BoundaryIndex) * BoundaryAngleStepDegrees;
			const float VisibleDistance = SampleVisibleDistanceForRelativeAngle(
				RelativeAngleDegrees,
				HalfFieldOfViewDegrees,
				RayAngleStepDegrees,
				AlwaysVisibleRadius,
				RayDistances);
			BoundaryPoints[BoundaryIndex] = BuildMaskBoundaryPoint(
				PlayerController,
				TraceOrigin,
				PlayerScreenPosition,
				FacingYawDegrees + RelativeAngleDegrees,
				VisibleDistance,
				InViewportSize);
		}

		OutVertices.Reserve(BoundarySegmentCount * 8);
		OutIndices.Reserve(BoundarySegmentCount * 12);
		for (int32 BoundaryIndex = 0; BoundaryIndex < BoundarySegmentCount; ++BoundaryIndex)
		{
			const FMaskBoundaryPoint& Current = BoundaryPoints[BoundaryIndex];
			const FMaskBoundaryPoint& Next = BoundaryPoints[(BoundaryIndex + 1) % BoundarySegmentCount];
			if (!Current.bValid || !Next.bValid)
			{
				continue;
			}

			AddMaskQuad(
				Current.Visible,
				Next.Visible,
				Next.Feather,
				Current.Feather,
				TransparentColor,
				TransparentColor,
				MaskColor,
				MaskColor,
				OutVertices,
				OutIndices);
			AddMaskQuad(
				Current.Feather,
				Next.Feather,
				Next.Border,
				Current.Border,
				MaskColor,
				MaskColor,
				MaskColor,
				MaskColor,
				OutVertices,
				OutIndices);
		}

		return OutIndices.Num() / 3;
	}
}

UTunaSweeperPlayerVisionComponent::UTunaSweeperPlayerVisionComponent()
{
	bAutoActivate = true;
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UTunaSweeperPlayerVisionComponent::BeginPlay()
{
	Super::BeginPlay();
	if (FMath::IsNearlyEqual(VisionSettings.SightDistance, TunaSweeperVision::CompactDefaultSightDistance) ||
		FMath::IsNearlyEqual(VisionSettings.SightDistance, TunaSweeperVision::LegacyDefaultSightDistance) ||
		FMath::IsNearlyEqual(VisionSettings.SightDistance, TunaSweeperVision::RecentDefaultSightDistance))
	{
		VisionSettings.SightDistance = TunaSweeperVision::FramedDefaultSightDistance;
	}
	if (FMath::IsNearlyEqual(VisionSettings.AlwaysVisibleRadius, TunaSweeperVision::LegacyDefaultAlwaysVisibleRadius))
	{
		VisionSettings.AlwaysVisibleRadius = TunaSweeperVision::FramedDefaultAlwaysVisibleRadius;
	}
	if (VisionSettings.MaskDownsampleFactor == TunaSweeperVision::LegacyMaskDownsampleFactor)
	{
		VisionSettings.MaskDownsampleFactor = TunaSweeperVision::FramedMaskDownsampleFactor;
	}
	if (VisionSettings.HiddenMaskAlpha == TunaSweeperVision::LegacyHiddenMaskAlpha)
	{
		VisionSettings.HiddenMaskAlpha = TunaSweeperVision::FramedHiddenMaskAlpha;
	}
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

	if (!IsVisionWorldEnabled())
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
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskVisible(false);
		}
		if (IsVisionDebugEnabled())
		{
			const APawn* OwnerPawn = Cast<APawn>(GetOwner());
			const AController* OwnerController = OwnerPawn ? OwnerPawn->GetController() : nullptr;
			const UWorld* World = GetWorld();
			const APlayerController* FirstPlayerController = World ? World->GetFirstPlayerController() : nullptr;
			const APawn* FirstPlayerPawn = FirstPlayerController ? FirstPlayerController->GetPawn() : nullptr;
			ShowVisionDebugStatus(FString::Printf(
				TEXT("[Vision] v2 no local gameplay pawn | owner=%s ownerClass=%s controller=%s controllerClass=%s firstPC=%s firstPawn=%s"),
				GetOwner() ? *GetOwner()->GetName() : TEXT("None"),
				GetOwner() ? *GetOwner()->GetClass()->GetName() : TEXT("None"),
				OwnerController ? *OwnerController->GetName() : TEXT("None"),
				OwnerController ? *OwnerController->GetClass()->GetName() : TEXT("None"),
				FirstPlayerController ? *FirstPlayerController->GetName() : TEXT("None"),
				FirstPlayerPawn ? *FirstPlayerPawn->GetName() : TEXT("None")),
				FColor::Red);
		}
		return;
	}

	if (IsVisionDebugEnabled())
	{
		DrawVisionDebugRangeWires();
		DrawVisionDebugInsideFieldOfView();
	}

	if (!ShouldUpdateVision())
	{
		if (IsVisionDebugEnabled())
		{
			ShowVisionDebugStatus(FString::Printf(
				TEXT("[Vision] v1 update skipped | active %s | overlay %s | debug %s"),
				IsActive() ? TEXT("true") : TEXT("false"),
				bRenderVisionOverlay ? TEXT("true") : TEXT("false"),
				IsVisionDebugEnabled() ? TEXT("true") : TEXT("false")),
				FColor::Orange);
		}
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskVisible(false);
		}
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
	if (!IsVisionWorldEnabled())
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
		if (IsVisionDebugEnabled())
		{
			ShowVisionDebugStatus(TEXT("[Vision] force refresh skipped | no player controller"), FColor::Red);
		}
		return;
	}

	int32 ViewportX = 0;
	int32 ViewportY = 0;
	PlayerController->GetViewportSize(ViewportX, ViewportY);
	if (ViewportX <= 0 || ViewportY <= 0)
	{
		if (IsVisionDebugEnabled())
		{
			ShowVisionDebugStatus(TEXT("[Vision] Debug enabled, but viewport size is zero."), FColor::Red);
		}
		return;
	}

	ViewportSize = FIntPoint(ViewportX, ViewportY);
	ConfigureOverlayWidgetForViewport(ViewportSize);

	if (bRenderVisionOverlay)
	{
		EnsureOverlayWidget(PlayerController);
	}

	TArray<float> RayDistances;
	FVector TraceOrigin = FVector::ZeroVector;
	float FacingYawDegrees = 0.0f;
	float RayAngleStepDegrees = 1.0f;
	if (!BuildVisibleRayDistances(RayDistances, TraceOrigin, FacingYawDegrees, RayAngleStepDegrees))
	{
		if (IsVisionDebugEnabled())
		{
			ShowVisionDebugStatus(TEXT("[Vision] v3 Debug enabled, but ray distance generation failed."), FColor::Orange);
		}
		if (VisionMaskWidget)
		{
			VisionMaskWidget->SetMaskVisible(false);
		}
		return;
	}

	TArray<FTunaSweeperVisionMaskVertex> MaskVertices;
	TArray<SlateIndex> MaskIndices;
	const int32 MaskTriangleCount = TunaSweeperVision::BuildHiddenMaskMeshFromView(
		PlayerController,
		VisionSettings,
		RayDistances,
		TraceOrigin,
		FacingYawDegrees,
		RayAngleStepDegrees,
		ViewportSize,
		MaskVertices,
		MaskIndices);
	if (IsVisionDebugEnabled())
	{
		ShowVisionDebugStatus(FString::Printf(
			TEXT("[Vision] v4 Debug active | viewport %dx%d | alpha %d | near %.0fcm | sight %.0fcm | rays %d | step %.2f | mask tris %d | overlay %s z %d"),
			ViewportSize.X,
			ViewportSize.Y,
			VisionSettings.HiddenMaskAlpha,
			VisionSettings.AlwaysVisibleRadius,
			VisionSettings.SightDistance,
			RayDistances.Num(),
			RayAngleStepDegrees,
			MaskTriangleCount,
			VisionMaskWidget ? TEXT("yes") : TEXT("no"),
			FMath::Max(OverlayZOrder, 1)));
	}
	if (MaskTriangleCount <= 0)
	{
		if (IsVisionDebugEnabled())
		{
			ShowVisionDebugStatus(TEXT("[Vision] No mask triangles generated; hiding mask fail-open."), FColor::Orange);
		}
		if (VisionMaskWidget)
		{
			VisionMaskWidget->ClearMaskMesh();
			VisionMaskWidget->SetMaskVisible(false);
		}
		return;
	}

	if (bRenderVisionOverlay && VisionMaskWidget)
	{
		VisionMaskWidget->SetMaskMesh(MoveTemp(MaskVertices), MoveTemp(MaskIndices));
		VisionMaskWidget->SetMaskVisible(true);
	}
}

APlayerController* UTunaSweeperPlayerVisionComponent::ResolveLocalPlayerController() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		return nullptr;
	}

	auto IsLocalGameplayControllerForOwner = [OwnerPawn](const APlayerController* CandidatePlayerController)
	{
		return CandidatePlayerController &&
			CandidatePlayerController->IsLocalController() &&
			CandidatePlayerController->GetPawn() == OwnerPawn;
	};

	AController* OwnerController = OwnerPawn->GetController();
	if (APlayerController* OwnerPlayerController = Cast<APlayerController>(OwnerController))
	{
		return IsLocalGameplayControllerForOwner(OwnerPlayerController) ? OwnerPlayerController : nullptr;
	}

	const UWorld* World = GetWorld();
	if (World)
	{
		for (FConstPlayerControllerIterator ControllerIt = World->GetPlayerControllerIterator(); ControllerIt; ++ControllerIt)
		{
			APlayerController* CandidatePlayerController = ControllerIt->Get();
			if (IsLocalGameplayControllerForOwner(CandidatePlayerController))
			{
				return CandidatePlayerController;
			}
		}
	}

	return nullptr;
}

bool UTunaSweeperPlayerVisionComponent::IsVisionWorldEnabled() const
{
	const UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return false;
	}

	const FString MapName = World->GetMapName();
	return MapName.EndsWith(TEXT("BunkerMap")) || MapName.EndsWith(TEXT("RaidMap"));
}

bool UTunaSweeperPlayerVisionComponent::ShouldUpdateVision() const
{
	return bRenderVisionOverlay || IsVisionDebugEnabled();
}

bool UTunaSweeperPlayerVisionComponent::IsVisionDebugEnabled() const
{
#if !WITH_EDITOR
	return false;
#else
	if (bEnableDebugOverride)
	{
		return true;
	}

	const UWorld* World = GetWorld();
	const UTunaSweeperGameInstance* TunaGameInstance = World ? World->GetGameInstance<UTunaSweeperGameInstance>() : nullptr;
	return TunaGameInstance && TunaGameInstance->IsVisionDebugEnabled();
#endif
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

	if (VisionMaskWidget || !PlayerController)
	{
		return;
	}

	VisionMaskWidget = CreateWidget<UTunaSweeperVisionMaskWidget>(
		PlayerController,
		UTunaSweeperVisionMaskWidget::StaticClass());
	if (VisionMaskWidget)
	{
		VisionMaskWidget->SetMaskTexture(MaskTexture);
		VisionMaskWidget->SetMaskVisible(false);
		VisionMaskWidget->AddToViewport(FMath::Max(OverlayZOrder, 1));
		ConfigureOverlayWidgetForViewport(ViewportSize);
	}
}

void UTunaSweeperPlayerVisionComponent::ConfigureOverlayWidgetForViewport(const FIntPoint& InViewportSize)
{
	if (!VisionMaskWidget || InViewportSize.X <= 0 || InViewportSize.Y <= 0)
	{
		return;
	}

	VisionMaskWidget->SetAnchorsInViewport(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
	VisionMaskWidget->SetAlignmentInViewport(FVector2D::ZeroVector);
	VisionMaskWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
	VisionMaskWidget->SetDesiredSizeInViewport(FVector2D(
		static_cast<float>(InViewportSize.X),
		static_cast<float>(InViewportSize.Y)));
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
	TArray<uint8> InitialPixels;
	InitialPixels.SetNumZeroed(MaskSize.X * MaskSize.Y * 4);
	MaskTexture = UTexture2D::CreateTransient(MaskSize.X, MaskSize.Y, PF_B8G8R8A8, NAME_None, InitialPixels);
	if (!MaskTexture)
	{
		return false;
	}

	MaskTexture->CompressionSettings = TC_EditorIcon;
#if WITH_EDITORONLY_DATA
	MaskTexture->MipGenSettings = TMGS_NoMipmaps;
#endif
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

bool UTunaSweeperPlayerVisionComponent::BuildVisibleRayDistances(
	TArray<float>& OutRayDistances,
	FVector& OutTraceOrigin,
	float& OutFacingYawDegrees,
	float& OutRayAngleStepDegrees)
{
	const AActor* OwnerActor = GetOwner();
	const UWorld* World = GetWorld();
	if (!OwnerActor || !World)
	{
		return false;
	}

	const float FieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f);
	const float HalfFieldOfViewDegrees = FieldOfViewDegrees * 0.5f;
	const float RequestedAngleStepDegrees = FMath::Clamp(VisionSettings.RayAngleStepDegrees, 0.1f, 45.0f);
	const int32 RaySegmentCount = FMath::Max(1, FMath::CeilToInt(FieldOfViewDegrees / RequestedAngleStepDegrees));
	const int32 RayCount = RaySegmentCount + 1;
	const float ActualAngleStepDegrees = FieldOfViewDegrees / static_cast<float>(RaySegmentCount);
	const float TraceDistance = FMath::Max(VisionSettings.AlwaysVisibleRadius, VisionSettings.SightDistance);
	const FVector TraceOrigin = OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, VisionSettings.TraceHeight));
	FVector FacingDirection = OwnerActor->GetActorForwardVector();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.Normalize())
	{
		FacingDirection = FVector::ForwardVector;
	}

	const float FacingYaw = FacingDirection.Rotation().Yaw;

	OutTraceOrigin = TraceOrigin;
	OutFacingYawDegrees = FacingYaw;
	OutRayAngleStepDegrees = ActualAngleStepDegrees;
	OutRayDistances.Reset(RayCount);
	for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
	{
		const float RelativeAngleDegrees =
			-HalfFieldOfViewDegrees + static_cast<float>(RayIndex) * ActualAngleStepDegrees;
		FVector Direction = FacingDirection.RotateAngleAxis(RelativeAngleDegrees, FVector::UpVector);
		Direction.Z = 0.0f;
		Direction.Normalize();

		FHitResult Hit;
		const float VisibleDistance = TraceVisibleDistance(TraceOrigin, Direction, true, Hit);

		OutRayDistances.Add(VisibleDistance);
	}

	return OutRayDistances.Num() == RayCount;
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

int32 UTunaSweeperPlayerVisionComponent::RasterizeVisionMaskFromView(
	APlayerController* PlayerController,
	const TArray<float>& RayDistances,
	const FVector& TraceOrigin,
	float FacingYawDegrees,
	float RayAngleStepDegrees)
{
	const int32 Width = MaskSize.X;
	const int32 Height = MaskSize.Y;
	const uint8 HiddenAlpha = static_cast<uint8>(FMath::Clamp(VisionSettings.HiddenMaskAlpha, 0, 255));
	VisibilityMask.Init(HiddenAlpha, Width * Height);

	if (!PlayerController || RayDistances.IsEmpty() || Width <= 0 || Height <= 0 ||
		ViewportSize.X <= 0 || ViewportSize.Y <= 0)
	{
		return 0;
	}

	const int32 RayCount = RayDistances.Num();
	const float HalfFieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f) * 0.5f;
	const float SafeRayAngleStepDegrees = FMath::Max(0.1f, RayAngleStepDegrees);
	const float ViewScaleX = static_cast<float>(ViewportSize.X) / static_cast<float>(Width);
	const float ViewScaleY = static_cast<float>(ViewportSize.Y) / static_cast<float>(Height);
	const float EdgePadding = FMath::Max(
		8.0f,
		FMath::Max(VisionSettings.SightDistance, VisionSettings.AlwaysVisibleRadius) /
			static_cast<float>(FMath::Max(Width, Height)));
	int32 VisiblePixelCount = 0;

	for (int32 Y = 0; Y < Height; ++Y)
	{
		uint8* Row = VisibilityMask.GetData() + Y * Width;
		const float ScreenY = (static_cast<float>(Y) + 0.5f) * ViewScaleY;

		for (int32 X = 0; X < Width; ++X)
		{
			const float ScreenX = (static_cast<float>(X) + 0.5f) * ViewScaleX;
			FVector WorldOrigin;
			FVector WorldDirection;
			if (!PlayerController->DeprojectScreenPositionToWorld(ScreenX, ScreenY, WorldOrigin, WorldDirection) ||
				FMath::IsNearlyZero(WorldDirection.Z))
			{
				continue;
			}

			const float PlaneIntersection =
				(TraceOrigin.Z - WorldOrigin.Z) / WorldDirection.Z;
			if (PlaneIntersection < 0.0f)
			{
				continue;
			}

			const FVector WorldPoint = WorldOrigin + WorldDirection * PlaneIntersection;
			FVector Delta = WorldPoint - TraceOrigin;
			Delta.Z = 0.0f;
			const float Distance = Delta.Size();
			if (Distance <= KINDA_SMALL_NUMBER)
			{
				Row[X] = 0;
				++VisiblePixelCount;
				continue;
			}

			const float AngleDegrees = FRotator::NormalizeAxis(Delta.Rotation().Yaw);
			const float RelativeAngleDegrees = FMath::FindDeltaAngleDegrees(FacingYawDegrees, AngleDegrees);
			if (Distance > VisionSettings.AlwaysVisibleRadius + EdgePadding &&
				FMath::Abs(RelativeAngleDegrees) > HalfFieldOfViewDegrees)
			{
				continue;
			}

			const float RayPosition = (RelativeAngleDegrees + HalfFieldOfViewDegrees) / SafeRayAngleStepDegrees;
			const int32 RayIndexA = FMath::Clamp(FMath::FloorToInt(RayPosition), 0, RayCount - 1);
			const int32 RayIndexB = FMath::Clamp(RayIndexA + 1, 0, RayCount - 1);
			const float RayAlpha = FMath::Frac(RayPosition);
			const float VisibleDistance = FMath::Lerp(RayDistances[RayIndexA], RayDistances[RayIndexB], RayAlpha);

			if (Distance <= VisionSettings.AlwaysVisibleRadius + EdgePadding ||
				Distance <= VisibleDistance + EdgePadding)
			{
				Row[X] = 0;
				++VisiblePixelCount;
			}
		}
	}

	return VisiblePixelCount;
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
	float TraceDistance,
	const FHitResult& Hit) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float MaxDebugDistance = FMath::Max(0.0f, TraceDistance);
	const bool bHitInRange = Hit.bBlockingHit && Hit.Distance > 0.0f && Hit.Distance < MaxDebugDistance;
	const float GreenDistance = bHitInRange ? FMath::Clamp(Hit.Distance, 0.0f, MaxDebugDistance) : MaxDebugDistance;
	const FVector DebugLift(0.0f, 0.0f, 22.0f);
	const FVector DebugStart = TraceOrigin + DebugLift;
	const float OneFrameLifeTime = 0.0f;

	DrawDebugLine(
		World,
		DebugStart,
		DebugStart + Direction * GreenDistance,
		FColor::Green,
		false,
		OneFrameLifeTime,
		1,
		3.0f);

	if (bHitInRange && GreenDistance < MaxDebugDistance)
	{
		DrawDebugLine(
			World,
			DebugStart + Direction * GreenDistance,
			DebugStart + Direction * MaxDebugDistance,
			FColor::Red,
			false,
			OneFrameLifeTime,
			1,
			2.0f);
	}

	if (Hit.bBlockingHit)
	{
		DrawDebugPoint(World, Hit.ImpactPoint + DebugLift, 9.0f, FColor::Red, false, OneFrameLifeTime, 1);
	}
}

void UTunaSweeperPlayerVisionComponent::DrawVisionDebugInsideFieldOfView() const
{
	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return;
	}

	FVector FacingDirection = OwnerActor->GetActorForwardVector();
	FacingDirection.Z = 0.0f;
	if (!FacingDirection.Normalize())
	{
		FacingDirection = FVector::ForwardVector;
	}

	const float FieldOfViewDegrees = FMath::Clamp(VisionSettings.FieldOfViewDegrees, 1.0f, 360.0f);
	const float HalfFieldOfViewDegrees = FieldOfViewDegrees * 0.5f;
	const float RequestedAngleStepDegrees = FMath::Clamp(VisionSettings.RayAngleStepDegrees, 0.1f, 45.0f);
	const int32 RaySegmentCount = FMath::Max(1, FMath::CeilToInt(FieldOfViewDegrees / RequestedAngleStepDegrees));
	const int32 RayCount = RaySegmentCount + 1;
	const float ActualAngleStepDegrees = FieldOfViewDegrees / static_cast<float>(RaySegmentCount);
	const float TraceDistance = FMath::Max(VisionSettings.AlwaysVisibleRadius, VisionSettings.SightDistance);
	const FVector TraceOrigin =
		OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, VisionSettings.TraceHeight));

	for (int32 RayIndex = 0; RayIndex < RayCount; ++RayIndex)
	{
		const float RelativeAngleDegrees =
			-HalfFieldOfViewDegrees + static_cast<float>(RayIndex) * ActualAngleStepDegrees;
		FVector Direction = FacingDirection.RotateAngleAxis(RelativeAngleDegrees, FVector::UpVector);
		Direction.Z = 0.0f;
		if (!Direction.Normalize())
		{
			continue;
		}

		FHitResult Hit;
		TraceVisibleDistance(TraceOrigin, Direction, true, Hit);
		DrawVisionDebug(TraceOrigin, Direction, TraceDistance, Hit);
	}

	DrawVisionDebugBounds(TraceOrigin, FacingDirection);
}

void UTunaSweeperPlayerVisionComponent::DrawVisionDebugRangeWires() const
{
	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return;
	}

	const FVector Center =
		OwnerActor->GetActorLocation() + FVector(0.0f, 0.0f, FMath::Max(0.0f, VisionSettings.TraceHeight));
	const float MinRadius = FMath::Max(0.0f, VisionSettings.AlwaysVisibleRadius);
	const float MaxRadius = FMath::Max(MinRadius, VisionSettings.SightDistance);
	const float OneFrameLifeTime = 0.0f;
	constexpr int32 SegmentCount = 60;

	auto DrawRangeWire = [World, Center, OneFrameLifeTime](float Radius, FColor Color, float Thickness)
	{
		if (Radius <= 0.0f)
		{
			return;
		}

		FVector PreviousPoint = Center + FVector(Radius, 0.0f, 0.0f);
		for (int32 SegmentIndex = 1; SegmentIndex <= SegmentCount; ++SegmentIndex)
		{
			const float AngleRadians = (static_cast<float>(SegmentIndex) / static_cast<float>(SegmentCount)) * 2.0f * PI;
			const FVector CurrentPoint = Center + FVector(
				FMath::Cos(AngleRadians) * Radius,
				FMath::Sin(AngleRadians) * Radius,
				0.0f);
			DrawDebugLine(World, PreviousPoint, CurrentPoint, Color, false, OneFrameLifeTime, 1, Thickness);
			PreviousPoint = CurrentPoint;
		}
	};

	DrawRangeWire(MinRadius, FColor::Cyan, 2.5f);
	DrawRangeWire(MaxRadius, FColor::Yellow, 2.0f);
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
	const float OneFrameLifeTime = 0.0f;
	const FVector LeftBoundary = FacingDirection.RotateAngleAxis(-HalfFieldOfViewDegrees, FVector::UpVector).GetSafeNormal();
	const FVector RightBoundary = FacingDirection.RotateAngleAxis(HalfFieldOfViewDegrees, FVector::UpVector).GetSafeNormal();

	DrawDebugSphere(World, TraceOrigin, 18.0f, 12, FColor::Yellow, false, OneFrameLifeTime, 1, 1.5f);
	DrawDebugCircle(
		World,
		TraceOrigin,
		AlwaysVisibleRadius,
		64,
		FColor::Cyan,
		false,
		OneFrameLifeTime,
		1,
		1.5f,
		FVector::ForwardVector,
		FVector::RightVector,
		false);
	DrawDebugLine(World, TraceOrigin, TraceOrigin + LeftBoundary * SightDistance, FColor::Green, false, OneFrameLifeTime, 1, 2.0f);
	DrawDebugLine(World, TraceOrigin, TraceOrigin + RightBoundary * SightDistance, FColor::Green, false, OneFrameLifeTime, 1, 2.0f);
}

void UTunaSweeperPlayerVisionComponent::ShowVisionDebugStatus(const FString& StatusText, FColor TextColor) const
{
	const UWorld* World = GetWorld();
	const double CurrentTimeSeconds = World ? World->GetTimeSeconds() : FPlatformTime::Seconds();
	if (CurrentTimeSeconds - LastDebugStatusTimeSeconds < 0.5)
	{
		return;
	}

	LastDebugStatusTimeSeconds = CurrentTimeSeconds;
	UE_LOG(LogTunaSweeperPlayerVision, Log, TEXT("%s"), *StatusText);
	if (bShowDebugStatusMessage && GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 0.75f, TextColor, StatusText);
	}
}
