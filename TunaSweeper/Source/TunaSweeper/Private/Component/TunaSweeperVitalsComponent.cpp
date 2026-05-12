#include "Component/TunaSweeperVitalsComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

void FTunaSweeperVitalsState::Normalize()
{
	MaxHealth = FMath::Max(1.0f, MaxHealth);
	MaxFood = FMath::Max(1.0f, MaxFood);
	MaxHydration = FMath::Max(1.0f, MaxHydration);
	Health = FMath::Clamp(Health, 0.0f, MaxHealth);
	Food = FMath::Clamp(Food, 0.0f, MaxFood);
	Hydration = FMath::Clamp(Hydration, 0.0f, MaxHydration);
}

void FTunaSweeperVitalsDepletionRates::ClampNonNegative()
{
	HealthPerSecond = FMath::Max(0.0f, HealthPerSecond);
	FoodPerSecond = FMath::Max(0.0f, FoodPerSecond);
	HydrationPerSecond = FMath::Max(0.0f, HydrationPerSecond);
}

void FTunaSweeperVitalsDepletionMultipliers::ClampNonNegative()
{
	Health = FMath::Max(0.0f, Health);
	Food = FMath::Max(0.0f, Food);
	Hydration = FMath::Max(0.0f, Hydration);
}

UTunaSweeperVitalsComponent::UTunaSweeperVitalsComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UTunaSweeperVitalsComponent::BeginPlay()
{
	Super::BeginPlay();

	VitalsState.Normalize();
	BaseDepletionRates.ClampNonNegative();
	AdditionalDepletionRates.ClampNonNegative();
	DepletionRateMultipliers.ClampNonNegative();
	BroadcastVitalsChanged();
}

void UTunaSweeperVitalsComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HasAuthority() || DeltaTime <= 0.0f)
	{
		return;
	}

	const FTunaSweeperVitalsDepletionRates EffectiveRates = GetEffectiveDepletionRates();
	if (FMath::IsNearlyZero(EffectiveRates.HealthPerSecond) &&
		FMath::IsNearlyZero(EffectiveRates.FoodPerSecond) &&
		FMath::IsNearlyZero(EffectiveRates.HydrationPerSecond))
	{
		return;
	}

	FTunaSweeperVitalsDelta Delta;
	Delta.Health = -EffectiveRates.HealthPerSecond * DeltaTime;
	Delta.Food = -EffectiveRates.FoodPerSecond * DeltaTime;
	Delta.Hydration = -EffectiveRates.HydrationPerSecond * DeltaTime;
	ApplyVitalsDeltaInternal(Delta);
}

void UTunaSweeperVitalsComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTunaSweeperVitalsComponent, VitalsState);
	DOREPLIFETIME(UTunaSweeperVitalsComponent, BaseDepletionRates);
	DOREPLIFETIME(UTunaSweeperVitalsComponent, AdditionalDepletionRates);
	DOREPLIFETIME(UTunaSweeperVitalsComponent, DepletionRateMultipliers);
}

FTunaSweeperVitalsDepletionRates UTunaSweeperVitalsComponent::GetEffectiveDepletionRates() const
{
	FTunaSweeperVitalsDepletionRates EffectiveRates;
	EffectiveRates.HealthPerSecond =
		(BaseDepletionRates.HealthPerSecond + AdditionalDepletionRates.HealthPerSecond) * DepletionRateMultipliers.Health;
	EffectiveRates.FoodPerSecond =
		(BaseDepletionRates.FoodPerSecond + AdditionalDepletionRates.FoodPerSecond) * DepletionRateMultipliers.Food;
	EffectiveRates.HydrationPerSecond =
		(BaseDepletionRates.HydrationPerSecond + AdditionalDepletionRates.HydrationPerSecond) * DepletionRateMultipliers.Hydration;
	EffectiveRates.ClampNonNegative();
	return EffectiveRates;
}

void UTunaSweeperVitalsComponent::ApplyVitalsDelta(const FTunaSweeperVitalsDelta& Delta)
{
	if (HasAuthority())
	{
		ApplyVitalsDeltaInternal(Delta);
	}
	else
	{
		ServerApplyVitalsDelta(Delta);
	}
}

