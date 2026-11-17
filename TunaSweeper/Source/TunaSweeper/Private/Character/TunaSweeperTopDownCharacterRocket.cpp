#include "TunaSweeperTopDownCharacterShared.h"
#include "Subsystem/TunaSweeperFactionSubsystem.h"
#include "Weapon/TunaSweeperTopAttackRocket.h"

bool ATunaSweeperTopDownCharacter::IsRocketTargetLocked() const
{
	return bRocketAiming && RocketLockElapsed >= FMath::Max(0.01f, RocketLockSeconds) &&
		ATunaSweeperTopAttackRocket::IsValidTarget(this, RocketAimTarget.Get());
}

float ATunaSweeperTopDownCharacter::GetRocketLockProgress() const
{
	return FMath::Clamp(RocketLockElapsed / FMath::Max(0.01f, RocketLockSeconds), 0.0f, 1.0f);
}

void ATunaSweeperTopDownCharacter::UpdateRocketAim(float DeltaSeconds)
{
	if (!bRocketAiming) return;
	UTunaSweeperGameInstance* Game = GetGameInstance<UTunaSweeperGameInstance>();
	FGuid CurrentUid;
	if (bIsDead || bIsRolling || IsGameplayActionInputLocked() || !Game ||
		!Game->TryGetSlotItemUid(PendingItemUseSlotReference, CurrentUid) || CurrentUid != PendingItemUseUid ||
		!Game->CanUseItemInSlot(PendingItemUseSlotReference, this))
	{
		CancelItemUse();
		return;
	}

	auto* Player = Cast<ATunaSweeperPlayerController>(GetController());
	if (!Player) { CancelItemUse(); return; }
	Player->TryGetCursorWorldPointOnPlane(GetActorLocation().Z, RocketAimLocation);
	FHitResult CursorHit;
	Player->GetHitResultUnderCursor(ECC_Visibility, false, CursorHit);
	if (CursorHit.bBlockingHit) RocketAimLocation = CursorHit.ImpactPoint;

	float MouseX = 0.0f, MouseY = 0.0f;
	FVector RayOrigin, RayDirection;
	AActor* Candidate = nullptr;
	if (Player->GetMousePosition(MouseX, MouseY) && Player->DeprojectMousePositionToWorld(RayOrigin, RayDirection))
	{
		TArray<AActor*> Enemies;
		if (const auto* Factions = GetWorld()->GetSubsystem<UTunaSweeperFactionSubsystem>())
			Factions->GetActorsWithAttitude(this, ETunaSweeperFactionAttitude::Hostile, Enemies);
		float BestDistance = FMath::Square(FMath::Max(1.0f, RocketLockRadiusPixels));
		for (AActor* Enemy : Enemies)
		{
			if (!ATunaSweeperTopAttackRocket::IsValidTarget(this, Enemy) ||
				FVector::DistSquared(GetActorLocation(), Enemy->GetActorLocation()) > FMath::Square(RocketLockRange)) continue;
			FVector Center, Extent;
			Enemy->GetActorBounds(true, Center, Extent);
			FVector2D Screen;
			if (!Player->ProjectWorldLocationToScreen(Center, Screen)) continue;
			const float Distance = CursorHit.GetActor() == Enemy ? 0.0f : FVector2D::DistSquared(Screen, FVector2D(MouseX, MouseY));
			if (Distance > BestDistance) continue;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(RocketAimSight), false, this);
			FHitResult SightHit;
			if (GetWorld()->LineTraceSingleByChannel(SightHit, RayOrigin, Center, ECC_Visibility, Params) && SightHit.GetActor() != Enemy) continue;
			BestDistance = Distance;
			Candidate = Enemy;
		}
	}
	if (Candidate != RocketAimTarget.Get())
	{
		RocketAimTarget = Candidate;
		RocketLockElapsed = 0.0f;
	}
	if (Candidate)
		RocketLockElapsed = FMath::Min(RocketLockElapsed + FMath::Max(0.0f, DeltaSeconds), FMath::Max(0.01f, RocketLockSeconds));
	else
		RocketLockElapsed = 0.0f;
}

void ATunaSweeperTopDownCharacter::FireAimedRocket()
{
	// Revalidate the slot and cursor at the click, including input state changes since the last tick.
	UpdateRocketAim(0.0f);
	if (!bRocketAiming) return;
	auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
	AActor* LockedTarget = IsRocketTargetLocked() ? RocketAimTarget.Get() : nullptr;
	if (Game && Game->TryLaunchRocketFromSlot(PendingItemUseSlotReference, PendingItemUseUid, this, LockedTarget, RocketAimLocation))
		CancelItemUse();
}
