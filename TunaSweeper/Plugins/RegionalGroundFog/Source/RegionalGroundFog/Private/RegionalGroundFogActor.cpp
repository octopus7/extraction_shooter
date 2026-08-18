#include "RegionalGroundFogActor.h"

#include "Components/LocalFogVolumeComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

namespace RegionalGroundFog
{
	constexpr int32 MaxFogNodes = 8;
	constexpr float LocalFogBaseRadius = 500.0f;

	FVector2D RandomUnitVector(FRandomStream& Stream)
	{
		const float Angle = Stream.FRandRange(0.0f, UE_TWO_PI);
		return FVector2D(FMath::Cos(Angle), FMath::Sin(Angle));
	}
}

ARegionalGroundFogActor::ARegionalGroundFogActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	Visualization = CreateDefaultSubobject<URegionalGroundFogVisualizationComponent>(TEXT("FogRegionVisualization"));
	Visualization->SetupAttachment(SceneRoot);
	Visualization->SetHiddenInGame(true);

	FogNodes.Add(FRegionalGroundFogNode());
	CardMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/RegionalGroundFog/Materials/M_RegionalGroundFogCard.M_RegionalGroundFogCard")));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMeshFinder(TEXT("/Engine/BasicShapes/Plane.Plane"));
	CardPlaneMesh = PlaneMeshFinder.Succeeded() ? PlaneMeshFinder.Object : nullptr;
}

void ARegionalGroundFogActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SynchronizeFogVolumeComponents();
}

void ARegionalGroundFogActor::BeginPlay()
{
	Super::BeginPlay();
	SynchronizeFogVolumeComponents();
	CreateDriftingCards();
}

void ARegionalGroundFogActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearDriftingCards();
	Super::EndPlay(EndPlayReason);
}

void ARegionalGroundFogActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	const int32 ActiveNodeCount = GetActiveNodeCount();
	if (ActiveNodeCount == 0 || RuntimeCards.IsEmpty())
	{
		return;
	}

	const float TimeSeconds = GetGameTimeSinceCreation();
	for (int32 CardIndex = 0; CardIndex < RuntimeCards.Num(); ++CardIndex)
	{
		FRuntimeDriftingCard& Card = RuntimeCards[CardIndex];
		if (!Card.Component || !Card.Material || !FogNodes.IsValidIndex(Card.NodeIndex))
		{
			continue;
		}

		const FRegionalGroundFogNode& Node = FogNodes[Card.NodeIndex];
		FVector RelativeLocation = Card.Component->GetRelativeLocation();
		RelativeLocation += FVector(Card.Direction.X, Card.Direction.Y, 0.0f) * DriftSpeed * DeltaSeconds;
		RelativeLocation.Z = Node.LocalCenter.Z + CardHeight + FMath::Sin(TimeSeconds * 0.35f + Card.Phase) * CurlAmplitude;

		const FVector2D Offset(RelativeLocation.X - Node.LocalCenter.X, RelativeLocation.Y - Node.LocalCenter.Y);
		const float DistanceFromNodeCenter = Offset.Size();
		if (DistanceFromNodeCenter >= Node.OuterRadius * 0.97f)
		{
			RespawnCard(CardIndex, true);
			continue;
		}

		const float Fade = 1.0f - FMath::SmoothStep(Node.CoreRadius, Node.OuterRadius, DistanceFromNodeCenter);
		Card.Material->SetScalarParameterValue(TEXT("OpacityScale"), CardOpacity * Fade);
		Card.Component->SetRelativeLocation(RelativeLocation);
	}
}

void ARegionalGroundFogActor::AddFogNode()
{
	Modify();
	if (FogNodes.Num() >= RegionalGroundFog::MaxFogNodes)
	{
		return;
	}

	FRegionalGroundFogNode NewNode;
	if (!FogNodes.IsEmpty())
	{
		NewNode = FogNodes.Last();
		NewNode.LocalCenter += FVector(NewNode.OuterRadius * 0.75f, 0.0f, 0.0f);
	}
	FogNodes.Add(NewNode);
	RefreshFogRegion();
}

