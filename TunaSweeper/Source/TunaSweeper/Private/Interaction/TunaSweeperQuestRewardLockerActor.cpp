#include "Interaction/TunaSweeperQuestRewardLockerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperQuestRewardLockerActor::ATunaSweeperQuestRewardLockerActor()
{
	ConfigureInteractionDefaults(ETunaSweeperInteractionType::QuestRewardLockerOpen, FText::GetEmpty(),
		TSoftClassPtr<UTunaSweeperInteractionMarkerWidget>(FSoftObjectPath(TEXT("/Game/UI/WBP_InteractionMarker.WBP_InteractionMarker_C"))),
		TEXT("ui.interaction.quest_reward_locker"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Iron(TEXT("/Game/Interaction/QuestRewardLocker/M_LockerBody.M_LockerBody"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Green(TEXT("/Game/Interaction/QuestRewardLocker/M_LockerDoor.M_LockerDoor"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Brass(TEXT("/Game/Interaction/QuestRewardLocker/M_LockerHandle.M_LockerHandle"));
	VisualMesh->SetRelativeLocation(FVector(0, 0, 100));
	VisualMesh->SetRelativeScale3D(FVector(0.6f, 1.6f, 2.0f));
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	VisualMesh->SetMaterial(0, Iron.Object);
	// Front is +X. Blocky inset doors and handles make the saved Blueprint a parcel locker.
	for (int32 Row = 0; Row < 3; ++Row)
	{
		for (int32 Column = 0; Column < 2; ++Column)
		{
			UStaticMeshComponent* Door = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ParcelDoor_%d_%d"), Row, Column));
			Door->SetupAttachment(SceneRoot);
			Door->SetStaticMesh(Cube.Object);
			Door->SetMaterial(0, Green.Object);
			Door->SetRelativeLocation(FVector(32, (Column == 0 ? -39 : 39), 36 + Row * 62));
			Door->SetRelativeScale3D(FVector(0.06f, 0.73f, 0.56f));
			Door->SetCollisionEnabled(ECollisionEnabled::NoCollision);
			UStaticMeshComponent* Handle = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ParcelHandle_%d_%d"), Row, Column));
			Handle->SetupAttachment(SceneRoot);
			Handle->SetStaticMesh(Cube.Object);
			Handle->SetMaterial(0, Brass.Object);
			Handle->SetRelativeLocation(FVector(38, (Column == 0 ? -16 : 62), 36 + Row * 62));
			Handle->SetRelativeScale3D(FVector(0.08f, 0.06f, 0.18f));
			Handle->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	InteractableComponent->SetRelativeLocation(FVector(45, 0, 160));
}
