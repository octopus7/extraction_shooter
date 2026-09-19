#include "Component/TunaSweeperCombatLabAutopilotComponent.h"
#include "AI/TunaSweeperEnemyCharacter.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Combat/TunaSweeperCombatLabDecision.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Game/TunaSweeperCombatLabGameMode.h"
#include "InputActionValue.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "Weapon/TunaSweeperProjectile.h"

UTunaSweeperCombatLabAutopilotComponent::UTunaSweeperCombatLabAutopilotComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}
void UTunaSweeperCombatLabAutopilotComponent::StopActions()
{
	if (auto* Player = Cast<ATunaSweeperTopDownCharacter>(GetOwner()))
	{
		Player->EndFire(FInputActionValue(false));
		Player->EndAim(FInputActionValue(false));
		Player->HandleMoveStopped(FInputActionValue(FVector2D::ZeroVector));
	}
	bFiring = false;
}
void UTunaSweeperCombatLabAutopilotComponent::SetAutopilotEnabled(bool bNewEnabled)
{
	auto* Player = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
	bEnabled = bNewEnabled && Player && GetWorld() && GetWorld()->GetAuthGameMode<ATunaSweeperCombatLabGameMode>();
	if (Player) Player->bExternalAimControl = bEnabled;
	StopActions();
	Target.Reset(); Path.Reset(); NextDecision = NextPath = NextBurst = 0;
	StateKey = bEnabled ? TEXT("ui.combat_lab.seek") : TEXT("ui.combat_lab.manual");
}
void UTunaSweeperCombatLabAutopilotComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	SetAutopilotEnabled(false);
	Super::EndPlay(Reason);
}
bool UTunaSweeperCombatLabAutopilotComponent::CanSee(const FVector& From, ATunaSweeperEnemyCharacter* Enemy) const
{
	FCollisionQueryParams Params(SCENE_QUERY_STAT(CombatLabSight), false, GetOwner());
	TArray<AActor*> Attached; GetOwner()->GetAttachedActors(Attached);
	Params.AddIgnoredActors(Attached);
	FHitResult Hit;
	return !GetWorld()->LineTraceSingleByChannel(Hit, From, Enemy->GetActorLocation(), ECC_Visibility, Params) || Hit.GetActor() == Enemy;
}
FVector UTunaSweeperCombatLabAutopilotComponent::Navigate(const FVector& Goal)
{
	const FVector Here = GetOwner()->GetActorLocation();
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now >= NextPath || FVector::DistSquared2D(Goal, LastGoal) > FMath::Square(250.f))
	{
		NextPath = Now + 0.65f; LastGoal = Goal; Path.Reset(); PathIndex = 1;
		if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
		{
			FNavLocation Projected;
			if (Nav->ProjectPointToNavigation(Goal, Projected, FVector(180,180,250)))
				if (UNavigationPath* Result = Nav->FindPathToLocationSynchronously(GetWorld(), Here, Projected.Location, GetOwner()))
					if (Result->IsValid()) Path = Result->PathPoints;
		}
	}
	while (Path.IsValidIndex(PathIndex) && FVector::DistSquared2D(Here, Path[PathIndex]) < FMath::Square(65.f)) ++PathIndex;
	return Path.IsValidIndex(PathIndex) ? (Path[PathIndex] - Here).GetSafeNormal2D() : FVector::ZeroVector;
}
void UTunaSweeperCombatLabAutopilotComponent::TickComponent(float Dt, ELevelTick TickType, FActorComponentTickFunction* Function)
{
	Super::TickComponent(Dt, TickType, Function);
	if (!bEnabled) return;
	auto* Player = Cast<ATunaSweeperTopDownCharacter>(GetOwner());
	auto* Mode = GetWorld()->GetAuthGameMode<ATunaSweeperCombatLabGameMode>();
	if (!Player || Player->IsDead() || !Mode || !Mode->IsRoundActive() || Player->IsGameplayActionInputLocked()) { StopActions(); return; }
	const float Now = GetWorld()->GetTimeSeconds();
	const FVector Here = Player->GetActorLocation();
	if (Now >= NextDecision)
	{
		NextDecision = Now + 0.18f;
		ATunaSweeperEnemyCharacter* Best = nullptr;
		float Score = TNumericLimits<float>::Max();
		for (auto* Enemy : Mode->GetAliveEnemies())
		{
			const float Distance = FVector::Dist2D(Here, Enemy->GetActorLocation());
			if (Distance > 1800 || !CanSee(Here, Enemy)) continue;
			const float Candidate = Distance - (Enemy == Target.Get() ? 250.f : 0.f);
			if (Candidate < Score) { Score = Candidate; Best = Enemy; }
		}
		if (Best != Target.Get()) { StopActions(); NextBurst = Now + 0.22f; }
		Target = Best;
	}
	FVector Move = FVector::ZeroVector;
	auto* Enemy = Target.Get();
	const bool bVisible = Enemy && !Enemy->IsDead() && CanSee(Here, Enemy);
	auto* GI = Player->GetGameInstance<UTunaSweeperGameInstance>();
	const bool bNeedsReload = GI && GI->GetWeaponLoadedAmmoCount(Player->GetSelectedWeaponSlotNumber()) == 0;
	if (!bVisible)
	{
		StopActions();
		// Explore known arena lanes. Hidden enemy positions are never used for aiming.
		const FVector Patrol[] = { FVector(100,0,100), FVector(650,-950,100), FVector(650,950,100), FVector(-700,800,100) };
		Move = Navigate(Patrol[(int32(Now / 7.f) + Mode->GetRoundNumber()) % 4]);
		StateKey = TEXT("ui.combat_lab.seek");
	}
	else
	{
		const FVector To = Enemy->GetActorLocation() - Here;
		const float Distance = To.Size2D();
		const FVector Direction = To.GetSafeNormal2D();
		StrafeSign = ((int32(Now / 3.5f) + Mode->GetRoundNumber()) % 2) ? 1.f : -1.f;
		const FVector Side(-Direction.Y * StrafeSign, Direction.X * StrafeSign, 0);
		FVector Goal = Here + Side * 340.f;
		if (Distance > 1000) Goal += Direction * 380.f;
		if (Distance < 650 || Player->IsWeaponReloading()) Goal -= Direction * 330.f;
		Goal.X = FMath::Clamp(Goal.X, -1400.f, 1400.f); Goal.Y = FMath::Clamp(Goal.Y, -1100.f, 1100.f);
		Move = Navigate(Goal);
		const FVector AimPoint = Enemy->GetActorLocation() + Enemy->GetVelocity() * 0.06f;
		FHitResult AimHit(Enemy, Enemy->GetCapsuleComponent(), AimPoint, -Direction);
		Player->SetAimWorldHit(AimPoint, AimHit);
		Player->BeginAim(FInputActionValue(true));
		StateKey = TEXT("ui.combat_lab.engage");
		if (!bNeedsReload && !Player->IsWeaponReloading() && !Player->IsRolling() && Now >= NextBurst)
		{
			bFiring = !bFiring;
			if (bFiring) Player->BeginFire(FInputActionValue(true)); else Player->EndFire(FInputActionValue(false));
			NextBurst = Now + (bFiring ? 0.48f : 0.38f);
		}
	}
	if (bNeedsReload || Player->IsWeaponReloading())
	{
		if (bFiring) { Player->EndFire(FInputActionValue(false)); bFiring = false; }
		if (!Player->IsWeaponReloading() && !Player->IsRolling()) Player->HandleReload(FInputActionValue(true));
		StateKey = TEXT("ui.combat_lab.reload");
		NextBurst = Now + 0.25f;
	}
	bool bDodge = false;
	if (Now >= NextRoll && !Player->IsRolling() && !Player->IsWeaponReloading() && Player->GetStamina() >= 40.f)
	{
		for (TActorIterator<ATunaSweeperProjectile> It(GetWorld()); It; ++It)
		{
			if (It->GetInstigator() == Player || !Cast<ATunaSweeperEnemyCharacter>(It->GetInstigator())) continue;
			const FVector Relative = It->GetActorLocation() - Here;
			if (Relative.SizeSquared2D() > FMath::Square(900.f) || FMath::Abs(Relative.Z) > 150.f) continue;
			const FVector Velocity = It->GetVelocity() - Player->GetVelocity();
			if (TunaSweeperCombatLab::IsIncomingThreat(Relative, Velocity))
			{
				Move = FVector(-Velocity.Y, Velocity.X, 0).GetSafeNormal() * StrafeSign;
				bDodge = true; break;
			}
		}
	}
	Player->HandleMove(FInputActionValue(FVector2D(Move.Y, Move.X)));
	if (bDodge)
	{
		Player->BeginRoll(FInputActionValue(true));
		if (Player->IsRolling()) { ++RollCount; NextRoll = Now + 1.6f; }
	}
	if (Player->IsRolling()) StateKey = TEXT("ui.combat_lab.dodge");
}