void UTunaSweeperVitalsComponent::ApplyConsumableVitalsEffect(const FTunaSweeperVitalsDelta& Effect)
{
	ApplyVitalsDelta(Effect);
}

void UTunaSweeperVitalsComponent::ApplyActionVitalsCost(const FTunaSweeperVitalsDelta& Cost)
{
	FTunaSweeperVitalsDelta Delta;
	Delta.Health = -FMath::Abs(Cost.Health);
	Delta.Food = -FMath::Abs(Cost.Food);
	Delta.Hydration = -FMath::Abs(Cost.Hydration);
	ApplyVitalsDelta(Delta);
}

void UTunaSweeperVitalsComponent::ServerApplyVitalsDelta_Implementation(const FTunaSweeperVitalsDelta& Delta)
{
	ApplyVitalsDeltaInternal(Delta);
}

void UTunaSweeperVitalsComponent::ServerApplyActionVitalsCost_Implementation(const FTunaSweeperVitalsDelta& Cost)
{
	FTunaSweeperVitalsDelta Delta;
	Delta.Health = -FMath::Abs(Cost.Health);
	Delta.Food = -FMath::Abs(Cost.Food);
	Delta.Hydration = -FMath::Abs(Cost.Hydration);
	ApplyVitalsDeltaInternal(Delta);
}

void UTunaSweeperVitalsComponent::SetBaseDepletionRates(const FTunaSweeperVitalsDepletionRates& NewBaseRates)
{
	if (!HasAuthority())
	{
		return;
	}

	BaseDepletionRates = NewBaseRates;
	BaseDepletionRates.ClampNonNegative();
	BroadcastVitalsChanged();
}

void UTunaSweeperVitalsComponent::SetDepletionRateAdditions(const FTunaSweeperVitalsDepletionRates& NewAdditionalRates)
{
	if (!HasAuthority())
	{
		return;
	}

	AdditionalDepletionRates = NewAdditionalRates;
	AdditionalDepletionRates.ClampNonNegative();
	BroadcastVitalsChanged();
}

void UTunaSweeperVitalsComponent::SetDepletionRateMultipliers(const FTunaSweeperVitalsDepletionMultipliers& NewMultipliers)
{
	if (!HasAuthority())
	{
		return;
	}

	DepletionRateMultipliers = NewMultipliers;
	DepletionRateMultipliers.ClampNonNegative();
	BroadcastVitalsChanged();
}

void UTunaSweeperVitalsComponent::OnRep_VitalsState()
{
	VitalsState.Normalize();
	BroadcastVitalsChanged();
}

void UTunaSweeperVitalsComponent::OnRep_DepletionSettings()
{
	BaseDepletionRates.ClampNonNegative();
	AdditionalDepletionRates.ClampNonNegative();
	DepletionRateMultipliers.ClampNonNegative();
	BroadcastVitalsChanged();
}

bool UTunaSweeperVitalsComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

void UTunaSweeperVitalsComponent::ApplyVitalsDeltaInternal(const FTunaSweeperVitalsDelta& Delta)
{
	const FTunaSweeperVitalsState PreviousState = VitalsState;
	VitalsState.Health += Delta.Health;
	VitalsState.Food += Delta.Food;
	VitalsState.Hydration += Delta.Hydration;
	VitalsState.Normalize();

	if (!FMath::IsNearlyEqual(PreviousState.Health, VitalsState.Health) ||
		!FMath::IsNearlyEqual(PreviousState.Food, VitalsState.Food) ||
		!FMath::IsNearlyEqual(PreviousState.Hydration, VitalsState.Hydration))
	{
		BroadcastVitalsChanged();
		if (AActor* Owner = GetOwner())
		{
			Owner->ForceNetUpdate();
		}
	}
}

void UTunaSweeperVitalsComponent::BroadcastVitalsChanged()
{
	OnVitalsChanged.Broadcast(VitalsState);
}
