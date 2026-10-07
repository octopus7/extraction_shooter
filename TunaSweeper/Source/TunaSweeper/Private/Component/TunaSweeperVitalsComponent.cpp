#include "Component/TunaSweeperVitalsComponent.h"
#include "Combat/TunaSweeperCombatValue.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

void FTunaSweeperVitalsState::Normalize()
{
	MaxHealth = FMath::Max(1.0f, TunaSweeperCombatValue::Round(MaxHealth));
	MaxFood = FMath::Max(1.0f, TunaSweeperCombatValue::Round(MaxFood));
	MaxHydration = FMath::Max(1.0f, TunaSweeperCombatValue::Round(MaxHydration));
	Health = TunaSweeperCombatValue::ClampGauge(Health, MaxHealth);
	Food = TunaSweeperCombatValue::ClampGauge(Food, MaxFood);
	Hydration = TunaSweeperCombatValue::ClampGauge(Hydration, MaxHydration);
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
	Delta.Health = TunaSweeperCombatValue::Accumulate(VitalsState.Health, VitalsState.MaxHealth,
		-EffectiveRates.HealthPerSecond * DeltaTime, HealthDepletionRemainder) - VitalsState.Health;
	Delta.Food = TunaSweeperCombatValue::Accumulate(VitalsState.Food, VitalsState.MaxFood,
		-EffectiveRates.FoodPerSecond * DeltaTime, FoodDepletionRemainder) - VitalsState.Food;
	Delta.Hydration = TunaSweeperCombatValue::Accumulate(VitalsState.Hydration, VitalsState.MaxHydration,
		-EffectiveRates.HydrationPerSecond * DeltaTime, HydrationDepletionRemainder) - VitalsState.Hydration;
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

void UTunaSweeperVitalsComponent::SetVitalsState(const FTunaSweeperVitalsState& NewVitalsState)
{
	if (!HasAuthority())
	{
		return;
	}

	const FTunaSweeperVitalsState PreviousState = VitalsState;
	VitalsState = NewVitalsState;
	VitalsState.Normalize();
	HealthDepletionRemainder = FoodDepletionRemainder = HydrationDepletionRemainder = 0.0;
	if (!FMath::IsNearlyEqual(PreviousState.Health, VitalsState.Health) ||
		!FMath::IsNearlyEqual(PreviousState.MaxHealth, VitalsState.MaxHealth) ||
		!FMath::IsNearlyEqual(PreviousState.Food, VitalsState.Food) ||
		!FMath::IsNearlyEqual(PreviousState.MaxFood, VitalsState.MaxFood) ||
		!FMath::IsNearlyEqual(PreviousState.Hydration, VitalsState.Hydration) ||
		!FMath::IsNearlyEqual(PreviousState.MaxHydration, VitalsState.MaxHydration))
	{
		BroadcastVitalsChanged();
		if (AActor* Owner = GetOwner())
		{
			Owner->ForceNetUpdate();
		}
	}
}

void UTunaSweeperVitalsComponent::SetMaxVitals(
	float NewMaxHealth,
	float NewMaxFood,
	float NewMaxHydration,
	bool bPreserveCurrentPercent)
{
	if (!HasAuthority())
	{
		return;
	}

	FTunaSweeperVitalsState NewState = VitalsState;
	const float HealthPercent = VitalsState.MaxHealth > 0.0f
		? FMath::Clamp(VitalsState.Health / VitalsState.MaxHealth, 0.0f, 1.0f)
		: 1.0f;
	const float FoodPercent = VitalsState.MaxFood > 0.0f
		? FMath::Clamp(VitalsState.Food / VitalsState.MaxFood, 0.0f, 1.0f)
		: 1.0f;
	const float HydrationPercent = VitalsState.MaxHydration > 0.0f
		? FMath::Clamp(VitalsState.Hydration / VitalsState.MaxHydration, 0.0f, 1.0f)
		: 1.0f;

	NewState.MaxHealth = FMath::Max(1.0f, TunaSweeperCombatValue::Round(NewMaxHealth));
	NewState.MaxFood = FMath::Max(1.0f, TunaSweeperCombatValue::Round(NewMaxFood));
	NewState.MaxHydration = FMath::Max(1.0f, TunaSweeperCombatValue::Round(NewMaxHydration));
	if (NewState.MaxHealth == VitalsState.MaxHealth && NewState.MaxFood == VitalsState.MaxFood &&
		NewState.MaxHydration == VitalsState.MaxHydration) return;
	if (bPreserveCurrentPercent)
	{
		NewState.Health = NewState.MaxHealth * HealthPercent;
		NewState.Food = NewState.MaxFood * FoodPercent;
		NewState.Hydration = NewState.MaxHydration * HydrationPercent;
	}

	SetVitalsState(NewState);
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
	VitalsState.Health += TunaSweeperCombatValue::RoundDelta(Delta.Health);
	VitalsState.Food += TunaSweeperCombatValue::RoundDelta(Delta.Food);
	VitalsState.Hydration += TunaSweeperCombatValue::RoundDelta(Delta.Hydration);
	VitalsState.Normalize();
	TunaSweeperCombatValue::DiscardOutwardRemainder(VitalsState.Health, VitalsState.MaxHealth, HealthDepletionRemainder);
	TunaSweeperCombatValue::DiscardOutwardRemainder(VitalsState.Food, VitalsState.MaxFood, FoodDepletionRemainder);
	TunaSweeperCombatValue::DiscardOutwardRemainder(VitalsState.Hydration, VitalsState.MaxHydration, HydrationDepletionRemainder);

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
