#include "Environment/TunaSweeperShallowPuddleActor.h"

#include "Components/DecalComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperShallowPuddleActor::ATunaSweeperShallowPuddleActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("PuddleRoot")));
	Surface = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WaterSurface"));
	Surface->SetupAttachment(RootComponent);
	Surface->SetAbsolute(false, true, true);
	Surface->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Surface->SetGenerateOverlapEvents(false);
	Surface->SetCanEverAffectNavigation(false);
	Surface->SetCastShadow(false);
	Surface->SetReceivesDecals(false);
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Plane(TEXT("/Engine/BasicShapes/Plane.Plane"));
	Surface->SetStaticMesh(Plane.Object);
	WetEdge = CreateDefaultSubobject<UDecalComponent>(TEXT("WetEdge"));
	WetEdge->SetupAttachment(RootComponent);
	WetEdge->SetAbsolute(true, true, true);
	WetEdge->FadeScreenSize = 0.001f;
	static ConstructorHelpers::FObjectFinder<USoundBase> Sound(TEXT("/Game/Environment/ShallowPuddle/Audio/SW_ShallowPuddle_Footstep.SW_ShallowPuddle_Footstep"));
	WaterFootstepSound = Sound.Object;
}

void ATunaSweeperShallowPuddleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RefreshPuddle();
}

void ATunaSweeperShallowPuddleActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshPuddle();
}

FVector2D ATunaSweeperShallowPuddleActor::GetWorldHalfExtent() const
{
	const FVector Scale = GetActorScale3D().GetAbs();
	return FVector2D(FMath::Max(1.0, HalfExtentCm.X) * FMath::Max(0.001, Scale.X),
		FMath::Max(1.0, HalfExtentCm.Y) * FMath::Max(0.001, Scale.Y));
}

void ATunaSweeperShallowPuddleActor::ApplySharedParameters(UMaterialInstanceDynamic* Material) const
{
	if (!Material) return;
	const FRotator Yaw(0, GetActorRotation().Yaw, 0);
	const FVector2D Extent = GetWorldHalfExtent();
	Material->SetVectorParameterValue(TEXT("PuddleCenter"), FLinearColor(GetActorLocation()));
	Material->SetVectorParameterValue(TEXT("PuddleAxisX"), FLinearColor(Yaw.RotateVector(FVector::ForwardVector)));
	Material->SetVectorParameterValue(TEXT("PuddleAxisY"), FLinearColor(Yaw.RotateVector(FVector::RightVector)));
	Material->SetVectorParameterValue(TEXT("PuddleExtent"), FLinearColor(Extent.X, Extent.Y, 0, 0));
	Material->SetScalarParameterValue(TEXT("OutlineIrregularity"), FMath::Clamp(OutlineIrregularity, 0.0f, 1.0f));
}

