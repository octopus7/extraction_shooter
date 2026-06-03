#include "Interaction/TunaSweeperSandbagCoverActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "TunaSweeperCollisionChannels.h"

namespace
{
	const TCHAR* DefaultSandbagMaterialPath = TEXT("/Game/Interaction/M_SandbagCover_Burlap.M_SandbagCover_Burlap");
	const TCHAR* DefaultSandbagOutlineMaterialPath = TEXT("/Game/Interaction/M_SandbagCover_OverlayOutline.M_SandbagCover_OverlayOutline");
	const TCHAR* DefaultSandbagStaticMeshPath = TEXT("/Game/Interaction/SM_Sandbag_LowPoly.SM_Sandbag_LowPoly");
	const TCHAR* FallbackVertexColorMaterialPath = TEXT("/Game/Prototype/M_Voxel_VertexColor.M_Voxel_VertexColor");

	constexpr int32 SandbagLayerCount = 4;
	constexpr int32 SandbagDepthCount = 2;
	constexpr int32 MaxSandbagMeshComponentCount = 36;
	constexpr int32 SandbagOutlineStencilValue = 3;
	const FVector BaseSandbagMeshExtent(21.0f, 28.0f, 9.0f);

	FVector MakeSafeBoxExtent(const FVector& InBoxExtent)
	{
		return FVector(
			FMath::Max(1.0f, InBoxExtent.X),
			FMath::Max(1.0f, InBoxExtent.Y),
			FMath::Max(1.0f, InBoxExtent.Z));
	}

	FVector MakeScaleFromExtents(const FVector& DesiredExtent, const FVector& BaseExtent)
	{
		return FVector(
			DesiredExtent.X / FMath::Max(1.0f, BaseExtent.X),
			DesiredExtent.Y / FMath::Max(1.0f, BaseExtent.Y),
			DesiredExtent.Z / FMath::Max(1.0f, BaseExtent.Z));
	}

	FRotator LerpRotatorComponentWise(const FRotator& A, const FRotator& B, float Alpha)
	{
		return FRotator(
			FMath::Lerp(A.Pitch, B.Pitch, Alpha),
			FMath::Lerp(A.Yaw, B.Yaw, Alpha),
			FMath::Lerp(A.Roll, B.Roll, Alpha));
	}
}

ATunaSweeperSandbagCoverActor::ATunaSweeperSandbagCoverActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.0f;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	BlockingCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BlockingCollision"));
	BlockingCollision->SetupAttachment(RootComponent);
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(true);

	SandbagMeshComponents.Reserve(MaxSandbagMeshComponentCount);
	for (int32 ComponentIndex = 0; ComponentIndex < MaxSandbagMeshComponentCount; ++ComponentIndex)
	{
		const FName ComponentName(*FString::Printf(TEXT("SandbagMesh_%02d"), ComponentIndex));
		UStaticMeshComponent* SandbagMesh = CreateDefaultSubobject<UStaticMeshComponent>(ComponentName);
		SandbagMesh->SetupAttachment(RootComponent);
		SandbagMesh->SetMobility(EComponentMobility::Movable);
		SandbagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		SandbagMesh->SetGenerateOverlapEvents(false);
		SandbagMesh->SetCanEverAffectNavigation(false);
		SandbagMesh->SetRenderCustomDepth(false);
		SandbagMesh->SetCustomDepthStencilValue(SandbagOutlineStencilValue);
		SandbagMesh->SetOverlayMaterial(nullptr);
		SandbagMesh->SetOverlayMaterialMaxDrawDistance(0.0f);
		SandbagMeshComponents.Add(SandbagMesh);
	}

	VisualMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultSandbagMaterialPath));
	OutlineMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(DefaultSandbagOutlineMaterialPath));
	SandbagStaticMesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(DefaultSandbagStaticMeshPath));

	ApplyCollisionDefaults();
}

void ATunaSweeperSandbagCoverActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	BoxExtent = MakeSafeBoxExtent(BoxExtent);
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;
	PassthroughRadius = FMath::Max(0.0f, PassthroughRadius);
	PassthroughVerticalTolerance = FMath::Max(0.0f, PassthroughVerticalTolerance);
	OutlineThickness = FMath::Max(0.5f, OutlineThickness);
	CollapseDurationSeconds = FMath::Max(0.05f, CollapseDurationSeconds);
	CollapseHoldSeconds = FMath::Max(0.0f, CollapseHoldSeconds);
	CollapseScatterDistance = FMath::Max(0.0f, CollapseScatterDistance);

	ResetCollapseState();
	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
}

void ATunaSweeperSandbagCoverActor::BeginPlay()
{
	Super::BeginPlay();

	BoxExtent = MakeSafeBoxExtent(BoxExtent);
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	CurrentHealth = MaxHealth;

	ResetCollapseState();
	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
	UpdatePassthroughOutline();
}

void ATunaSweeperSandbagCoverActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCoverDestroyed)
	{
		UpdateCollapse(DeltaSeconds);
		return;
	}

	UpdatePassthroughOutline();
}

float ATunaSweeperSandbagCoverActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	if (bCoverDestroyed || DamageAmount <= 0.0f)
	{
		return 0.0f;
	}

	const float AppliedDamage = FMath::Min(CurrentHealth, DamageAmount);
	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	UpdateDamageVisual();

	if (CurrentHealth <= 0.0f)
	{
		DestroyCover();
	}

	return AppliedDamage;
}

void ATunaSweeperSandbagCoverActor::ConfigureCoverDefaults(
	FName InCoverId,
	const FVector& InBoxExtent,
	float InMaxHealth,
	float InPassthroughRadius)
{
	CoverId = InCoverId;
	BoxExtent = MakeSafeBoxExtent(InBoxExtent);
	MaxHealth = FMath::Max(1.0f, InMaxHealth);
	CurrentHealth = MaxHealth;
	PassthroughRadius = FMath::Max(0.0f, InPassthroughRadius);
	PassthroughVerticalTolerance = FMath::Max(0.0f, BoxExtent.Z);
	OutlineThickness = FMath::Max(0.5f, BoxExtent.Z / 15.0f);
	CollapseScatterDistance = FMath::Max(0.0f, BoxExtent.X * 1.8f);

	ResetCollapseState();
	ApplyCollisionDefaults();
	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
}

void ATunaSweeperSandbagCoverActor::ConfigureCoverVisualDefaults(
	TSoftObjectPtr<UMaterialInterface> InVisualMaterial,
	TSoftObjectPtr<UMaterialInterface> InOutlineMaterial)
{
	if (!InVisualMaterial.IsNull())
	{
		VisualMaterial = InVisualMaterial;
	}
	if (!InOutlineMaterial.IsNull())
	{
		OutlineMaterial = InOutlineMaterial;
	}

	ApplyMaterials();
	UpdateDamageVisual();
}

void ATunaSweeperSandbagCoverActor::ConfigureCoverMeshDefaults(TSoftObjectPtr<UStaticMesh> InSandbagMesh)
{
	if (!InSandbagMesh.IsNull())
	{
		SandbagStaticMesh = InSandbagMesh;
	}

	RebuildMeshes();
	ApplyMaterials();
	UpdateDamageVisual();
}

bool ATunaSweeperSandbagCoverActor::ShouldAllowPlayerProjectilePassthrough(APawn* InstigatorPawn) const
{
	if (bCoverDestroyed || !InstigatorPawn || !InstigatorPawn->IsPlayerControlled())
	{
		return false;
	}

	const FVector LocalPawnLocation = GetActorTransform().InverseTransformPosition(InstigatorPawn->GetActorLocation());
	const float OutsideX = FMath::Max(FMath::Abs(LocalPawnLocation.X) - BoxExtent.X, 0.0f);
	const float OutsideY = FMath::Max(FMath::Abs(LocalPawnLocation.Y) - BoxExtent.Y, 0.0f);
	const float HorizontalDistanceSquared = OutsideX * OutsideX + OutsideY * OutsideY;
	const float SafePassthroughRadius = FMath::Max(0.0f, PassthroughRadius);
	if (HorizontalDistanceSquared > FMath::Square(SafePassthroughRadius))
	{
		return false;
	}

	const float MinZ = -PassthroughVerticalTolerance;
	const float MaxZ = BoxExtent.Z * 2.0f + PassthroughVerticalTolerance;
	return LocalPawnLocation.Z >= MinZ && LocalPawnLocation.Z <= MaxZ;
}

