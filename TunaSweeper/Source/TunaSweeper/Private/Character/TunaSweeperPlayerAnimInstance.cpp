#include "Character/TunaSweeperPlayerAnimInstance.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

void UTunaSweeperPlayerAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const auto* Player = Cast<ATunaSweeperTopDownCharacter>(TryGetPawnOwner());
	bUseUnarmedLocomotion = Player && !Player->GetEquippedWeapon();
	if (!bUseUnarmedLocomotion)
	{
		UnarmedGait = 0.f;
		return;
	}

	const FVector LocalVelocity = FRotator(0.f, Player->GetActorRotation().Yaw, 0.f).UnrotateVector(Player->GetVelocity());
	const float Speed = LocalVelocity.Size2D();
	if (Speed > 1.f)
	{
		TravelAngle = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}

	const float TopSpeed = FMath::Max(Player->GetCharacterMovement()->MaxWalkSpeed, 1.f);
	const float Gait = FMath::Clamp(Speed / TopSpeed, 0.f, 1.f) * (Player->IsSprinting() ? 2.f : 1.f);
	UnarmedGait = FMath::FInterpTo(UnarmedGait, Gait, DeltaSeconds, 14.f);
}
