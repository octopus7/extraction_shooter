#include "Vehicle/TunaSweeperATVActor.h"
#include "Vehicle/TunaSweeperVehicleMountComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperATVActor::ATunaSweeperATVActor()
{
	ChassisCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("ChassisCollision"));
	SetRootComponent(ChassisCollision);
	ChassisCollision->SetBoxExtent(FVector(85.0f, 55.0f, 45.0f));
	ChassisCollision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	VehicleMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
	VehicleMesh->SetupAttachment(ChassisCollision);
	VehicleMesh->SetRelativeLocation(FVector(0, 0, -45));
	VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> Mesh(TEXT("/Game/Meshes/Props/ATV/SKM_ATV.SKM_ATV"));
	if (Mesh.Succeeded()) VehicleMesh->SetSkeletalMesh(Mesh.Object);
	MountComponent = CreateDefaultSubobject<UTunaSweeperVehicleMountComponent>(TEXT("MountComponent"));
	MountComponent->SetupAttachment(VehicleMesh, TEXT("seat"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Start(TEXT("/Game/Audio/ATV/SW_ATV_Mount_Start.SW_ATV_Mount_Start"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Idle(TEXT("/Game/Audio/ATV/SW_ATV_Idle_Loop.SW_ATV_Idle_Loop"));
	static ConstructorHelpers::FObjectFinder<USoundBase> Stop(TEXT("/Game/Audio/ATV/SW_ATV_Dismount_Stop.SW_ATV_Dismount_Stop"));
	MountComponent->EngineStartSound = Start.Object;
	MountComponent->EngineIdleSound = Idle.Object;
	MountComponent->EngineStopSound = Stop.Object;
}
