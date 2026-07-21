#include "Environment/TunaSweeperOcclusionRevealBox.h"

#include "Component/TunaSweeperOcclusionRevealComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperOcclusionRevealBox::ATunaSweeperOcclusionRevealBox()
{
	RevealMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RevealMesh"));
	SetRootComponent(RevealMesh);
	RevealMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> BoxMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (BoxMesh.Succeeded())
	{
		RevealMesh->SetStaticMesh(BoxMesh.Object);
	}

	OcclusionRevealComponent = CreateDefaultSubobject<UTunaSweeperOcclusionRevealComponent>(TEXT("OcclusionRevealComponent"));
}
