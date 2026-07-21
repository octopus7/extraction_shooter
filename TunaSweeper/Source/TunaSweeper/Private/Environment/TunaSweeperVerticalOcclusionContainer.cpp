#include "Environment/TunaSweeperVerticalOcclusionContainer.h"

#include "Component/TunaSweeperVerticalOcclusionRevealComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ATunaSweeperVerticalOcclusionContainer::ATunaSweeperVerticalOcclusionContainer()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	const ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	UStaticMesh* DefaultCubeMesh = CubeMesh.Object;
	auto ConfigureSection = [this, Root, DefaultCubeMesh](const TCHAR* Name, const FVector& Location, const FVector& Scale)
	{
		UStaticMeshComponent* Section = CreateDefaultSubobject<UStaticMeshComponent>(Name);
		Section->SetupAttachment(Root);
		Section->SetRelativeLocation(Location);
		Section->SetRelativeScale3D(Scale);
		Section->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		if (DefaultCubeMesh)
		{
			Section->SetStaticMesh(DefaultCubeMesh);
		}
		return Section;
	};

	LeftPillar = ConfigureSection(TEXT("LeftPillar"), FVector(-190.0f, 0.0f, 125.0f), FVector(0.25f, 2.5f, 2.5f));
	RightPillar = ConfigureSection(TEXT("RightPillar"), FVector(190.0f, 0.0f, 125.0f), FVector(0.25f, 2.5f, 2.5f));
	TopBeam = ConfigureSection(TEXT("TopBeam"), FVector(0.0f, 0.0f, 260.0f), FVector(4.05f, 2.5f, 0.25f));
	VerticalOcclusionRevealComponent = CreateDefaultSubobject<UTunaSweeperVerticalOcclusionRevealComponent>(TEXT("VerticalOcclusionRevealComponent"));
}
