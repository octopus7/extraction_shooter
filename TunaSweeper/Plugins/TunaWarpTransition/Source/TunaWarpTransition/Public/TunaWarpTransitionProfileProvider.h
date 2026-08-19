#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TunaWarpTransitionProfileProvider.generated.h"

class UTunaWarpTransitionProfile;

/** Implement on a GameInstance to supply the project's default warp profile. */
UINTERFACE(BlueprintType)
class TUNAWARPTRANSITION_API UTunaWarpTransitionProfileProvider : public UInterface
{
	GENERATED_BODY()
};

class TUNAWARPTRANSITION_API ITunaWarpTransitionProfileProvider
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Tuna Warp Transition")
	UTunaWarpTransitionProfile* GetWarpTransitionProfile() const;
};
