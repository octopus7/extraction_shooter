#include "Component/TunaSweeperLaserSightComponent.h"

#include "NiagaraSystem.h"
#include "UObject/ConstructorHelpers.h"

UTunaSweeperLaserSightComponent::UTunaSweeperLaserSightComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	bAutoActivate = false;
	SetAutoActivate(false);
	SetHiddenInGame(true);
	SetVisibility(false, true);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LaserSightSystem(
		TEXT("/Game/FX/NS_LaserSight.NS_LaserSight"));
	if (LaserSightSystem.Succeeded())
	{
		SetAsset(LaserSightSystem.Object);
	}

	ApplyLaserSightParameters();
}

void UTunaSweeperLaserSightComponent::BeginPlay()
{
	Super::BeginPlay();
	ApplyLaserSightParameters();
	SetLaserSightEnabled(bLaserSightEnabled);
}

void UTunaSweeperLaserSightComponent::SetBeamEnd(const FVector& InBeamEnd)
{
	BeamEnd = InBeamEnd;
	ApplyLaserSightParameters();
}

void UTunaSweeperLaserSightComponent::SetLaserSightEnabled(bool bEnabled)
{
	bLaserSightEnabled = bEnabled;
	SetHiddenInGame(!bLaserSightEnabled);
	SetVisibility(bLaserSightEnabled, true);

	if (bLaserSightEnabled)
	{
		ApplyLaserSightParameters();
		Activate(true);
	}
	else
	{
		DeactivateImmediate();
	}
}

void UTunaSweeperLaserSightComponent::ApplyLaserSightParameters()
{
	SetVariableVec3(FName(TEXT("User.BeamEnd")), BeamEnd);
}
