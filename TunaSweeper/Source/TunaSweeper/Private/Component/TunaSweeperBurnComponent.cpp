#include "Component/TunaSweeperBurnComponent.h"

#include "AI/TunaSweeperEnemyCharacter.h"
#include "Component/TunaSweeperFactionComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "UI/TunaSweeperGameHudWidget.h"

UTunaSweeperBurnComponent::UTunaSweeperBurnComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetIsReplicatedByDefault(true);
	BurningEffect = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(
		TEXT("/Game/Effects/NS_ExplosiveBarrel_Burning.NS_ExplosiveBarrel_Burning")));
}

void UTunaSweeperBurnComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshBurningEffect();
}

void UTunaSweeperBurnComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBurn();
	Super::EndPlay(EndPlayReason);
}

void UTunaSweeperBurnComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UTunaSweeperBurnComponent, bIsBurning);
}

bool UTunaSweeperBurnComponent::CanBurnActor(const AActor* Actor)
{
	const ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(Actor);
	if (!IsValid(Enemy) || Enemy->IsActorBeingDestroyed() || Enemy->IsDead())
	{
		return false;
	}
	const UTunaSweeperFactionComponent* Faction = Enemy->GetFactionComponent();
	const UWorld* World = Enemy->GetWorld();
	const UTunaSweeperFactionSubsystem* Factions = World
		? World->GetSubsystem<UTunaSweeperFactionSubsystem>() : nullptr;
	return Faction && Factions && Faction->CanBeCombatTarget() &&
		Factions->GetFactionAttitudeById(TunaSweeperFactionIds::Player, Faction->GetFactionId()) ==
			ETunaSweeperFactionAttitude::Hostile;
}

bool UTunaSweeperBurnComponent::CanApplyFromSource(const AActor* SourceActor) const
{
	const UWorld* World = GetWorld();
	const UTunaSweeperFactionSubsystem* Factions = World
		? World->GetSubsystem<UTunaSweeperFactionSubsystem>() : nullptr;
	return !Factions || Factions->CanApplyCombatEffect(SourceActor, GetOwner());
}

bool UTunaSweeperBurnComponent::TryApplyBurn(
	const FTunaSweeperBurnSpec& BurnSpec, AController* EventInstigator, AActor* DamageCauser)
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->HasAuthority() || !CanBurnActor(Owner))
	{
		return false;
	}
	FTunaSweeperBurnSpec SafeSpec = BurnSpec;
	SafeSpec.Normalize();
	if (!SafeSpec.bEnabled)
	{
		return false;
	}

	AController* SourceController = IsValid(EventInstigator) ? EventInstigator :
		(IsValid(DamageCauser) ? DamageCauser->GetInstigatorController() : nullptr);
	// Projectiles are destroyed on impact. Keep the firing pawn and controller for future kill credit.
	AActor* SourceActor = SourceController ? SourceController->GetPawn() : nullptr;
	if (!IsValid(SourceActor) && IsValid(DamageCauser))
	{
		SourceActor = DamageCauser->GetInstigator();
		if (!IsValid(SourceActor)) SourceActor = DamageCauser->GetOwner();
		if (!IsValid(SourceActor)) SourceActor = DamageCauser;
	}
	if (!CanApplyFromSource(SourceActor))
	{
		return false;
	}

	const float IncomingDamage = SafeSpec.GetDamagePerTick();
	if (!bIsBurning || IncomingDamage >= ActiveDamagePerTick)
	{
		ActiveDamagePerTick = IncomingDamage;
		BurnInstigator = SourceController;
		BurnDamageSource = SourceActor;
	}
	if (!bIsBurning)
	{
		TickAccumulator = 0.0;
	}
	// Refresh the finite duration without delaying the next scheduled tick or weakening an existing burn.
	RemainingSeconds = FMath::Max(RemainingSeconds, static_cast<double>(SafeSpec.GetDurationSeconds()));
	bIsBurning = true;
	SetComponentTickEnabled(true);
	RefreshBurningEffect();
	Owner->ForceNetUpdate();
	return true;
}

void UTunaSweeperBurnComponent::ClearBurn()
{
	const bool bWasBurning = bIsBurning;
	bIsBurning = false;
	RemainingSeconds = 0.0;
	TickAccumulator = 0.0;
	ActiveDamagePerTick = 0.0f;
	BurnInstigator.Reset();
	BurnDamageSource.Reset();
	SetComponentTickEnabled(false);
	StopBurningEffect();
	if (bWasBurning && GetOwner() && GetOwner()->HasAuthority())
	{
		GetOwner()->ForceNetUpdate();
	}
}

AActor* UTunaSweeperBurnComponent::ResolveDamageSource() const
{
	if (AActor* Source = BurnDamageSource.Get())
	{
		return Source;
	}
	return BurnInstigator.IsValid() ? BurnInstigator->GetPawn() : nullptr;
}

