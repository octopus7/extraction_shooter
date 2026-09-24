#pragma once
#include "CoreMinimal.h"
#include "Interfaces/OnlineIdentityInterface.h"

// UE's EOS identity implementation owns the Steam ticket and EOS Connect exchange.
// This boundary deliberately exposes no ticket bytes to game code.
struct FTunaSweeperPlatformAuthAdapter
{
    static bool AutoLogin(const IOnlineIdentityPtr& Identity)
    {
        return Identity.IsValid() && Identity->AutoLogin(0);
    }
};
