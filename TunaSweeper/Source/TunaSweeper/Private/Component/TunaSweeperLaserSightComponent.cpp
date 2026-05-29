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
}

void UTunaSweeperLaserSightComponent::BeginPlay()
{
	Super::BeginPlay();
	SetLaserSightEnabled(bLaserSightEnabled);
}

void UTunaSweeperLaserSightComponent::SetLaserSightEnabled(bool bEnabled)
{
	bLaserSightEnabled = bEnabled;
	SetHiddenInGame(!bLaserSightEnabled);
	SetVisibility(bLaserSightEnabled, true);

	if (bLaserSightEnabled)
	{
		Activate(true);
	}
	else
	{
		DeactivateImmediate();
	}
}