void ATunaSweeperSandbagCoverActor::ApplyCollisionDefaults()
{
	if (!BlockingCollision)
	{
		return;
	}

	BlockingCollision->SetRelativeLocation(FVector(0.0f, 0.0f, BoxExtent.Z));
	BlockingCollision->SetBoxExtent(BoxExtent);
	BlockingCollision->SetCollisionEnabled(bCoverDestroyed ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
	BlockingCollision->SetCollisionObjectType(ECC_WorldStatic);
	BlockingCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BlockingCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::Projectile, ECR_Block);
	BlockingCollision->SetCollisionResponseToChannel(TunaSweeperCollisionChannels::VisionOccluder, ECR_Block);
	BlockingCollision->SetGenerateOverlapEvents(false);
	BlockingCollision->CanCharacterStepUpOn = ECB_No;
	BlockingCollision->SetHiddenInGame(true);
	BlockingCollision->SetVisibility(false);
	BlockingCollision->SetCanEverAffectNavigation(!bCoverDestroyed);
}

void ATunaSweeperSandbagCoverActor::RebuildMeshes()
{
	UStaticMesh* LoadedSandbagMesh = SandbagStaticMesh.LoadSynchronous();

	int32 ComponentIndex = 0;
	const float LayerHeight = BoxExtent.Z * 2.0f / static_cast<float>(SandbagLayerCount);
	const float DepthCenterOffset = BoxExtent.X * 0.34f;
	const float DepthExtent = BoxExtent.X * 0.43f;

	for (int32 LayerIndex = 0; LayerIndex < SandbagLayerCount; ++LayerIndex)
	{
		const int32 ColumnCount = (LayerIndex % 2 == 0) ? 4 : 5;
		const float SegmentLength = BoxExtent.Y * 2.0f / static_cast<float>(ColumnCount);
		const float ZCenter = LayerHeight * (static_cast<float>(LayerIndex) + 0.5f);

		for (int32 DepthIndex = 0; DepthIndex < SandbagDepthCount; ++DepthIndex)
		{
			const float XCenter = DepthIndex == 0 ? -DepthCenterOffset : DepthCenterOffset;
			for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
			{
				if (!SandbagMeshComponents.IsValidIndex(ComponentIndex))
				{
					return;
				}

				UStaticMeshComponent* SandbagMesh = SandbagMeshComponents[ComponentIndex];
				++ComponentIndex;
				if (!SandbagMesh)
				{
					continue;
				}

				const float YCenter = -BoxExtent.Y + SegmentLength * (static_cast<float>(ColumnIndex) + 0.5f);
				const FVector BagCenter(XCenter, YCenter, ZCenter);
				const FVector BagExtent(DepthExtent, SegmentLength * 0.455f, LayerHeight * 0.42f);
				const float AlternatingYaw = static_cast<float>(((ColumnIndex + LayerIndex + DepthIndex) % 3) - 1) * 2.0f;
				const float AlternatingRoll = DepthIndex == 0 ? -1.4f : 1.4f;

				SandbagMesh->SetStaticMesh(LoadedSandbagMesh);
				SandbagMesh->SetRelativeLocation(BagCenter);
				SandbagMesh->SetRelativeRotation(FRotator(0.0f, AlternatingYaw, AlternatingRoll));
				SandbagMesh->SetRelativeScale3D(MakeScaleFromExtents(BagExtent, BaseSandbagMeshExtent));
				SandbagMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
				SandbagMesh->SetGenerateOverlapEvents(false);
				SandbagMesh->SetRenderCustomDepth(bOutlineActive && !bCoverDestroyed);
				SandbagMesh->SetCustomDepthStencilValue(SandbagOutlineStencilValue);
				SandbagMesh->SetOverlayMaterial(bOutlineActive && !bCoverDestroyed ? DynamicOutlineMaterial.Get() : nullptr);
				SandbagMesh->SetOverlayMaterialMaxDrawDistance(0.0f);
				SandbagMesh->SetHiddenInGame(false);
				SandbagMesh->SetVisibility(true, true);
			}
		}
	}

	for (; ComponentIndex < SandbagMeshComponents.Num(); ++ComponentIndex)
	{
		if (UStaticMeshComponent* SandbagMesh = SandbagMeshComponents[ComponentIndex])
		{
			SandbagMesh->SetStaticMesh(nullptr);
			SandbagMesh->SetHiddenInGame(true);
			SandbagMesh->SetVisibility(false, true);
			SandbagMesh->SetRenderCustomDepth(false);
			SandbagMesh->SetCustomDepthStencilValue(SandbagOutlineStencilValue);
			SandbagMesh->SetOverlayMaterial(nullptr);
		}
	}
}

