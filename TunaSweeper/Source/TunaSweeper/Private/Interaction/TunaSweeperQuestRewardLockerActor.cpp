#include "Interaction/TunaSweeperQuestRewardLockerActor.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
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
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Screen(TEXT("/Game/Interaction/QuestRewardLocker/M_LockerScreen.M_LockerScreen"));
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> Recess(TEXT("/Game/Interaction/QuestRewardLocker/M_LockerRecess.M_LockerRecess"));
	// +X faces the room; +Y is the seated user's right. The housing sits inside the wall.
	VisualMesh->SetRelativeLocation(FVector(-4, 0, 162));
	VisualMesh->SetRelativeScale3D(FVector(0.12f, 1.12f, 0.44f));
	VisualMesh->SetCollisionProfileName(TEXT("BlockAll"));
	VisualMesh->SetMaterial(0, Iron.Object);
	OutletRoot = CreateDefaultSubobject<USceneComponent>(TEXT("OutletRoot"));
	OutletRoot->SetupAttachment(SceneRoot);
	OutletRoot->SetRelativeLocation(FVector(0, 145, 0));
	auto AddPanelPart = [&](const TCHAR* Name, FVector Location, FVector Scale, UMaterialInterface* Material)
	{
		UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		const bool bOutlet = FString(Name).StartsWith(TEXT("Outlet"));
		Part->SetupAttachment(bOutlet ? OutletRoot.Get() : SceneRoot.Get());
		if (bOutlet) Location.Y -= 145;
		Part->SetStaticMesh(Cube.Object);
		Part->SetMaterial(0, Material);
		Part->SetRelativeLocation(Location);
		Part->SetRelativeScale3D(Scale);
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	};
	AddPanelPart(TEXT("ScreenGlass"), FVector(2.5, 0, 162), FVector(0.01, 1.0, 0.32), Screen.Object);
	AddPanelPart(TEXT("ScreenStatus"), FVector(3.2, -37, 162), FVector(0.008, 0.12, 0.15), Green.Object);
	AddPanelPart(TEXT("ScreenLineTop"), FVector(3.2, 10, 167), FVector(0.008, 0.56, 0.018), Brass.Object);
	AddPanelPart(TEXT("ScreenLineBottom"), FVector(3.2, 1, 158), FVector(0.008, 0.38, 0.018), Green.Object);
	AddPanelPart(TEXT("OutletHousing"), FVector(-4, 145, 103), FVector(0.12, 0.74, 0.66), Iron.Object);
	AddPanelPart(TEXT("OutletRecess"), FVector(2.3, 145, 103), FVector(0.008, 0.62, 0.52), Recess.Object);
	AddPanelPart(TEXT("OutletTopTrim"), FVector(4, 145, 133), FVector(0.06, 0.74, 0.05), Green.Object);
	AddPanelPart(TEXT("OutletLeftTrim"), FVector(4, 110, 103), FVector(0.06, 0.04, 0.58), Green.Object);
	AddPanelPart(TEXT("OutletRightTrim"), FVector(4, 180, 103), FVector(0.06, 0.04, 0.58), Green.Object);
	AddPanelPart(TEXT("OutletTray"), FVector(8, 145, 74), FVector(0.22, 0.72, 0.045), Iron.Object);
	AddPanelPart(TEXT("OutletIndicator"), FVector(4, 145, 144), FVector(0.015, 0.2, 0.025), Screen.Object);
	InteractableComponent->SetRelativeLocation(FVector(35, 145, 135));
}


void ATunaSweeperQuestRewardLockerActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	OutletRoot->SetRelativeTransform(OutletTransform);
	InteractableComponent->SetRelativeLocation(OutletTransform.TransformPosition(FVector(35, 0, 135)));
}
