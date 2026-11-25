#include "Game/VillageTunnelPlayerStart.h"
#include "Camera/CameraTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/TunaSweeperPlayerController.h"

AVillageTunnelPlayerStart::AVillageTunnelPlayerStart()
{
    ArrivalGroup = ETunaSweeperRaidArrivalGroup::VillageTunnel;
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = false;
}

void AVillageTunnelPlayerStart::BeginArrival(ATunaSweeperPlayerController* Player)
{
    ACharacter* Character = Player ? Player->GetCharacter() : nullptr;
    if (!Character) return;
    Controller = Player;
    Walker = Character;
    Origin = Character->GetActorLocation();
    Elapsed = 0;
    bAirborne = bLaunched = false;
    bInputLocked = false;
    Player->SetViewTarget(this);
    SetActorTickEnabled(true);
}

void AVillageTunnelPlayerStart::CalcCamera(float DeltaSeconds, FMinimalViewInfo& View)
{
    const FVector Focus = GetActorLocation() + GetActorForwardVector()*345.f - FVector(0,0,70);
    View.Location = Focus + GetActorForwardVector()*610.f + GetActorRightVector()*830.f + FVector(0,0,675);
    View.Rotation = (Focus - View.Location).Rotation();
    View.FOV = 58.f;
}

void AVillageTunnelPlayerStart::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    ACharacter* Character = Walker.Get();
    if (!Character || !Controller.IsValid() || Controller->GetPawn() != Character)
    {
        FinishArrival();
        return;
    }
    if (!bInputLocked)
    {
        Controller->SetEndingInputLock(true);
        Controller->DisableInput(Controller.Get());
        Character->DisableInput(Controller.Get());
        bInputLocked = true;
    }
    Elapsed += DeltaSeconds;
    const FVector Direction = GetActorForwardVector();
    if (!bLaunched)
    {
        Character->SetActorRotation(Direction.Rotation());
        if (Elapsed > 0.35f)
        {
            const float MaxSpeed = Character->GetCharacterMovement()->MaxWalkSpeed;
            Character->AddMovementInput(Direction, FMath::Clamp(210.f / FMath::Max(MaxSpeed, 1.f), 0.f, 1.f), true);
        }
        if (FVector::DotProduct(Character->GetActorLocation() - Origin, Direction) >= 330.f)
        {
            Character->LaunchCharacter(Direction * 405.f + FVector(0,0,245), true, true);
            bLaunched = true;
        }
    }
    else
    {
        bAirborne |= Character->GetCharacterMovement()->IsFalling();
        if (bAirborne && Character->GetCharacterMovement()->IsMovingOnGround())
        {
            FinishArrival();
            return;
        }
    }
    if (Elapsed > 7.f) FinishArrival();
}

void AVillageTunnelPlayerStart::FinishArrival()
{
    SetActorTickEnabled(false);

    if (Controller.IsValid())
    {
        if (APawn* Pawn = Controller->GetPawn()) Controller->SetViewTargetWithBlend(Pawn, 0.3f);
        if (Walker.IsValid()) Walker->EnableInput(Controller.Get());
        Controller->EnableInput(Controller.Get());
        Controller->SetEndingInputLock(false);
    }
    Controller.Reset();
    Walker.Reset();
}

void AVillageTunnelPlayerStart::EndPlay(EEndPlayReason::Type Reason)
{
    if (Controller.IsValid()) FinishArrival();
    Super::EndPlay(Reason);
}