void ATunaSweeperSandbagCoverActor::ApplyMaterials()
{
	DynamicVisualMaterial = nullptr;
	DynamicOutlineMaterial = nullptr;

	UMaterialInterface* LoadedVisualMaterial = VisualMaterial.LoadSynchronous();
	if (!LoadedVisualMaterial)
	{
		LoadedVisualMaterial = LoadObject<UMaterialInterface>(nullptr, FallbackVertexColorMaterialPath);
	}
	if (LoadedVisualMaterial)
	{
		DynamicVisualMaterial = UMaterialInstanceDynamic::Create(LoadedVisualMaterial, this);
	}

	if (UMaterialInterface* LoadedOutlineMaterial = OutlineMaterial.LoadSynchronous())
	{
		DynamicOutlineMaterial = UMaterialInstanceDynamic::Create(LoadedOutlineMaterial, this);
		if (DynamicOutlineMaterial)
		{
			DynamicOutlineMaterial->SetScalarParameterValue(TEXT("OutlineThickness"), FMath::Max(0.5f, OutlineThickness));
			DynamicOutlineMaterial->SetScalarParameterValue(TEXT("StencilMaskValue"), static_cast<float>(SandbagOutlineStencilValue));
			DynamicOutlineMaterial->SetVectorParameterValue(TEXT("OutlineColor"), FLinearColor(0.78f, 0.98f, 0.32f, 1.0f));
		}
	}

	for (UStaticMeshComponent* SandbagMesh : SandbagMeshComponents)
	{
		if (!SandbagMesh)
		{
			continue;
		}
		if (DynamicVisualMaterial)
		{
			SandbagMesh->SetMaterial(0, DynamicVisualMaterial);
		}
		SandbagMesh->SetOverlayMaterial(bOutlineActive && !bCoverDestroyed ? DynamicOutlineMaterial.Get() : nullptr);
		SandbagMesh->SetOverlayMaterialMaxDrawDistance(0.0f);
	}
}

void ATunaSweeperSandbagCoverActor::UpdateDamageVisual()
{
	if (!DynamicVisualMaterial)
	{
		return;
	}

	const float DamageAlpha = 1.0f - FMath::Clamp(CurrentHealth / FMath::Max(1.0f, MaxHealth), 0.0f, 1.0f);
	DynamicVisualMaterial->SetScalarParameterValue(TEXT("DamageAlpha"), DamageAlpha);
	DynamicVisualMaterial->SetVectorParameterValue(TEXT("DamageTint"), FLinearColor(0.46f, 0.37f, 0.26f, 1.0f));
}

void ATunaSweeperSandbagCoverActor::UpdatePassthroughOutline()
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	SetOutlineActive(ShouldAllowPlayerProjectilePassthrough(PlayerPawn));
}

