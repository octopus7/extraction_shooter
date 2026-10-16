#include "TunaSweeperGameHudWidgetShared.h"
#include "TunaSweeperDeliveryIndicator.h"

#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "Interaction/TunaSweeperDeliveryDestinationActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"

void UTunaSweeperGameHudWidget::RefreshDeliveryIndicators(float DeltaTime)
{
    DeliveryIndicatorTargets.Reset();
    if (!GetWorld()) return;
    DeliveryIndicatorScanCountdown -= FMath::Max(0.f, DeltaTime);
    if (DeliveryIndicatorScanCountdown <= 0.f)
    {
        DeliveryDestinations.Reset();
        for (TActorIterator<ATunaSweeperDeliveryDestinationActor> It(GetWorld()); It; ++It)
            DeliveryDestinations.Add(*It);
        DeliveryDestinations.Sort([](const auto& A, const auto& B)
        {
            const auto* Left = A.Get(); const auto* Right = B.Get();
            if (!Left || !Right) return Left != nullptr;
            if (Left->QuestId != Right->QuestId) return Left->QuestId < Right->QuestId;
            return Left->GetFName().LexicalLess(Right->GetFName());
        });
        DeliveryIndicatorScanCountdown = .5f;
    }
    if (IsGameplayBottomHudSuppressed()) return;
    auto* Game = GetGameInstance<UTunaSweeperGameInstance>();
    auto* Items = Game ? Game->GetSubsystem<UTunaSweeperItemDataSubsystem>() : nullptr;
    for (const auto& WeakDestination : DeliveryDestinations)
    {
        const auto* Destination = WeakDestination.Get();
        int32 ItemId = 0;
        if (!TunaSweeperDeliveryIndicator::TryGetItemId(Destination, ItemId)) continue;
        FDeliveryIndicatorTarget Target;
        Target.Destination = WeakDestination; Target.ItemId = ItemId;
        DeliveryIndicatorTargets.Add(Target);
        if (Items && !DeliveryIndicatorIcons.Contains(ItemId))
        {
            FTunaSweeperItemDefinition Definition;
            UTexture2D* Icon = nullptr;
            if (Items->TryGetItemDefinition(ItemId, Definition))
            {
                const FString Path = Items->BuildItemIconObjectPath(Definition);
                if (!Path.IsEmpty()) Icon = LoadObject<UTexture2D>(nullptr, *Path);
            }
            // Keep loaded icons GC-safe and cache a missing icon to avoid repeated synchronous loads.
            DeliveryIndicatorIcons.Add(ItemId, Icon);
        }
    }
}

void UTunaSweeperGameHudWidget::DrawDeliveryIndicators(
    const FGeometry& Geometry, FSlateWindowElementList& Elements, int32& Layer) const
{
    if (IsGameplayBottomHudSuppressed()) return;
    auto* Controller = GetOwningPlayer();
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (!Pawn || !Controller->PlayerCameraManager) return;
    if (const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn); Character && Character->IsDead()) return;
    const FVector2D Size = Geometry.GetLocalSize();
    if (Size.X < 128 || Size.Y < 128) return;
    const FGeometry PlayerGeometry = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(Controller);
    const APlayerCameraManager* Camera = Controller->PlayerCameraManager;
    const FRotationMatrix CameraBasis(Camera->GetCameraRotation());
    TArray<FVector2D> DrawnCenters;
    for (const auto& Target : DeliveryIndicatorTargets)
    {
        const auto* Destination = Target.Destination.Get();
        int32 ItemId = 0;
        // Recheck at paint time too: submission or losing the parcel immediately removes the marker.
        if (!TunaSweeperDeliveryIndicator::TryGetItemId(Destination, ItemId)) continue;
        const FVector Location = Destination->Interactable ? Destination->Interactable->GetInteractionLocation() : Destination->GetActorLocation();
        FVector2D Projected;
        const bool bProjected = UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(Controller, Location, Projected, true);
        if (bProjected)
        {
            Projected = Geometry.AbsoluteToLocal(PlayerGeometry.LocalToAbsolute(Projected));
        }
        else
        {
            // Projection behind the camera must not mirror the direction.
            const FVector Offset = Location - Camera->GetCameraLocation();
            FVector2D Direction(FVector::DotProduct(Offset, CameraBasis.GetUnitAxis(EAxis::Y)),
                -FVector::DotProduct(Offset, CameraBasis.GetUnitAxis(EAxis::Z)));
            Direction.Normalize();
            if (Direction.IsNearlyZero()) Direction = FVector2D(0,-1);
            Projected = Size * .5 + Direction * FMath::Max(Size.X, Size.Y) * 2;
        }
        TunaSweeperDeliveryIndicator::FPlacement Placement;
        if (!TunaSweeperDeliveryIndicator::ComputePlacement(Size, Projected, bProjected,
            GetWorld()->GetTimeSeconds(), Placement)) continue;
        // If parcels share a bearing, use separate inward lanes without rotating their arrows.
        for (int32 Attempt = 0; Attempt < DrawnCenters.Num(); ++Attempt)
        {
            if (!DrawnCenters.ContainsByPredicate([&](const FVector2D& Other) { return FVector2D::Distance(Other, Placement.Position) < 68; })) break;
            Placement.Position -= Placement.bAtEdge ? Placement.Direction * 68 : FVector2D(0,68);
            Placement.Position.X = FMath::Clamp(Placement.Position.X, 48.0, Size.X - 48.0);
            Placement.Position.Y = FMath::Clamp(Placement.Position.Y, 48.0, Size.Y - 48.0);
        }
        DrawnCenters.Add(Placement.Position);
        const auto* Icon = DeliveryIndicatorIcons.Find(ItemId);
        TunaSweeperDeliveryIndicator::DrawMarker(Placement, Icon ? Icon->Get() : nullptr, Geometry, Elements, Layer);
    }
}
