#include "TunaSweeperGameHudWidgetShared.h"
#include "TunaSweeperDeliveryIndicator.h"

#include "Camera/PlayerCameraManager.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "SceneView.h"
#include "Interaction/TunaSweeperDeliveryDestinationActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Fonts/FontMeasure.h"
#include "Raid/TunaSweeperSupplyDropActor.h"
#include "Subsystem/TunaSweeperSupplyDropSubsystem.h"

namespace
{
    void DrawSupplyTicker(const FGeometry& Geometry, FSlateWindowElementList& Elements,
        int32& Layer, float Seconds, const FText& Message)
    {
        if (Message.IsEmpty()) return;
        const float Width = FMath::Min(520.f, Geometry.GetLocalSize().X - 48.f);
        const FVector2f Origin((Geometry.GetLocalSize().X - Width) * .5f, 10.f);
        const auto Strip = Geometry.ToPaintGeometry(FVector2f(Width, 34.f), FSlateLayoutTransform(Origin));
        FSlateDrawElement::MakeBox(Elements, ++Layer, Strip, FCoreStyle::Get().GetBrush("WhiteBrush"),
            ESlateDrawEffect::None, FLinearColor(.024f,.033f,.035f,.90f));

        const FSlateFontInfo Font = TunaSweeperUIFont::MakeFont(nullptr, 16, ETunaSweeperUIFontWeight::Bold);
        const FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Message, Font);
        const float Pitch = TextSize.X + 80.f;
        const float Offset = FMath::Fmod(Seconds * 64.f, Pitch);
        const float Alpha = .72f + .28f * FMath::Cos(Seconds * UE_TWO_PI / 2.2f);
        Elements.PushClip(FSlateClippingZone(Strip));
        ++Layer;
        for (float X = 12.f - Offset; X < Width; X += Pitch)
        {
            const FVector2f Position = Origin + FVector2f(X, (34.f - TextSize.Y) * .5f);
            FSlateDrawElement::MakeText(Elements, Layer,
                Geometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(Position)),
                Message, Font, ESlateDrawEffect::None, FLinearColor(1.f,.86f,.53f,Alpha));
        }
        Elements.PopClip();
    }
}

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
    const ULocalPlayer* Player = Controller->GetLocalPlayer();
    FSceneViewProjectionData Projection;
    if (!Player || !Player->ViewportClient || !Player->ViewportClient->Viewport ||
        !Player->GetProjectionData(Player->ViewportClient->Viewport, Projection)) return;
    const FIntRect Rect = Projection.GetConstrainedViewRect();
    if (Rect.Width() <= 0 || Rect.Height() <= 0) return;
    const FMatrix ViewProjection = Projection.ComputeViewProjectionMatrix();
    const APlayerCameraManager* Camera = Controller->PlayerCameraManager;
    const FRotationMatrix CameraBasis(Camera->GetCameraRotation());
    auto ProjectMarker = [&](const FVector& Location, TunaSweeperDeliveryIndicator::FPlacement& Placement)
    {
        FVector2D Projected;
        const bool bProjected = FSceneView::ProjectWorldToScreen(Location, Rect, ViewProjection, Projected);
        if (bProjected)
        {
            Projected = (Projected - FVector2D(Rect.Min)) * Size / FVector2D(Rect.Size());
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
        return TunaSweeperDeliveryIndicator::ComputePlacement(Size, Projected, bProjected,
            GetWorld()->GetTimeSeconds(), Placement);
    };

    TArray<FVector2D> DrawnCenters;
    const auto* Drops = GetWorld()->GetSubsystem<UTunaSweeperSupplyDropSubsystem>();
    if (const auto* Drop = Drops ? Drops->GetDescendingDrop() : nullptr)
    {
        TunaSweeperDeliveryIndicator::FPlacement Placement;
        if (ProjectMarker(Drop->GetActorLocation(), Placement))
        {
            TunaSweeperDeliveryIndicator::DrawMarker(Placement, nullptr, Geometry, Elements, Layer, true);
            DrawnCenters.Add(Placement.Position);
        }
        DrawSupplyTicker(Geometry, Elements, Layer, Drop->GetDescentElapsed(),
            ResolveUiText(GetGameInstance<UTunaSweeperGameInstance>(), TEXT("ui.hud.supply_drop_descending"), TEXT("")));
    }

    for (const auto& Target : DeliveryIndicatorTargets)
    {
        const auto* Destination = Target.Destination.Get();
        int32 ItemId = 0;
        if (!TunaSweeperDeliveryIndicator::TryGetItemId(Destination, ItemId)) continue;
        const FVector Location = Destination->Interactable ? Destination->Interactable->GetInteractionLocation() : Destination->GetActorLocation();
        TunaSweeperDeliveryIndicator::FPlacement Placement;
        if (!ProjectMarker(Location, Placement)) continue;
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
