#include "Interaction/TunaSweeperSelfDestructInteractableActor.h"

#include "Component/TunaSweeperVitalsComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "TimerManager.h"
#include "UI/TunaSweeperSpeechBubbleWidget.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperSelfDestructInteractableActor::ATunaSweeperSelfDestructInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(RootComponent);
	VisualMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.55f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
	}

	InteractableComponent = CreateDefaultSubobject<UTunaSweeperInteractableComponent>(TEXT("Interactable"));
	InteractableComponent->SetupAttachment(RootComponent);
	InteractableComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 110.0f));
	InteractableComponent->SetInteractionTypeAndDisplayName(
		ETunaSweeperInteractionType::SelfDestruct,
		FText::FromString(TEXT("\uC790\uD3ED")));

	SpeechBubbleWidgetClass = TSoftClassPtr<UTunaSweeperSpeechBubbleWidget>(
		FSoftObjectPath(TEXT("/Game/UI/WBP_SpeechBubble.WBP_SpeechBubble_C")));

	SpeechBubbleWidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("SpeechBubble"));
	SpeechBubbleWidgetComponent->SetupAttachment(RootComponent);
	SpeechBubbleWidgetComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 180.0f));
	SpeechBubbleWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	SpeechBubbleWidgetComponent->SetDrawSize(FVector2D(180.0f, 72.0f));
	SpeechBubbleWidgetComponent->SetPivot(FVector2D(0.5f, 1.0f));
	SpeechBubbleWidgetComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SpeechBubbleWidgetComponent->SetVisibility(false);
	SpeechBubbleWidgetComponent->SetHiddenInGame(false);
}

bool ATunaSweeperSelfDestructInteractableActor::StartSelfDestruct(APawn* InstigatorPawn)
{
	if (bExploded || bCountdownActive || !GetWorld())
	{
		return bCountdownActive;
	}

	EnsureSpeechBubbleWidgetClass();

	bCountdownActive = true;
	CountdownInstigator = InstigatorPawn;
	CurrentCountdownValue = FMath::Max(1, CountdownStartNumber);
	SetSpeechBubbleText(FText::AsNumber(CurrentCountdownValue));

	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&ATunaSweeperSelfDestructInteractableActor::AdvanceCountdown,
		FMath::Max(0.01f, CountdownStepSeconds),
		true);

	return true;
}

void ATunaSweeperSelfDestructInteractableActor::ConfigureSelfDestructDefaults(
	TSoftClassPtr<UTunaSweeperInteractionMarkerWidget> InMarkerWidgetClass,
	TSoftClassPtr<UTunaSweeperSpeechBubbleWidget> InSpeechBubbleWidgetClass)
{
	Modify();
	if (InteractableComponent)
	{
		InteractableComponent->ConfigureInteractionDefaults(
			ETunaSweeperInteractionType::SelfDestruct,
			FText::FromString(TEXT("\uC790\uD3ED")),
			InMarkerWidgetClass);
	}

	if (!InSpeechBubbleWidgetClass.IsNull())
	{
		SpeechBubbleWidgetClass = InSpeechBubbleWidgetClass;
		if (SpeechBubbleWidgetComponent)
		{
			if (TSubclassOf<UTunaSweeperSpeechBubbleWidget> LoadedClass = SpeechBubbleWidgetClass.LoadSynchronous())
			{
				SpeechBubbleWidgetComponent->SetWidgetClass(LoadedClass);
			}
		}
	}
}

void ATunaSweeperSelfDestructInteractableActor::AdvanceCountdown()
{
	--CurrentCountdownValue;
	if (CurrentCountdownValue > 0)
	{
		SetSpeechBubbleText(FText::AsNumber(CurrentCountdownValue));
		return;
	}

	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
	SetSpeechBubbleText(FText::FromString(TEXT("Boom!")));
	if (BoomDisplaySeconds <= 0.0f)
	{
		Explode();
		return;
	}

	GetWorldTimerManager().SetTimer(
		BoomTimerHandle,
		this,
		&ATunaSweeperSelfDestructInteractableActor::Explode,
		BoomDisplaySeconds,
		false);
}

void ATunaSweeperSelfDestructInteractableActor::Explode()
{
	if (bExploded)
	{
		return;
	}

	bExploded = true;
	bCountdownActive = false;
	ApplyExplosionDamage();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Red, TEXT("Boom!"));
	}

	Destroy();
}

void ATunaSweeperSelfDestructInteractableActor::ApplyExplosionDamage()
{
	UWorld* World = GetWorld();
	if (!World || ExplosionDamage <= 0.0f || ExplosionRadius <= 0.0f)
	{
		return;
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(TunaSweeperSelfDestructOverlap), false, this);
	if (!World->OverlapMultiByObjectType(
		Overlaps,
		GetActorLocation(),
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(ExplosionRadius),
		QueryParams))
	{
		return;
	}

	TSet<AActor*> DamagedActors;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* DamagedActor = Overlap.GetActor();
		if (!IsValid(DamagedActor) || DamagedActors.Contains(DamagedActor))
		{
			continue;
		}

		DamagedActors.Add(DamagedActor);
		if (UTunaSweeperVitalsComponent* VitalsComponent = DamagedActor->FindComponentByClass<UTunaSweeperVitalsComponent>())
		{
			FTunaSweeperVitalsDelta DamageDelta;
			DamageDelta.Health = -ExplosionDamage;
			VitalsComponent->ApplyVitalsDelta(DamageDelta);
		}
	}
}

void ATunaSweeperSelfDestructInteractableActor::SetSpeechBubbleText(const FText& InText)
{
	if (!SpeechBubbleWidgetComponent)
	{
		return;
	}

	SpeechBubbleWidgetComponent->SetVisibility(true);
	if (UTunaSweeperSpeechBubbleWidget* SpeechBubbleWidget =
		Cast<UTunaSweeperSpeechBubbleWidget>(SpeechBubbleWidgetComponent->GetUserWidgetObject()))
	{
		SpeechBubbleWidget->SetBubbleText(InText);
	}
}

void ATunaSweeperSelfDestructInteractableActor::EnsureSpeechBubbleWidgetClass()
{
	if (!SpeechBubbleWidgetComponent)
	{
		return;
	}

	if (TSubclassOf<UTunaSweeperSpeechBubbleWidget> LoadedClass = SpeechBubbleWidgetClass.LoadSynchronous())
	{
		if (SpeechBubbleWidgetComponent->GetWidgetClass() != LoadedClass)
		{
			SpeechBubbleWidgetComponent->SetWidgetClass(LoadedClass);
		}
		SpeechBubbleWidgetComponent->InitWidget();
	}
}
