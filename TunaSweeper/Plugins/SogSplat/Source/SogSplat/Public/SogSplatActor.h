#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SogSplatActor.generated.h"

class USogSplatComponent;

UCLASS()
class SOGSPLAT_API ASogSplatActor : public AActor
{
	GENERATED_BODY()

public:
	ASogSplatActor();

	UFUNCTION(BlueprintPure, Category = "SOG")
	USogSplatComponent* GetSogSplatComponent() const;

private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "SOG", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USogSplatComponent> SogSplatComponent;
};
