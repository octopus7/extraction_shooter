#include "Interaction/TunaSweeperInteractableActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/TunaSweeperInteractionMarkerWidget.h"
#include "UObject/ConstructorHelpers.h"

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
	MarkerWidgetComponent->SetDrawSize(FVector2D(360.0f, 80.0f));
	MarkerWidgetComponent->SetPivot(FVector2D(0.5f, 0.5f));
	MarkerWidgetComponent->SetVisibility(false);

	MarkerWidgetClass = TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C")));
}

void ATunaSweeperInteractableActor::BeginPlay()
{
	Super::BeginPlay();

	EnsureMarkerWidgetClass();
	ApplyMarkerState();
}

void ATunaSweeperInteractableActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	EnsureMarkerWidgetClass();
	ApplyMarkerState();
}

void ATunaSweeperInteractableActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UpdateMarker(DeltaSeconds);
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

	MarkerAlpha = FMath::FInterpTo(MarkerAlpha, TargetAlpha, DeltaSeconds, MarkerFadeInterpSpeed);
	MarkerRingScale = FMath::FInterpTo(MarkerRingScale, TargetScale, DeltaSeconds, MarkerScaleInterpSpeed);

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
		MarkerWidget->SetMarkerPresentation(MarkerAlpha, MarkerRingScale);
	}
}