void ATunaSweeperShallowPuddleActor::RefreshPuddle()
{
	const FVector2D Extent = GetWorldHalfExtent();
	const FRotator Yaw(0, GetActorRotation().Yaw, 0);
	Surface->SetWorldRotation(Yaw);
	Surface->SetWorldScale3D(FVector(Extent.X / 50.0, Extent.Y / 50.0, 1));
	const float Depth = FMath::Clamp(MaxWaterDepthCm, 0.1f, 50.0f);
	const float Width = FMath::Clamp(WetEdgeWidth, 0.01f, 0.4f);
	WetEdge->SetWorldLocation(GetActorLocation() - FVector(0, 0, Depth * 0.5f));
	WetEdge->SetWorldRotation(FRotator(-90, Yaw.Yaw, 0));
	WetEdge->SetWorldScale3D(FVector::OneVector);
	WetEdge->DecalSize = FVector(Depth * 0.5f + 0.5f, Extent.Y * (1 + Width), Extent.X * (1 + Width));
	WetEdge->MarkRenderStateDirty();

	if (!WaterInstance || WaterInstance->Parent != WaterMaterial)
	{
		WaterInstance = WaterMaterial ? UMaterialInstanceDynamic::Create(WaterMaterial, this) : nullptr;
	}
	Surface->SetMaterial(0, WaterInstance);
	Surface->SetVisibility(WaterMaterial != nullptr);
	ApplySharedParameters(WaterInstance);
	if (WaterInstance)
	{
		WaterInstance->SetScalarParameterValue(TEXT("WaterRoughness"), FMath::Clamp(WaterRoughness, 0.02f, 1.0f));
		WaterInstance->SetVectorParameterValue(TEXT("Absorption"), Absorption.GetClamped(0, 100));
		WaterInstance->SetVectorParameterValue(TEXT("Scattering"), Scattering.GetClamped(0, 100));
		WaterInstance->SetScalarParameterValue(TEXT("RippleStrength"), FMath::Clamp(RippleStrength, 0.0f, 0.15f));
		WaterInstance->SetScalarParameterValue(TEXT("RippleSpeed"), FMath::Clamp(RippleSpeed, 0.0f, 5.0f));
	}
	if (!WetEdgeInstance || WetEdgeInstance->Parent != WetEdgeMaterial)
	{
		WetEdgeInstance = WetEdgeMaterial ? UMaterialInstanceDynamic::Create(WetEdgeMaterial, this) : nullptr;
	}
	WetEdge->SetDecalMaterial(WetEdgeInstance);
	WetEdge->SetVisibility(WetEdgeMaterial != nullptr && Wetness > 0);
	ApplySharedParameters(WetEdgeInstance);
	if (WetEdgeInstance)
	{
		WetEdgeInstance->SetScalarParameterValue(TEXT("WetEdgeWidth"), Width);
		WetEdgeInstance->SetScalarParameterValue(TEXT("Wetness"), FMath::Clamp(Wetness, 0.0f, 1.0f));
		WetEdgeInstance->SetVectorParameterValue(TEXT("WetGroundColor"), WetGroundColor);
	}
}

bool ATunaSweeperShallowPuddleActor::ContainsGroundPoint(FVector GroundPoint) const
{
	if (GroundPoint.ContainsNaN()) return false;
	const FVector Delta = GroundPoint - GetActorLocation();
	// Half a centimetre allows floor trace precision without wetting an upper floor.
	if (Delta.Z > 0.5 || Delta.Z < -FMath::Clamp(MaxWaterDepthCm, 0.1f, 50.0f)) return false;
	const FVector Local = FRotator(0, GetActorRotation().Yaw, 0).UnrotateVector(Delta);
	const FVector2D Extent = GetWorldHalfExtent();
	const FVector2D Point(Local.X / Extent.X, Local.Y / Extent.Y);
	const double Angle = FMath::Atan2(Point.Y, Point.X);
	// Keep this equation identical to the saved water and wet-edge material Custom expressions.
	const double Radius = 0.8 + FMath::Clamp(OutlineIrregularity, 0.0f, 1.0f) *
		(0.1 * FMath::Sin(3 * Angle) + 0.055 * FMath::Sin(5 * Angle) + 0.025 * FMath::Cos(7 * Angle));
	return Point.SizeSquared() <= Radius * Radius;
}

bool ATunaSweeperShallowPuddleActor::IsWaterSurfaceActive() const
{
	return !IsHidden() && WaterMaterial && Surface && Surface->IsVisible() && !Surface->bHiddenInGame;
}

ATunaSweeperShallowPuddleActor* ATunaSweeperShallowPuddleActor::FindPuddleAtGroundPoint(UWorld* World, const FVector& GroundPoint)
{
	if (!World) return nullptr;
	ATunaSweeperShallowPuddleActor* Result = nullptr;
	for (TActorIterator<ATunaSweeperShallowPuddleActor> It(World); It; ++It)
	{
		if (It->IsWaterSurfaceActive() && It->ContainsGroundPoint(GroundPoint) &&
			(!Result || It->GetActorLocation().Z > Result->GetActorLocation().Z))
		{
			Result = *It;
		}
	}
	return Result;
}

void ATunaSweeperShallowPuddleActor::NotifyFootstep(FVector GroundPoint, float SpeedCmPerSecond, bool bSprinting, AActor* StepInstigator)
{
	if (!IsWaterSurfaceActive() || !ContainsGroundPoint(GroundPoint)) return;
	GroundPoint.Z = GetActorLocation().Z;
	OnPuddleFootstep.Broadcast(GroundPoint, FMath::Max(0.0f, SpeedCmPerSecond), bSprinting, StepInstigator);
}