void ARegionalGroundFogActor::RemoveLastFogNode()
{
	Modify();
	if (FogNodes.Num() <= 1)
	{
		return;
	}

	FogNodes.RemoveAt(FogNodes.Num() - 1);
	RefreshFogRegion();
}

void ARegionalGroundFogActor::RefreshFogRegion()
{
	NormalizeFogNodes();
	SynchronizeFogVolumeComponents();
	if (HasActorBegunPlay())
	{
		CreateDriftingCards();
	}
}

int32 ARegionalGroundFogActor::GetActiveNodeCount() const
{
	return FMath::Min(FogNodes.Num(), RegionalGroundFog::MaxFogNodes);
}

void ARegionalGroundFogActor::NormalizeFogNodes()
{
	if (FogNodes.IsEmpty())
	{
		FogNodes.Add(FRegionalGroundFogNode());
	}

	for (FRegionalGroundFogNode& Node : FogNodes)
	{
		Node.OuterRadius = FMath::Max(Node.OuterRadius, 100.0f);
		Node.CoreRadius = FMath::Clamp(Node.CoreRadius, 0.0f, Node.OuterRadius);
		Node.DensityMultiplier = FMath::Max(Node.DensityMultiplier, 0.0f);
	}
}

void ARegionalGroundFogActor::SynchronizeFogVolumeComponents()
{
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		return;
	}

	NormalizeFogNodes();
	const int32 DesiredComponentCount = GetActiveNodeCount();

	for (int32 Index = FogVolumeComponents.Num() - 1; Index >= DesiredComponentCount; --Index)
	{
		if (ULocalFogVolumeComponent* Component = FogVolumeComponents[Index])
		{
			Component->DestroyComponent();
		}
		FogVolumeComponents.RemoveAt(Index);
	}

	while (FogVolumeComponents.Num() < DesiredComponentCount)
	{
		ULocalFogVolumeComponent* Component = NewObject<ULocalFogVolumeComponent>(this, NAME_None, RF_Transactional);
		if (!Component)
		{
			break;
		}

		Component->SetMobility(EComponentMobility::Movable);
		Component->SetupAttachment(SceneRoot);
		AddInstanceComponent(Component);
		Component->RegisterComponent();
		FogVolumeComponents.Add(Component);
	}

	for (int32 Index = 0; Index < FogVolumeComponents.Num(); ++Index)
	{
		ULocalFogVolumeComponent* Component = FogVolumeComponents[Index];
		if (!Component || !FogNodes.IsValidIndex(Index))
		{
			continue;
		}

		const FRegionalGroundFogNode& Node = FogNodes[Index];
		Component->SetRelativeLocation(Node.LocalCenter);
		Component->SetRelativeScale3D(FVector(Node.OuterRadius / RegionalGroundFog::LocalFogBaseRadius));
		Component->SetRadialFogExtinction(RadialFogDensity * Node.DensityMultiplier);
		Component->SetHeightFogExtinction(HeightFogDensity * Node.DensityMultiplier);
		Component->SetHeightFogFalloff(HeightFogFalloff);
		Component->SetHeightFogOffset(HeightFogOffset);
		Component->SetFogPhaseG(ScatteringDistribution);
		Component->SetFogAlbedo(FogAlbedo);
		Component->FogSortPriority = FogSortPriority;
		Component->MarkRenderStateDirty();
	}
}

void ARegionalGroundFogActor::ClearDriftingCards()
{
	for (FRuntimeDriftingCard& Card : RuntimeCards)
	{
		if (Card.Component)
		{
			Card.Component->DestroyComponent();
		}
	}
	RuntimeCards.Reset();
}

