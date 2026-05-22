#include "SogSplatActor.h"

#include "SogSplatComponent.h"

ASogSplatActor::ASogSplatActor()
{
	PrimaryActorTick.bCanEverTick = false;

	SogSplatComponent = CreateDefaultSubobject<USogSplatComponent>(TEXT("SogSplatComponent"));
	RootComponent = SogSplatComponent;
}

USogSplatComponent* ASogSplatActor::GetSogSplatComponent() const
{
	return SogSplatComponent;
}
