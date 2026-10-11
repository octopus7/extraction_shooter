#include "Interaction/TunaSweeperQuestArrivalTrigger.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Game/TunaSweeperGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"
#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"

ATunaSweeperQuestArrivalTrigger::ATunaSweeperQuestArrivalTrigger()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_PostPhysics;
	ArrivalArea = CreateDefaultSubobject<UBoxComponent>(TEXT("ArrivalArea"));
	SetRootComponent(ArrivalArea);
	ArrivalArea->SetBoxExtent(FVector(250.0f, 250.0f, 250.0f));
	ArrivalArea->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrivalArea->SetGenerateOverlapEvents(false);
	ArrivalArea->SetCanEverAffectNavigation(false);
}

void ATunaSweeperQuestArrivalTrigger::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	CheckArrivals();
}

bool ATunaSweeperQuestArrivalTrigger::CanReportArrival(const ATunaSweeperTopDownCharacter* Character) const
{
	if (!IsValid(Character) || !Character->IsLocallyControlled() || Character->IsDead() ||
		Character->GetWorld() != GetWorld() || Character->IsActorBeingDestroyed()) return false;
	const FVector LocalPosition = ArrivalArea->GetComponentTransform().InverseTransformPosition(Character->GetActorLocation());
	const FVector Extent = ArrivalArea->GetUnscaledBoxExtent();
	const FVector Scale = ArrivalArea->GetComponentScale().GetAbs();
	if (LocalPosition.ContainsNaN() || Extent.GetMin() <= 0.0 || Scale.GetMin() <= UE_SMALL_NUMBER ||
		FMath::Abs(LocalPosition.X) > Extent.X || FMath::Abs(LocalPosition.Y) > Extent.Y ||
		FMath::Abs(LocalPosition.Z) > Extent.Z) return false;
	if (!bRequireMountedVehicle) return true;

	const auto* Mount = Character->GetVehicleMount();
	const auto* Vehicle = IsValid(Mount) ? Cast<ATunaSweeperATVActor>(Mount->GetOwner()) : nullptr;
	return IsValid(Vehicle) && !Vehicle->IsActorBeingDestroyed() && !Vehicle->IsVehicleDestroyed() &&
		Mount->GetRider() == Character && Character->GetAttachParentActor() == Vehicle &&
		(RequiredVehicleTag.IsNone() || Vehicle->ActorHasTag(RequiredVehicleTag));
}

void ATunaSweeperQuestArrivalTrigger::CheckArrivals()
{
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld() || LocationId.IsNone() || IsActorBeingDestroyed() || !ArrivalArea) return;
	auto* Game = World->GetGameInstance();
	auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
	if (!Quests) return;
	for (const int32 QuestId : RequiredCompletedQuestIds)
	{
		if (QuestId <= 0 || !Quests->FindQuestDefinition(QuestId) ||
			Quests->GetQuestState(QuestId) != ETunaSweeperQuestState::RewardCompleted) return;
	}
	const auto* TunaGame = Cast<UTunaSweeperGameInstance>(Game);
	for (const FName ProgressId : RequiredCompletedProgressIds)
	{
		FTunaSweeperWorldProgressSaveData Progress;
		if (ProgressId.IsNone() || !TunaGame || !TunaGame->TryGetWorldProgressState(ProgressId, Progress) ||
			Progress.State != ETunaSweeperWorldProgressState::Completed) return;
	}
	// Poll the actual rider, not the vehicle/train overlap. This also covers load, acceptance,
	// or mounting while already inside the box, and ignores empty vehicles and NPCs.
	for (auto It = World->GetPlayerControllerIterator(); It; ++It)
	{
		const auto* Controller = It->Get();
		const auto* Character = Controller ? Cast<ATunaSweeperTopDownCharacter>(Controller->GetPawn()) : nullptr;
		if (CanReportArrival(Character)) Quests->NotifyLocationReached(LocationId, bRequireMountedVehicle);
	}
}