void ATunaSweeperSandbagCoverActor::SetOutlineActive(bool bEnabled)
{
	bOutlineActive = bEnabled && !bCoverDestroyed;

	for (UStaticMeshComponent* SandbagMesh : SandbagMeshComponents)
	{
		if (SandbagMesh)
		{
			SandbagMesh->SetCustomDepthStencilValue(SandbagOutlineStencilValue);
			if (DynamicOutlineMaterial)
			{
				DynamicOutlineMaterial->SetScalarParameterValue(TEXT("OutlineThickness"), FMath::Max(0.5f, OutlineThickness));
				DynamicOutlineMaterial->SetScalarParameterValue(TEXT("StencilMaskValue"), static_cast<float>(SandbagOutlineStencilValue));
			}
			SandbagMesh->SetRenderCustomDepth(bOutlineActive && SandbagMesh->GetStaticMesh() != nullptr);
			SandbagMesh->SetOverlayMaterial(bOutlineActive ? DynamicOutlineMaterial.Get() : nullptr);
			SandbagMesh->SetOverlayMaterialMaxDrawDistance(0.0f);
		}
	}
}

void ATunaSweeperSandbagCoverActor::DestroyCover()
{
	BeginCollapse();
}

void ATunaSweeperSandbagCoverActor::BeginCollapse()
{
	if (bCoverDestroyed)
	{
		return;
	}

	bCoverDestroyed = true;
	bOutlineActive = false;
	CollapseElapsedSeconds = 0.0f;
	CollapseStates.Reset();
	CollapseStates.SetNum(SandbagMeshComponents.Num());

	SetActorEnableCollision(false);
	if (BlockingCollision)
	{
		BlockingCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		BlockingCollision->SetCanEverAffectNavigation(false);
	}
	for (UStaticMeshComponent* SandbagMesh : SandbagMeshComponents)
	{
		if (SandbagMesh)
		{
			SandbagMesh->SetRenderCustomDepth(false);
			SandbagMesh->SetOverlayMaterial(nullptr);
		}
	}

	const float SafeHeight = FMath::Max(1.0f, BoxExtent.Z * 2.0f);
	for (int32 Index = 0; Index < SandbagMeshComponents.Num(); ++Index)
	{
		UStaticMeshComponent* SandbagMesh = SandbagMeshComponents[Index];
		if (!SandbagMesh)
		{
			continue;
		}

		FTunaSweeperSandbagCollapseState& CollapseState = CollapseStates[Index];
		CollapseState.StartLocation = SandbagMesh->GetRelativeLocation();
		CollapseState.StartRotation = SandbagMesh->GetRelativeRotation();

		const FVector StartLocation = CollapseState.StartLocation;
		const float HeightAlpha = FMath::Clamp(StartLocation.Z / SafeHeight, 0.0f, 1.0f);
		const float AngleRadians = FMath::DegreesToRadians(FMath::Fmod(static_cast<float>(Index) * 137.507f, 360.0f));
		const FVector SpiralDirection(FMath::Cos(AngleRadians), FMath::Sin(AngleRadians), 0.0f);
		FVector SpillDirection(StartLocation.X * 1.7f, StartLocation.Y * 0.32f, 0.0f);
		SpillDirection += SpiralDirection * FMath::Max(24.0f, BoxExtent.X * 0.65f);
		SpillDirection = SpillDirection.GetSafeNormal();
		if (SpillDirection.IsNearlyZero())
		{
			SpillDirection = SpiralDirection;
		}

		const float ScatterDistance =
			FMath::Lerp(BoxExtent.X * 0.4f, CollapseScatterDistance, HeightAlpha) +
			static_cast<float>(Index % 5) * 8.0f;
		CollapseState.TargetLocation = StartLocation + SpillDirection * ScatterDistance;
		CollapseState.TargetLocation.Z = FMath::Max(8.0f, BoxExtent.Z * 0.12f) + static_cast<float>(Index % 3) * 1.5f;
		CollapseState.BurstOffset =
			SpillDirection *
			(FMath::Lerp(BoxExtent.X * 0.55f, BoxExtent.X * 1.2f, HeightAlpha) +
				static_cast<float>((Index * 7) % 5) * 2.5f);
		CollapseState.BurstLift =
			FMath::Lerp(BoxExtent.Z * 0.12f, BoxExtent.Z * 0.34f, HeightAlpha) +
			static_cast<float>(Index % 3) * 1.25f;

		const float DirectionRoll = SpillDirection.X >= 0.0f ? 82.0f : -82.0f;
		const float DirectionPitch = SpillDirection.Y >= 0.0f ? -48.0f : 48.0f;
		CollapseState.TargetRotation = FRotator(
			CollapseState.StartRotation.Pitch + DirectionPitch + static_cast<float>((Index % 3) - 1) * 8.0f,
			CollapseState.StartRotation.Yaw + FMath::RadiansToDegrees(AngleRadians) * 0.12f,
			CollapseState.StartRotation.Roll + DirectionRoll + static_cast<float>((Index % 4) - 1) * 6.0f);
		CollapseState.BurstRotation = FRotator(
			SpillDirection.Y >= 0.0f ? -14.0f : 14.0f,
			static_cast<float>((Index % 5) - 2) * 5.0f,
			SpillDirection.X >= 0.0f ? 18.0f : -18.0f);
		CollapseState.DelaySeconds = static_cast<float>(Index % 6) * 0.018f;
	}
}

