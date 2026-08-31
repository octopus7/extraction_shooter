#include "Interaction/TunaSweeperAchievementLocationTrigger.h"

#include "Character/TunaSweeperTopDownCharacter.h"
#include "Components/BoxComponent.h"
#include "Engine/GameInstance.h"
#include "Subsystem/TunaSweeperAchievementSubsystem.h"

ATunaSweeperAchievementLocationTrigger::ATunaSweeperAchievementLocationTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(150.0f, 150.0f, 120.0f));
	TriggerVolume->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerVolume->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerVolume->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerVolume->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerVolume->SetGenerateOverlapEvents(true);
	TriggerVolume->OnComponentBeginOverlap.AddDynamic(
		this,
		&ATunaSweeperAchievementLocationTrigger::HandleTriggerBeginOverlap);
}

void ATunaSweeperAchievementLocationTrigger::HandleTriggerBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComponent,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (bTriggeredThisInstance || LocationId.IsNone())
	{
		return;
	}

	const ATunaSweeperTopDownCharacter* PlayerCharacter = Cast<ATunaSweeperTopDownCharacter>(OtherActor);
	if (!PlayerCharacter || !PlayerCharacter->IsLocallyControlled())
	{
		return;
	}

	UGameInstance* GameInstance = GetGameInstance();
	UTunaSweeperAchievementSubsystem* AchievementSubsystem = GameInstance
		? GameInstance->GetSubsystem<UTunaSweeperAchievementSubsystem>()
		: nullptr;
	if (!AchievementSubsystem)
	{
		return;
	}

	bTriggeredThisInstance = true;
	AchievementSubsystem->ReportLocationReached(LocationId);
}
