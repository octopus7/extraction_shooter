#pragma once
#include "CoreMinimal.h"
class UPixelWorldProfile;
class AActor;
class UWorld;

namespace PixelWorld
{
    bool IsRoot(const AActor* Actor);
    bool Generate(UWorld* World, const UPixelWorldProfile& Profile, AActor* ReplaceRoot, AActor*& OutRoot, FText& Error);
    TArray<AActor*> OwnedActors(AActor* Root);
}