void ATunaSweeperSandbagCoverActor::UpdateCollapse(float DeltaSeconds)
{
	CollapseElapsedSeconds += FMath::Max(0.0f, DeltaSeconds);

	const float SafeCollapseDuration = FMath::Max(0.05f, CollapseDurationSeconds);
	for (int32 Index = 0; Index < SandbagMeshComponents.Num() && Index < CollapseStates.Num(); ++Index)
	{
		UStaticMeshComponent* SandbagMesh = SandbagMeshComponents[Index];
		if (!SandbagMesh)
		{
			continue;
		}

		const FTunaSweeperSandbagCollapseState& CollapseState = CollapseStates[Index];
		const float LocalDuration = FMath::Max(0.05f, SafeCollapseDuration - CollapseState.DelaySeconds);
		const float RawAlpha = FMath::Clamp(
			(CollapseElapsedSeconds - CollapseState.DelaySeconds) / LocalDuration,
			0.0f,
			1.0f);
		const float MoveAlpha = RawAlpha * RawAlpha * (3.0f - 2.0f * RawAlpha);
		const float FallAlpha = FMath::Clamp(FMath::Pow(RawAlpha, 1.15f), 0.0f, 1.0f);
		const float BurstRiseAlpha = 1.0f - FMath::Square(1.0f - FMath::Clamp(RawAlpha / 0.28f, 0.0f, 1.0f));
		const float BurstFallAlpha = FMath::Square(1.0f - FMath::Clamp((RawAlpha - 0.24f) / 0.58f, 0.0f, 1.0f));
		const float BurstAlpha = BurstRiseAlpha * BurstFallAlpha;

		FVector NewLocation = FMath::Lerp(CollapseState.StartLocation, CollapseState.TargetLocation, MoveAlpha);
		NewLocation.Z = FMath::Lerp(CollapseState.StartLocation.Z, CollapseState.TargetLocation.Z, FallAlpha);
		NewLocation += CollapseState.BurstOffset * BurstAlpha;
		NewLocation.Z += CollapseState.BurstLift * BurstAlpha;
		SandbagMesh->SetRelativeLocation(NewLocation);

		FRotator NewRotation = LerpRotatorComponentWise(CollapseState.StartRotation, CollapseState.TargetRotation, MoveAlpha);
		NewRotation.Pitch += CollapseState.BurstRotation.Pitch * BurstAlpha;
		NewRotation.Yaw += CollapseState.BurstRotation.Yaw * BurstAlpha;
		NewRotation.Roll += CollapseState.BurstRotation.Roll * BurstAlpha;
		SandbagMesh->SetRelativeRotation(NewRotation);
	}

	if (CollapseElapsedSeconds >= SafeCollapseDuration + CollapseHoldSeconds)
	{
		Destroy();
	}
}

void ATunaSweeperSandbagCoverActor::ResetCollapseState()
{
	bCoverDestroyed = false;
	bOutlineActive = false;
	CollapseElapsedSeconds = 0.0f;
	CollapseStates.Reset();
	SetActorEnableCollision(true);
}
