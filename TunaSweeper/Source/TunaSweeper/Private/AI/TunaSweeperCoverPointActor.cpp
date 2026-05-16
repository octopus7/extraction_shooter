#include "AI/TunaSweeperCoverPointActor.h"

#include "Components/ArrowComponent.h"
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/Texture2D.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	FVector GetSanitizedExtent(const FVector& Extent)
	{
		return FVector(
			FMath::Max(1.0f, FMath::Abs(Extent.X)),
			FMath::Max(1.0f, FMath::Abs(Extent.Y)),
			FMath::Max(1.0f, FMath::Abs(Extent.Z)));
	}

	float GetPeekSideSign(ETunaSweeperCoverPeekSide PeekSide)
	{
		return PeekSide == ETunaSweeperCoverPeekSide::Right ? 1.0f : -1.0f;
	}
}

ATunaSweeperCoverPointActor::ATunaSweeperCoverPointActor()
{
	PrimaryActorTick.bCanEverTick = false;

	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootScene);

	CoverAreaComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CoverArea"));
	CoverAreaComponent->SetupAttachment(RootScene);
	CoverAreaComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CoverAreaComponent->SetGenerateOverlapEvents(false);
	CoverAreaComponent->SetCanEverAffectNavigation(false);
	CoverAreaComponent->SetHiddenInGame(true);
	CoverAreaComponent->ShapeColor = FColor(0, 175, 255);

	ReservationRadiusComponent = CreateDefaultSubobject<USphereComponent>(TEXT("ReservationRadius"));
	ReservationRadiusComponent->SetupAttachment(RootScene);
	ReservationRadiusComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ReservationRadiusComponent->SetGenerateOverlapEvents(false);
	ReservationRadiusComponent->SetCanEverAffectNavigation(false);
	ReservationRadiusComponent->SetHiddenInGame(true);
	ReservationRadiusComponent->ShapeColor = FColor(0, 255, 150);

	FacingArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("FacingDirection"));
	FacingArrowComponent->SetupAttachment(RootScene);
	FacingArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FacingArrowComponent->SetHiddenInGame(true);
	FacingArrowComponent->ArrowColor = FColor(255, 210, 0);

	LeftPeekArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("LeftPeekLocation"));
	LeftPeekArrowComponent->SetupAttachment(RootScene);
	LeftPeekArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	LeftPeekArrowComponent->SetHiddenInGame(true);
	LeftPeekArrowComponent->ArrowColor = FColor(90, 150, 255);

	RightPeekArrowComponent = CreateDefaultSubobject<UArrowComponent>(TEXT("RightPeekLocation"));
	RightPeekArrowComponent->SetupAttachment(RootScene);
	RightPeekArrowComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	RightPeekArrowComponent->SetHiddenInGame(true);
	RightPeekArrowComponent->ArrowColor = FColor(90, 150, 255);

#if WITH_EDITORONLY_DATA
	EditorIconComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("EditorIcon"));
	EditorIconComponent->SetupAttachment(RootScene);
	EditorIconComponent->SetHiddenInGame(true);
	EditorIconComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));

	static ConstructorHelpers::FObjectFinder<UTexture2D> EditorIconTexture(TEXT("/Engine/EditorResources/S_Note.S_Note"));
	if (EditorIconTexture.Succeeded())
	{
		EditorIconComponent->SetSprite(EditorIconTexture.Object);
	}
#endif

	UpdateVisualizerComponents();
}

void ATunaSweeperCoverPointActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateVisualizerComponents();
}

FVector ATunaSweeperCoverPointActor::GetCoverLocation() const
{
	return GetActorLocation();
}

FVector ATunaSweeperCoverPointActor::GetCoverFacingDirection() const
{
	return GetActorForwardVector();
}

FVector ATunaSweeperCoverPointActor::GetPeekLocation(ETunaSweeperCoverPeekSide PeekSide) const
{
	const FVector LocalPeekLocation(
		PeekForwardOffset,
		GetPeekSideSign(PeekSide) * PeekSideOffset,
		0.0f);

	return GetActorTransform().TransformPosition(LocalPeekLocation);
}

bool ATunaSweeperCoverPointActor::IsPeekSideEnabled(ETunaSweeperCoverPeekSide PeekSide) const
{
	return PeekSide == ETunaSweeperCoverPeekSide::Right ? bAllowRightPeek : bAllowLeftPeek;
}

bool ATunaSweeperCoverPointActor::IsReserved() const
{
	return ReservedBy.IsValid();
}

AActor* ATunaSweeperCoverPointActor::GetReservedBy() const
{
	return ReservedBy.Get();
}

bool ATunaSweeperCoverPointActor::TryReserve(AActor* ReservingActor)
{
	if (!ReservingActor)
	{
		return false;
	}

	if (ReservedBy.IsValid() && ReservedBy.Get() != ReservingActor)
	{
		return false;
	}

	ReservedBy = ReservingActor;
	return true;
}

void ATunaSweeperCoverPointActor::ReleaseReservation(AActor* ReservingActor)
{
	if (!ReservedBy.IsValid())
	{
		return;
	}

	if (!ReservingActor || ReservedBy.Get() == ReservingActor)
	{
		ReservedBy.Reset();
	}
}

void ATunaSweeperCoverPointActor::UpdateVisualizerComponents()
{
	const FVector SanitizedCoverExtent = GetSanitizedExtent(CoverAreaExtent);
	const float SanitizedReservationRadius = FMath::Max(1.0f, ReservationRadius);
	const float SanitizedPeekForwardOffset = FMath::Max(0.0f, PeekForwardOffset);
	const float SanitizedPeekSideOffset = FMath::Max(0.0f, PeekSideOffset);
	const float ArrowLength = FMath::Max(120.0f, SanitizedCoverExtent.X + SanitizedPeekForwardOffset);

	if (CoverAreaComponent)
	{
		CoverAreaComponent->SetBoxExtent(SanitizedCoverExtent, false);
		CoverAreaComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SanitizedCoverExtent.Z));
	}

	if (ReservationRadiusComponent)
	{
		ReservationRadiusComponent->SetSphereRadius(SanitizedReservationRadius, false);
		ReservationRadiusComponent->SetRelativeLocation(FVector::ZeroVector);
	}

	if (FacingArrowComponent)
	{
		FacingArrowComponent->ArrowSize = 1.2f;
		FacingArrowComponent->ArrowLength = ArrowLength;
		FacingArrowComponent->SetRelativeLocation(FVector(0.0f, 0.0f, SanitizedCoverExtent.Z));
		FacingArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (LeftPeekArrowComponent)
	{
		LeftPeekArrowComponent->SetVisibility(bAllowLeftPeek);
		LeftPeekArrowComponent->ArrowSize = 0.7f;
		LeftPeekArrowComponent->ArrowLength = 70.0f;
		LeftPeekArrowComponent->SetRelativeLocation(FVector(SanitizedPeekForwardOffset, -SanitizedPeekSideOffset, SanitizedCoverExtent.Z));
		LeftPeekArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
	}

	if (RightPeekArrowComponent)
	{
		RightPeekArrowComponent->SetVisibility(bAllowRightPeek);
		RightPeekArrowComponent->ArrowSize = 0.7f;
		RightPeekArrowComponent->ArrowLength = 70.0f;
		RightPeekArrowComponent->SetRelativeLocation(FVector(SanitizedPeekForwardOffset, SanitizedPeekSideOffset, SanitizedCoverExtent.Z));
		RightPeekArrowComponent->SetRelativeRotation(FRotator::ZeroRotator);
	}
}
