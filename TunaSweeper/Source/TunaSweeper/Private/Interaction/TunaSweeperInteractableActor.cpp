#include "Interaction/TunaSweeperInteractableActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/TunaSweeperInteractionSubsystem.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UObject/ConstructorHelpers.h"

namespace TunaSweeperInteractionMarkerLayout
{
	constexpr float DrawWidth = 360.0f;
	constexpr float DrawHeight = 80.0f;
	constexpr float MarkerRootWidth = 300.0f;
	constexpr float MarkerBoxWidth = 56.0f;

	FVector2D GetDrawSize()
	{
		return FVector2D(DrawWidth, DrawHeight);
	}

	FVector2D GetMarkerCenteredPivot()
	{
		const float MarkerCenterX = ((DrawWidth - MarkerRootWidth) * 0.5f) + (MarkerBoxWidth * 0.5f);
		return FVector2D(MarkerCenterX / DrawWidth, 0.5f);
	}
}

ATunaSweeperInteractableActor::ATunaSweeperInteractableActor()
{
	PrimaryActorTick.bCanEverTick = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.75f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}

	MarkerWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("MarkerWidget"));
	MarkerWidgetComponent->SetupAttachment(RootComponent);
	MarkerWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	MarkerWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ApplyMarkerWidgetLayout();
	MarkerWidgetComponent->SetVisibility(false);

	MarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
}

void ATunaSweeperInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	ApplyMarkerWidgetLayout();
	EnsureMarkerWidgetClass();
	ApplyMarkerState();

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = GetWorld()->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		InteractionSubsystem->RegisterInteractable(this);
	}
}

void ATunaSweeperInteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	ApplyMarkerWidgetLayout();
	EnsureMarkerWidgetClass();
	ApplyMarkerState();
}

void ATunaSweeperInteractableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMarker(DeltaSeconds);
}

void ATunaSweeperInteractableActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
		{
			InteractionSubsystem->UnregisterInteractable(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

bool ATunaSweeperInteractableActor::IsWithinInteractionDistance(const AActor* OtherActor) const
{
	return OtherActor && GetSquaredDistance2DTo(OtherActor) <= FMath::Square(InteractionDistance);
}

float ATunaSweeperInteractableActor::GetSquaredDistance2DTo(const AActor* OtherActor) const
{
	return OtherActor
		? FVector::DistSquared2D(GetActorLocation(), OtherActor->GetActorLocation())
		: TNumericLimits<float>::Max();
}

bool ATunaSweeperInteractableActor::RequestInteraction(APawn* InstigatorPawn)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = World->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		return InteractionSubsystem->RequestInteraction(this, InstigatorPawn);
	}

	return false;
}

void ATunaSweeperInteractableActor::ConfigureInteractionDefaults(
	ETunaSweeperInteractionType InInteractionType,
	const FText& InInteractionDisplayName,
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass)
{
	InteractionType = InInteractionType;
	InteractionDisplayName = InInteractionDisplayName;
	MarkerWidgetClass = InMarkerWidgetClass;
}

void ATunaSweeperInteractableActor::ApplyMarkerWidgetLayout()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	MarkerWidgetComponent->SetDrawSize(TunaSweeperInteractionMarkerLayout::GetDrawSize());
	MarkerWidgetComponent->SetPivot(TunaSweeperInteractionMarkerLayout::GetMarkerCenteredPivot());
}

void ATunaSweeperInteractableActor::EnsureMarkerWidgetClass()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	if (TSubclassOf<UTunaSweeperInteractionMarkerWidget> LoadedClass = MarkerWidgetClass.LoadSynchronous())
	{
		if (MarkerWidgetComponent->GetWidgetClass() != LoadedClass)
		{
			MarkerWidgetComponent->SetWidgetClass(LoadedClass);
		}
	}
}

void ATunaSweeperInteractableActor::UpdateMarker(float DeltaSeconds)
{
	const APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(this, 0);
	const bool bInsideVisibleDistance =
		PlayerPawn &&
		FVector::DistSquared2D(PlayerPawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(MarkerVisibleDistance);

	const float TargetAlpha = bInsideVisibleDistance ? 1.0f : 0.0f;
	const float TargetScale = bInsideVisibleDistance ? 1.0f : 3.0f;
	bool bIsFocusedInteractable = false;

	if (UTunaSweeperInteractionSubsystem* InteractionSubsystem = GetWorld()->GetSubsystem<UTunaSweeperInteractionSubsystem>())
	{
		bIsFocusedInteractable =
			InteractionSubsystem->GetFocusedInteractable() == this &&
			PlayerPawn &&
			IsWithinInteractionDistance(PlayerPawn);
	}

	const float TargetLabelAlpha = bIsFocusedInteractable ? 1.0f : 0.0f;

	MarkerAlpha = FMath::FInterpTo(MarkerAlpha, TargetAlpha, DeltaSeconds, MarkerFadeInterpSpeed);
	MarkerRingScale = FMath::FInterpTo(MarkerRingScale, TargetScale, DeltaSeconds, MarkerScaleInterpSpeed);
	LabelAlpha = FMath::FInterpTo(LabelAlpha, TargetLabelAlpha, DeltaSeconds, LabelFadeInterpSpeed);

	ApplyMarkerState();
}

void ATunaSweeperInteractableActor::ApplyMarkerState()
{
	if (!MarkerWidgetComponent)
	{
		return;
	}

	const bool bVisible = MarkerAlpha > 0.01f;
	MarkerWidgetComponent->SetVisibility(bVisible);

	if (UTunaSweeperInteractionMarkerWidget* MarkerWidget = Cast<UTunaSweeperInteractionMarkerWidget>(MarkerWidgetComponent->GetUserWidgetObject()))
	{
		MarkerWidget->SetMarkerText(InteractionDisplayName);
		MarkerWidget->SetMarkerPresentation(MarkerAlpha, MarkerRingScale, LabelAlpha);
	}
}
