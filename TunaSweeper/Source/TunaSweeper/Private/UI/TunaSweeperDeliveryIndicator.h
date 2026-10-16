#pragma once

#include "CoreMinimal.h"

class ATunaSweeperDeliveryDestinationActor;
class UTexture2D;
struct FGeometry;
class FSlateWindowElementList;

namespace TunaSweeperDeliveryIndicator
{
    struct FPlacement
    {
        FVector2D Position = FVector2D::ZeroVector;
        FVector2D Direction = FVector2D(0, -1);
        bool bAtEdge = false;
    };

    // All positions and distances are HUD-local Slate units, independent of world scale and DPI.
    bool ComputePlacement(const FVector2D& ViewSize, const FVector2D& ProjectedPosition,
        bool bInFront, float Seconds, FPlacement& Out, float Inset = 80.0f,
        float BounceHeight = 14.0f, float BouncePeriod = 1.2f);
    void DrawMarker(const FPlacement& Placement, UTexture2D* Icon, const FGeometry& Geometry,
        FSlateWindowElementList& Elements, int32& Layer);
    bool TryGetItemId(const ATunaSweeperDeliveryDestinationActor* Destination, int32& OutItemId);
}