void ARegionalGroundFogActor::CreateDriftingCards()
{
	ClearDriftingCards();
	if (!bEnableDriftingCards || DriftingCardCount <= 0 || GetActiveNodeCount() == 0 || !CardPlaneMesh)
	{
		return;
	}

	UMaterialInterface* BaseMaterial = CardMaterial.LoadSynchronous();
	if (!BaseMaterial)
	{
		UE_LOG(LogTemp, Warning, TEXT("RegionalGroundFog: card material is unavailable. Local Fog Volumes remain active."));
		return;
	}

	RandomStream.Initialize(RandomSeed);
	RuntimeCards.Reserve(DriftingCardCount);
	for (int32 Index = 0; Index < DriftingCardCount; ++Index)
	{
		UStaticMeshComponent* CardComponent = NewObject<UStaticMeshComponent>(this, NAME_None, RF_Transient);
		if (!CardComponent)
		{
			continue;
		}

		CardComponent->SetMobility(EComponentMobility::Movable);
		CardComponent->SetupAttachment(SceneRoot);
		CardComponent->SetStaticMesh(CardPlaneMesh);
		CardComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
		CardComponent->SetGenerateOverlapEvents(false);
		CardComponent->SetCastShadow(false);
		CardComponent->SetReceivesDecals(false);
		CardComponent->SetCanEverAffectNavigation(false);
		AddInstanceComponent(CardComponent);
		CardComponent->RegisterComponent();

		FRuntimeDriftingCard& Card = RuntimeCards.AddDefaulted_GetRef();
		Card.Component = CardComponent;
		Card.Material = UMaterialInstanceDynamic::Create(BaseMaterial, this);
		Card.Direction = RegionalGroundFog::RandomUnitVector(RandomStream);
		Card.Phase = RandomStream.FRandRange(0.0f, UE_TWO_PI);
		Card.SizeMultiplier = RandomStream.FRandRange(0.70f, 1.22f);
		if (Card.Material)
		{
			Card.Material->SetVectorParameterValue(TEXT("FogColor"), CardColor);
			CardComponent->SetMaterial(0, Card.Material);
		}

		RespawnCard(RuntimeCards.Num() - 1, true);
	}
}

void ARegionalGroundFogActor::RespawnCard(const int32 CardIndex, const bool bRandomizeNode)
{
	if (!RuntimeCards.IsValidIndex(CardIndex) || GetActiveNodeCount() == 0)
	{
		return;
	}

	FRuntimeDriftingCard& Card = RuntimeCards[CardIndex];
	if (!Card.Component || !Card.Material)
	{
		return;
	}

	if (bRandomizeNode || !FogNodes.IsValidIndex(Card.NodeIndex))
	{
		Card.NodeIndex = RandomStream.RandRange(0, GetActiveNodeCount() - 1);
	}

	const FRegionalGroundFogNode& Node = FogNodes[Card.NodeIndex];
	const FVector2D Direction = RegionalGroundFog::RandomUnitVector(RandomStream);
	const float Radius = FMath::Sqrt(RandomStream.FRand()) * FMath::Max(Node.CoreRadius, 1.0f);
	Card.Direction = RegionalGroundFog::RandomUnitVector(RandomStream);
	Card.Phase = RandomStream.FRandRange(0.0f, UE_TWO_PI);

	const FVector Location = Node.LocalCenter + FVector(Direction.X * Radius, Direction.Y * Radius, CardHeight + RandomStream.FRandRange(-CardHeightVariance, CardHeightVariance));
	const float PlaneScale = (CardDiameter * Card.SizeMultiplier) / 100.0f;
	Card.Component->SetRelativeLocation(Location);
	Card.Component->SetRelativeRotation(FRotator(0.0f, RandomStream.FRandRange(0.0f, 360.0f), 0.0f));
	Card.Component->SetRelativeScale3D(FVector(PlaneScale * 1.25f, PlaneScale * 0.80f, 1.0f));
	Card.Material->SetScalarParameterValue(TEXT("OpacityScale"), CardOpacity * RandomStream.FRandRange(0.65f, 1.0f));
}

#if WITH_EDITOR
void ARegionalGroundFogActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshFogRegion();
}
#endif
