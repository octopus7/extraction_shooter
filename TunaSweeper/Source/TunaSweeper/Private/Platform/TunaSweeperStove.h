#pragma once

#include "CoreMinimal.h"

namespace TunaSweeperStove
{
    void Startup();
    void Shutdown();
    // Cached during module startup, before any world or GameInstance save loading.
    // Root is the resolved directory configured in STOVE Studio; caller appends the build flavor.
    bool TryGetCloudSaveRoot(FString& OutDirectory);
    bool TryGetSaveAccountId(FString& OutAccountId);
}