void UTunaSweeperBurnComponent::TickComponent(
	float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bIsBurning || !GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!CanBurnActor(GetOwner()) || !CanApplyFromSource(ResolveDamageSource()))
	{
		ClearBurn();
		return;
	}
	if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.0f)
	{
		return;
	}

	const double EffectiveDelta = FMath::Min(static_cast<double>(DeltaTime), RemainingSeconds);
	RemainingSeconds = FMath::Max(0.0, RemainingSeconds - EffectiveDelta);
	TickAccumulator += EffectiveDelta;
	// A long frame consumes only time inside the finite lifetime and still delivers the final tick.
	while (bIsBurning && TickAccumulator + UE_SMALL_NUMBER >= FTunaSweeperBurnSpec::TickIntervalSeconds)
	{
		TickAccumulator = FMath::Max(0.0, TickAccumulator - FTunaSweeperBurnSpec::TickIntervalSeconds);
		ApplyBurnTick();
		// TakeDamage can synchronously kill the owner and call ClearBurn, resetting all state.
		if (!CanBurnActor(GetOwner()))
		{
			ClearBurn();
			return;
		}
	}
	if (bIsBurning && RemainingSeconds <= UE_SMALL_NUMBER)
	{
		ClearBurn();
	}
}

void UTunaSweeperBurnComponent::ApplyBurnTick()
{
	ATunaSweeperEnemyCharacter* Enemy = Cast<ATunaSweeperEnemyCharacter>(GetOwner());
	if (!Enemy || !CanApplyFromSource(ResolveDamageSource()))
	{
		ClearBurn();
		return;
	}
	AController* Instigator = BurnInstigator.Get();
	const bool bShowDamageNumber = !Enemy->IsTemporaryVideoBulletStormEnabled();
	FVector BoundsOrigin;
	FVector BoundsExtent;
	Enemy->GetActorBounds(false, BoundsOrigin, BoundsExtent);
	const FVector DamageNumberLocation = BoundsOrigin + FVector(0.0f, 0.0f, BoundsExtent.Z + 34.0f);
	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		Enemy, ActiveDamagePerTick, Instigator, ResolveDamageSource(), UDamageType::StaticClass());
	if (AppliedDamage > 0.0f && bShowDamageNumber)
	{
		if (ATunaSweeperPlayerController* PlayerController = Cast<ATunaSweeperPlayerController>(Instigator);
			PlayerController && PlayerController->IsLocalController())
		{
			if (UTunaSweeperGameHudWidget* Hud = PlayerController->GetGameHudWidget())
			{
				Hud->ShowDamageNumber(AppliedDamage, DamageNumberLocation, ETunaSweeperDamageNumberType::Normal);
			}
		}
	}
}

void UTunaSweeperBurnComponent::OnRep_IsBurning()
{
	RefreshBurningEffect();
}

void UTunaSweeperBurnComponent::RefreshBurningEffect()
{
	if (!bIsBurning)
	{
		StopBurningEffect();
		return;
	}
	UWorld* World = GetWorld();
	if (BurningEffectComponent || !World || !World->HasBegunPlay() || World->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}
	AActor* Owner = GetOwner();
	UNiagaraSystem* Effect = BurningEffect.LoadSynchronous();
	if (!Owner || !Owner->GetRootComponent() || !Effect)
	{
		return;
	}
	BurningEffectComponent = NewObject<UNiagaraComponent>(Owner);
	Owner->AddInstanceComponent(BurningEffectComponent);
	BurningEffectComponent->SetAutoActivate(false);
	BurningEffectComponent->SetAutoDestroy(false);
	BurningEffectComponent->SetAsset(Effect);
	BurningEffectComponent->SetupAttachment(Owner->GetRootComponent());
	// Flames follow the enemy's position but always rise along the world's up direction.
	BurningEffectComponent->SetAbsolute(false, true, true);
	BurningEffectComponent->SetRelativeLocation(BurningEffectOffset);
	BurningEffectComponent->SetWorldRotation(FRotator::ZeroRotator);
	const float SafeScale = FMath::IsFinite(BurningEffectScale) ? FMath::Clamp(BurningEffectScale, 0.01f, 10.0f) : 0.65f;
	BurningEffectComponent->SetWorldScale3D(FVector(SafeScale));
	BurningEffectComponent->SetVariableFloat(TEXT("User.Fade"), 1.0f);
	BurningEffectComponent->RegisterComponent();
	BurningEffectComponent->Activate(true);
}

void UTunaSweeperBurnComponent::StopBurningEffect()
{
	if (BurningEffectComponent)
	{
		BurningEffectComponent->DeactivateImmediate();
		BurningEffectComponent->DestroyComponent();
		BurningEffectComponent = nullptr;
	}
}
