#include "Component/TunaSweeperDebuffComponent.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Component/TunaSweeperVitalsComponent.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/TunaSweeperDebuffDataSubsystem.h"

UTunaSweeperDebuffComponent::UTunaSweeperDebuffComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	SetIsReplicatedByDefault(true);
}

void UTunaSweeperDebuffComponent::BeginPlay()
{
	Super::BeginPlay();

	NormalizeActiveDebuffs();
}

void UTunaSweeperDebuffComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!HasAuthority() || DeltaTime <= 0.0f || ActiveDebuffs.Num() <= 0)
	{
		return;
	}

	bool bChanged = false;
	const float ClampedDeltaTime = FMath::Max(0.0f, DeltaTime);
	for (FTunaSweeperActiveDebuffState& DebuffState : ActiveDebuffs)
	{
		const float PreviousRemainingSeconds = DebuffState.RemainingSeconds;
		const float EffectiveDeltaSeconds = FMath::Min(ClampedDeltaTime, PreviousRemainingSeconds);
		DebuffState.RemainingSeconds = FMath::Max(0.0f, DebuffState.RemainingSeconds - ClampedDeltaTime);
		bChanged = true;

		if (EffectiveDeltaSeconds <= 0.0f || DebuffState.TickIntervalSeconds <= 0.0f)
		{
			continue;
		}

		DebuffState.TickAccumulator += EffectiveDeltaSeconds;
		while (DebuffState.TickAccumulator + KINDA_SMALL_NUMBER >= DebuffState.TickIntervalSeconds)
		{
			DebuffState.TickAccumulator = FMath::Max(
				0.0f,
				DebuffState.TickAccumulator - DebuffState.TickIntervalSeconds);
			ApplyTickDamage(DebuffState);
		}
	}

	for (int32 DebuffIndex = ActiveDebuffs.Num() - 1; DebuffIndex >= 0; --DebuffIndex)
	{
		if (ActiveDebuffs[DebuffIndex].RemainingSeconds <= KINDA_SMALL_NUMBER)
		{
			ActiveDebuffs.RemoveAt(DebuffIndex);
			bChanged = true;
		}
	}

	if (bChanged)
	{
		MarkDebuffsChanged();
	}
}

void UTunaSweeperDebuffComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UTunaSweeperDebuffComponent, ActiveDebuffs);
}

bool UTunaSweeperDebuffComponent::HasDebuff(FName DebuffId) const
{
	return ActiveDebuffs.ContainsByPredicate(
		[DebuffId](const FTunaSweeperActiveDebuffState& DebuffState)
		{
			return DebuffState.DebuffId == DebuffId;
		});
}

bool UTunaSweeperDebuffComponent::TryApplyDebuff(
	FName DebuffId,
	float ApplyChanceBonus,
	float DurationBonusSeconds,
	AActor* SourceActor)
{
	if (DebuffId.IsNone())
	{
		return false;
	}

	if (!HasAuthority())
	{
		ServerTryApplyDebuff(DebuffId, ApplyChanceBonus, DurationBonusSeconds, SourceActor);
		return false;
	}

	return ApplyDebuffInternal(DebuffId, ApplyChanceBonus, DurationBonusSeconds, SourceActor);
}

bool UTunaSweeperDebuffComponent::RemoveDebuff(FName DebuffId)
{
	if (DebuffId.IsNone())
	{
		return false;
	}

	if (!HasAuthority())
	{
		ServerRemoveDebuff(DebuffId);
		return false;
	}

	return RemoveDebuffInternal(DebuffId);
}

int32 UTunaSweeperDebuffComponent::RemoveDebuffs(const TArray<FName>& DebuffIds)
{
	if (DebuffIds.Num() <= 0)
	{
		return 0;
	}

	if (!HasAuthority())
	{
		ServerRemoveDebuffs(DebuffIds);
		return 0;
	}

	return RemoveDebuffsInternal(DebuffIds);
}

void UTunaSweeperDebuffComponent::OnRep_ActiveDebuffs()
{
	NormalizeActiveDebuffs();
}

void UTunaSweeperDebuffComponent::ServerTryApplyDebuff_Implementation(
	FName DebuffId,
	float ApplyChanceBonus,
	float DurationBonusSeconds,
	AActor* SourceActor)
{
	ApplyDebuffInternal(DebuffId, ApplyChanceBonus, DurationBonusSeconds, SourceActor);
}

void UTunaSweeperDebuffComponent::ServerRemoveDebuff_Implementation(FName DebuffId)
{
	RemoveDebuffInternal(DebuffId);
}

void UTunaSweeperDebuffComponent::ServerRemoveDebuffs_Implementation(const TArray<FName>& DebuffIds)
{
	RemoveDebuffsInternal(DebuffIds);
}

bool UTunaSweeperDebuffComponent::HasAuthority() const
{
	const AActor* Owner = GetOwner();
	return Owner && Owner->HasAuthority();
}

