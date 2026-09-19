#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Character/TunaSweeperTopDownCharacter.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UI/TunaSweeperVehicleDismountWidget.h"

UTunaSweeperVehicleMountComponent::UTunaSweeperVehicleMountComponent()
{
	InteractionType = ETunaSweeperInteractionType::VehicleMount;
	InteractionDisplayName = FText::GetEmpty();
	InteractionDisplayNameStringKey = TEXT("ui.vehicle.mount");
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

bool UTunaSweeperVehicleMountComponent::CanMount(const ATunaSweeperTopDownCharacter* Character) const
{
	if (!IsValid(Character) || !IsValid(GetOwner()) || GetOwner()->IsActorBeingDestroyed() || Rider.IsValid() ||
		Character->IsDead() || Character->IsMountedInVehicle() || Character->GetAttachParentActor() ||
		!IsWithinInteractionDistance(Character) || FMath::Abs(Character->GetActorLocation().Z - GetComponentLocation().Z) > InteractionDistance)
	{
		return false;
	}
	const auto* Controller = Cast<ATunaSweeperPlayerController>(Character->GetController());
	return !Controller || (!Controller->IsInventoryUiOpen() && !Controller->IsPauseMenuOpen() &&
		!Controller->IsDialogueSequenceActive() && !Controller->IsHousingModeOpen());
}

bool UTunaSweeperVehicleMountComponent::TryMount(ATunaSweeperTopDownCharacter* Character)
{
	if (!CanMount(Character)) return false;
	Character->CancelActiveGameplayActions();
	auto* Movement = Character->GetCharacterMovement();
	SavedMovementMode = Movement->MovementMode;
	SavedCustomMovementMode = Movement->CustomMovementMode;
	SavedCapsuleCollision = Character->GetCapsuleComponent()->GetCollisionEnabled();
	LastOnFootTransform = Character->GetActorTransform();
	if (!Character->AttachToComponent(this, FAttachmentTransformRules::SnapToTargetNotIncludingScale)) return false;
	Rider = Character;
	Character->VehicleMount = this;
	Movement->DisableMovement();
	// Keep query collision so projectiles can still hit the rider.
	Character->GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Character->GetCapsuleComponent()->IgnoreActorWhenMoving(GetOwner(), true);
	Character->SetActorRelativeLocation(RiderRelativeTransform.GetLocation());
	Character->SetActorRelativeRotation(RiderRelativeTransform.GetRotation());
	StationarySeconds = 0;
	bHintVisible = false;
	PreviousVehicleLocation = GetComponentLocation();
	if (auto* Controller = Cast<APlayerController>(Character->GetController()); Controller && Controller->IsLocalController() && Controller->GetLocalPlayer())
	{
		DismountWidget = CreateWidget<UTunaSweeperVehicleDismountWidget>(Controller);
		if (DismountWidget)
		{
			DismountWidget->AddToPlayerScreen(20);
			DismountWidget->SetAnchorsInViewport(FAnchors(0.5f, 0.82f));
			DismountWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
			DismountWidget->SetDesiredSizeInViewport(FVector2D(180, 36));
			DismountWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	StopEngineAudio();
	if (EngineStartSound)
	{
		EngineAudio = UGameplayStatics::SpawnSoundAttached(EngineStartSound, this);
		GetWorld()->GetTimerManager().SetTimer(EngineStartTimer, this,
			&UTunaSweeperVehicleMountComponent::StartIdleAudio, FMath::Max(0.01f, EngineStartSound->GetDuration()), false);
	}
	else StartIdleAudio();
	OnMounted.Broadcast(Character);
	return true;
}

bool UTunaSweeperVehicleMountComponent::FindDismountLocation(FVector& OutLocation) const
{
	const auto* Character = Rider.Get();
	const UWorld* World = GetWorld();
	if (!Character || !World) return false;
	const auto* Capsule = Character->GetCapsuleComponent();
	const float Radius = Capsule->GetScaledCapsuleRadius();
	const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
	FCollisionQueryParams GroundParams(SCENE_QUERY_STAT(VehicleDismountGround), false, Character);
	GroundParams.AddIgnoredActor(GetOwner());
	FCollisionQueryParams ClearanceParams(SCENE_QUERY_STAT(VehicleDismountClearance), false, Character);
	// Try both sides and the rear; never teleport through a wall to an otherwise empty exit.
	const FVector Origin = GetComponentLocation();
	const FRotator YawRotation(0, GetOwner()->GetActorRotation().Yaw, 0);
	for (const float Angle : {90.0f, -90.0f, 180.0f, 45.0f, -45.0f, 135.0f, -135.0f})
	{
		const FVector Offset = YawRotation.RotateVector(FVector::ForwardVector.RotateAngleAxis(Angle, FVector::UpVector)) * DismountDistance;
		const FVector Candidate = Origin + Offset;
		FHitResult Ground;
		if (!World->LineTraceSingleByChannel(Ground, Candidate + FVector(0,0,50), Candidate - FVector(0,0,250), ECC_Pawn, GroundParams) ||
			!Character->GetCharacterMovement()->IsWalkable(Ground)) continue;
		const FVector Location = Ground.ImpactPoint + FVector(0,0,HalfHeight + 2.0f);
		const FCollisionShape Shape = FCollisionShape::MakeCapsule(Radius, HalfHeight);
		if (World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn, Shape, ClearanceParams)) continue;
		FHitResult PathHit;
		// Ignore only the ATV while crossing its body from the seat to the side.
		if (World->SweepSingleByChannel(PathHit, FVector(Origin.X, Origin.Y, Location.Z), Location,
			FQuat::Identity, ECC_Pawn, Shape, GroundParams)) continue;
		OutLocation = Location;
		return true;
	}
	return false;
}

bool UTunaSweeperVehicleMountComponent::TryDismount()
{
	FVector Location;
	if (!FindDismountLocation(Location)) return false;
	ReleaseRider(Location, true);
	return true;
}

void UTunaSweeperVehicleMountComponent::ReleaseRider(const FVector& Location, bool bPlaySound)
{
	auto* Character = Rider.Get();
	Rider.Reset();
	StopEngineAudio();
	bHintVisible = false;
	StationarySeconds = 0;
	if (DismountWidget) DismountWidget->RemoveFromParent();
	DismountWidget = nullptr;
	if (!Character) return;
	Character->VehicleMount = nullptr;
	Character->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	Character->SetActorLocationAndRotation(Location, FRotator(0, Character->GetActorRotation().Yaw, 0), false, nullptr, ETeleportType::TeleportPhysics);
	Character->GetCapsuleComponent()->IgnoreActorWhenMoving(GetOwner(), false);
	Character->GetCapsuleComponent()->SetCollisionEnabled(SavedCapsuleCollision);
	Character->GetCharacterMovement()->SetMovementMode(SavedMovementMode, SavedCustomMovementMode);
	Character->CancelActiveGameplayActions();
	if (bPlaySound && EngineStopSound) UGameplayStatics::PlaySoundAtLocation(this, EngineStopSound, GetComponentLocation());
	OnDismounted.Broadcast(Character);
}

void UTunaSweeperVehicleMountComponent::ReleaseRiderForEndPlay()
{
	FVector Location;
	if (!FindDismountLocation(Location)) Location = LastOnFootTransform.GetLocation();
	ReleaseRider(Location, false);
}

void UTunaSweeperVehicleMountComponent::StopEngineAudio()
{
	if (GetWorld()) GetWorld()->GetTimerManager().ClearTimer(EngineStartTimer);
	if (IsValid(EngineAudio)) EngineAudio->Stop();
	EngineAudio = nullptr;
}

void UTunaSweeperVehicleMountComponent::StartIdleAudio()
{
	StopEngineAudio();
	if (Rider.IsValid() && EngineIdleSound) EngineAudio = UGameplayStatics::SpawnSoundAttached(EngineIdleSound, this);
}

void UTunaSweeperVehicleMountComponent::UpdateStationaryHint(float DeltaTime, float Speed)
{
	StationarySeconds = Rider.IsValid() && Speed <= StationarySpeedThreshold
		? StationarySeconds + FMath::Max(0.0f, DeltaTime) : 0.0f;
	bHintVisible = Rider.IsValid() && Speed <= StationarySpeedThreshold && StationarySeconds >= DismountHintDelay;
}

void UTunaSweeperVehicleMountComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!Rider.IsValid())
	{
		if (EngineAudio || DismountWidget) ReleaseRiderForEndPlay();
		return;
	}
	if (Rider->IsDead() || Rider->GetVehicleMount() != this || Rider->GetRootComponent()->GetAttachParent() != this)
	{
		ReleaseRiderForEndPlay();
		return;
	}
	const FVector Location = GetComponentLocation();
	const float MeasuredSpeed = DeltaTime > SMALL_NUMBER ? FVector::Distance(Location, PreviousVehicleLocation) / DeltaTime : 0.0f;
	PreviousVehicleLocation = Location;
	UpdateStationaryHint(DeltaTime, FMath::Max(MeasuredSpeed, GetOwner()->GetVelocity().Size()));
	if (DismountWidget)
	{
		const auto* PC = Cast<ATunaSweeperPlayerController>(Rider->GetController());
		const bool bUiBlocked = PC && (PC->IsInventoryUiOpen() || PC->IsPauseMenuOpen() || PC->IsDialogueSequenceActive() || PC->IsHousingModeOpen());
		DismountWidget->SetVisibility(bHintVisible && !bUiBlocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UTunaSweeperVehicleMountComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	ReleaseRiderForEndPlay();
	Super::EndPlay(Reason);
}
