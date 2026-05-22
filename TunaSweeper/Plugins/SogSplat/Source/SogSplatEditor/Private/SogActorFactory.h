#pragma once

#include "ActorFactories/ActorFactory.h"
#include "CoreMinimal.h"
#include "SogActorFactory.generated.h"

UCLASS(Transient)
class USogActorFactory : public UActorFactory
{
	GENERATED_BODY()

public:
	USogActorFactory();

	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual void PostSpawnActor(UObject* Asset, AActor* NewActor) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
};