bool UTunaSweeperDebuffComponent::ApplyDebuffInternal(
	FName DebuffId,
	float ApplyChanceBonus,
	float DurationBonusSeconds,
	AActor* SourceActor)
{
	if (DebuffId.IsNone())
	{
		return false;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperDebuffDataSubsystem>()
		: nullptr;

	FTunaSweeperDebuffDefinition Definition;
	if (!DebuffDataSubsystem || !DebuffDataSubsystem->TryGetDebuffDefinition(DebuffId, Definition))
	{
		return false;
	}

	const float ApplyChance = FMath::Clamp(
		Definition.BaseApplyChance + FMath::Max(0.0f, ApplyChanceBonus),
		0.0f,
		1.0f);
	if (ApplyChance <= 0.0f || FMath::FRand() > ApplyChance)
	{
		return false;
	}

	(void)SourceActor;

	const float DurationSeconds = FMath::Max(
		0.01f,
		Definition.DurationSeconds + FMath::Max(0.0f, DurationBonusSeconds));

	if (FTunaSweeperActiveDebuffState* ExistingDebuff = ActiveDebuffs.FindByPredicate(
		[DebuffId](const FTunaSweeperActiveDebuffState& DebuffState)
		{
			return DebuffState.DebuffId == DebuffId;
		}))
	{
		ExistingDebuff->DurationSeconds = DurationSeconds;
		ExistingDebuff->RemainingSeconds = DurationSeconds;
		ExistingDebuff->TickIntervalSeconds = Definition.TickIntervalSeconds;
		ExistingDebuff->DamagePerTick = Definition.DamagePerTick;
		ExistingDebuff->Normalize();
		MarkDebuffsChanged();
		return true;
	}

	FTunaSweeperActiveDebuffState NewDebuff;
	NewDebuff.DebuffId = DebuffId;
	NewDebuff.DurationSeconds = DurationSeconds;
	NewDebuff.RemainingSeconds = DurationSeconds;
	NewDebuff.TickIntervalSeconds = Definition.TickIntervalSeconds;
	NewDebuff.DamagePerTick = Definition.DamagePerTick;
	NewDebuff.TickAccumulator = 0.0f;
	NewDebuff.AppliedOrder = NextAppliedOrder++;
	NewDebuff.Normalize();
	ActiveDebuffs.Add(NewDebuff);
	MarkDebuffsChanged();
	return true;
}

bool UTunaSweeperDebuffComponent::RemoveDebuffInternal(FName DebuffId)
{
	const int32 RemovedCount = ActiveDebuffs.RemoveAll(
		[DebuffId](const FTunaSweeperActiveDebuffState& DebuffState)
		{
			return DebuffState.DebuffId == DebuffId;
		});

	if (RemovedCount <= 0)
	{
		return false;
	}

	MarkDebuffsChanged();
	return true;
}

int32 UTunaSweeperDebuffComponent::RemoveDebuffsInternal(const TArray<FName>& DebuffIds)
{
	int32 RemovedCount = 0;
	for (const FName DebuffId : DebuffIds)
	{
		if (DebuffId.IsNone())
		{
			continue;
		}

		RemovedCount += ActiveDebuffs.RemoveAll(
			[DebuffId](const FTunaSweeperActiveDebuffState& DebuffState)
			{
				return DebuffState.DebuffId == DebuffId;
			});
	}

	if (RemovedCount > 0)
	{
		MarkDebuffsChanged();
	}

	return RemovedCount;
}

void UTunaSweeperDebuffComponent::ApplyTickDamage(const FTunaSweeperActiveDebuffState& DebuffState)
{
	if (DebuffState.DamagePerTick <= 0.0f)
	{
		return;
	}

	UTunaSweeperVitalsComponent* VitalsComponent = GetOwner()
		? GetOwner()->FindComponentByClass<UTunaSweeperVitalsComponent>()
		: nullptr;
	if (!VitalsComponent || VitalsComponent->GetVitalsState().Health <= 0.0f)
	{
		return;
	}

	const float PreviousHealth = VitalsComponent->GetVitalsState().Health;
	FTunaSweeperVitalsDelta DamageDelta;
	DamageDelta.Health = -DebuffState.DamagePerTick;
	VitalsComponent->ApplyVitalsDelta(DamageDelta);
	if (VitalsComponent->GetVitalsState().Health >= PreviousHealth)
	{
		return;
	}

	UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	UTunaSweeperDebuffDataSubsystem* DebuffDataSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperDebuffDataSubsystem>()
		: nullptr;

	FTunaSweeperDebuffDefinition Definition;
	if (DebuffDataSubsystem &&
		DebuffDataSubsystem->TryGetDebuffDefinition(DebuffState.DebuffId, Definition))
	{
		if (ATunaSweeperTopDownCharacter* TunaCharacter = Cast<ATunaSweeperTopDownCharacter>(GetOwner()))
		{
			TunaCharacter->TriggerDebuffCameraReaction(DebuffState.DebuffId, Definition.CameraReaction);
		}
	}
}

void UTunaSweeperDebuffComponent::NormalizeActiveDebuffs()
{
	for (FTunaSweeperActiveDebuffState& DebuffState : ActiveDebuffs)
	{
		DebuffState.Normalize();
	}

	ActiveDebuffs.Sort(
		[](const FTunaSweeperActiveDebuffState& Left, const FTunaSweeperActiveDebuffState& Right)
		{
			return Left.AppliedOrder < Right.AppliedOrder;
		});
}

void UTunaSweeperDebuffComponent::MarkDebuffsChanged()
{
	NormalizeActiveDebuffs();
	if (AActor* Owner = GetOwner())
	{
		Owner->ForceNetUpdate();
	}
}
