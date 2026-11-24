#include "Environment/NearbyPlayerFacingComponent.h"

#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UNearbyPlayerFacingComponent::UNearbyPlayerFacingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UNearbyPlayerFacingComponent::BeginPlay()
{
    Super::BeginPlay();
    RestYaw = GetOwner()->GetActorRotation().Yaw;
}

void UNearbyPlayerFacingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* TickFunction)
{
    Super::TickComponent(DeltaTime, TickType, TickFunction);
    AActor* Owner = GetOwner();
    const APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
    const FVector ToPlayer = Player ? Player->GetActorLocation() - Owner->GetActorLocation() : FVector::ZeroVector;
    const float Radius = bTracking ? FMath::Max(StopDistance, StartDistance) : StartDistance;
    if (Player && ToPlayer.SizeSquared2D() <= FMath::Square(Radius))
    {
        ReactionTime += DeltaTime;
        if (ReactionTime >= .28f)
        {
            bTracking = true;
            ReturnDelay = 2.f;
        }
    }
    else
    {
        bTracking = false;
        ReactionTime = 0.f;
    }

    if (!bTracking && ReturnDelay > 0.f)
    {
        ReturnDelay -= DeltaTime;
        return;
    }
    if (bTracking && ToPlayer.SizeSquared2D() < KINDA_SMALL_NUMBER) return;
    FRotator Rotation = Owner->GetActorRotation();
    const float Target = bTracking ? ToPlayer.Rotation().Yaw - ModelForwardYaw : RestYaw;
    const float Error = FMath::FindDeltaAngleDegrees(Rotation.Yaw, Target);
    if (FMath::Abs(Error) < .02f) return;
    const float Step = Error * FMath::Clamp(DeltaTime * (bTracking ? 2.8f : 1.8f), 0.f, 1.f);
    Rotation.Yaw += FMath::Clamp(Step, -90.f * DeltaTime, 90.f * DeltaTime);
    Owner->SetActorRotation(Rotation);
}
