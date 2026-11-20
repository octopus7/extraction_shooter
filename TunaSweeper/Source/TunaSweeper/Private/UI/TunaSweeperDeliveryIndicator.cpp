#include "TunaSweeperDeliveryIndicator.h"

#include "Engine/Texture2D.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Interaction/TunaSweeperDeliveryDestinationActor.h"
#include "Rendering/DrawElements.h"
#include "Styling/SlateBrush.h"
#include "Styling/CoreStyle.h"
#include "Subsystem/TunaSweeperItemDataSubsystem.h"
#include "Subsystem/TunaSweeperQuestSubsystem.h"

namespace TunaSweeperDeliveryIndicator
{
    bool ComputePlacement(const FVector2D& ViewSize, const FVector2D& ProjectedPosition,
        bool bInFront, float Seconds, FPlacement& Out, float Inset, float BounceHeight, float BouncePeriod)
    {
        Out = FPlacement();
        if (ViewSize.ContainsNaN() || ProjectedPosition.ContainsNaN() ||
            ViewSize.X < 128 || ViewSize.Y < 128 || !FMath::IsFinite(Seconds)) return false;
        const double SafeInset = FMath::Clamp(static_cast<double>(Inset), 48.0, FMath::Min(ViewSize.X, ViewSize.Y) * .4);
        const FVector2D Center = ViewSize * .5;
        const FVector2D Extent = Center - FVector2D(SafeInset, SafeInset);
        Out.Direction = (ProjectedPosition - Center).GetSafeNormal();
        if (Out.Direction.IsNearlyZero()) Out.Direction = FVector2D(0, -1);
        Out.bAtEdge = !bInFront || FMath::Abs(ProjectedPosition.X - Center.X) > Extent.X ||
            FMath::Abs(ProjectedPosition.Y - Center.Y) > Extent.Y;
        if (!Out.bAtEdge)
        {
            Out.Position = ProjectedPosition;
            return true;
        }
        const double RayLength = FMath::Min(
            Extent.X / FMath::Max(FMath::Abs(Out.Direction.X), 0.0001),
            Extent.Y / FMath::Max(FMath::Abs(Out.Direction.Y), 0.0001));
        const float Phase = FMath::Fmod(FMath::Max(0.f, Seconds), FMath::Max(.2f, BouncePeriod)) / FMath::Max(.2f, BouncePeriod);
        // A larger hop, a small settling hop, then a pause. Only position bounces; bearing stays fixed.
        const float T = Phase < .4f ? Phase / .4f : (Phase - .4f) / .24f;
        const float Hop = Phase < .64f ? 4.f * T * (1.f - T) * (Phase < .4f ? 1.f : .3f) : 0.f;
        const double Lift = FMath::Clamp(static_cast<double>(BounceHeight), 0.0, RayLength * .25) * FMath::Max(0.f, Hop);
        Out.Position = Center + Out.Direction * (RayLength - Lift);
        return true;
    }

    void DrawMarker(const FPlacement& Placement, UTexture2D* Icon, const FGeometry& Geometry,
        FSlateWindowElementList& Elements, int32& Layer, bool bSupplyDrop)
    {
        const FLinearColor Gold(1.f, .76f, .22f, 1.f);
        auto BoxGeometry = [&](const FVector2D& Position, const FVector2D& Size)
        {
            return Geometry.ToPaintGeometry(FVector2f(Size), FSlateLayoutTransform(FVector2f(Position)));
        };
        FSlateBrush Badge;
        Badge.DrawAs = ESlateBrushDrawType::RoundedBox;
        Badge.TintColor = FSlateColor(FLinearColor::White);
        Badge.OutlineSettings = FSlateBrushOutlineSettings(25.f, FSlateColor(Gold), 2.f);
        Badge.OutlineSettings.bUseBrushTransparency = false;
        FSlateDrawElement::MakeBox(Elements, Layer + 1,
            BoxGeometry(Placement.Position - FVector2D(25,25), FVector2D(50,50)),
            &Badge, ESlateDrawEffect::None, FLinearColor(.018f,.025f,.03f,.92f));
        if (bSupplyDrop)
        {
            const FVector2D C = Placement.Position;
            auto Stroke = [&](TArray<FVector2D> Points, FLinearColor Color, float Width)
            {
                for (auto& Point : Points) Point += C;
                FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(),
                    Points, ESlateDrawEffect::None, Color, true, Width);
            };
            const FLinearColor Canvas(.68f, .79f, .41f);
            Stroke({{-15,-4},{-12,-10},{-7,-14},{0,-16},{7,-14},{12,-10},{15,-4},{-15,-4}}, Canvas, 2.5f);
            Stroke({{-6,-4},{-4,-11},{0,-16},{4,-11},{6,-4}}, Canvas, 1.5f);
            Stroke({{-14,-3},{-5,7},{5,7},{14,-3}}, FLinearColor::White, 1.5f);
            FSlateDrawElement::MakeBox(Elements, Layer + 2, BoxGeometry(C + FVector2D(-8,7), FVector2D(16,10)),
                FCoreStyle::Get().GetBrush("WhiteBrush"), ESlateDrawEffect::None, FLinearColor(.8f,.11f,.075f));
            Stroke({{-7,9},{7,9}}, FLinearColor(1.f,.64f,.38f), 1.5f);
        }
        else if (Icon)
        {
            FSlateBrush Image;
            Image.DrawAs = ESlateBrushDrawType::Image;
            Image.SetResourceObject(Icon);
            Image.SetImageSize(FVector2D(36,36));
            FSlateDrawElement::MakeBox(Elements, Layer + 2,
                BoxGeometry(Placement.Position - FVector2D(18,18), FVector2D(36,36)),
                &Image, ESlateDrawEffect::None, FLinearColor::White);
        }
        else
        {
            // Geometry-only parcel fallback; no unlocalized text or missing texture label.
            const auto C = Placement.Position;
            FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(),
                {C+FVector2D(-11,-10),C+FVector2D(11,-10),C+FVector2D(11,10),C+FVector2D(-11,10),C+FVector2D(-11,-10)},
                ESlateDrawEffect::None, Gold, true, 2.f);
            FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(),
                {C+FVector2D(0,-10),C+FVector2D(0,10)}, ESlateDrawEffect::None, Gold, true, 2.f);
        }
        if (Placement.bAtEdge)
        {
            const FVector2D D = Placement.Direction;
            const FVector2D Side(-D.Y, D.X);
            const FVector2D Base = Placement.Position + D * 30;
            const FVector2D Tip = Base + D * (bSupplyDrop ? 23 : 10);
            TArray<FVector2D> Arrow = {Base + Side * 8, Tip, Base - Side * 8};
            if (bSupplyDrop) Arrow.Add(Base + Side * 8);
            FSlateDrawElement::MakeLines(Elements, Layer + 2, Geometry.ToPaintGeometry(), Arrow,
                ESlateDrawEffect::None, FLinearColor(0,0,0,.9f), true, 7.f);
            FSlateDrawElement::MakeLines(Elements, Layer + 3, Geometry.ToPaintGeometry(), Arrow,
                ESlateDrawEffect::None, Gold, true, 3.f);
        }
        Layer += 3;
    }

    bool TryGetItemId(const ATunaSweeperDeliveryDestinationActor* Destination, int32& OutItemId)
    {
        OutItemId = 0;
        if (!IsValid(Destination) || Destination->IsActorBeingDestroyed() || Destination->IsHidden() || !Destination->CanDeliver()) return false;
        auto* Game = Destination->GetGameInstance<UTunaSweeperGameInstance>();
        auto* Quests = Game ? Game->GetSubsystem<UTunaSweeperQuestSubsystem>() : nullptr;
        const auto* Definition = Quests ? Quests->FindQuestDefinition(Destination->QuestId) : nullptr;
        if (!Definition) return false;
        const auto* Objective = Definition->Objectives.FindByPredicate(
            [Destination](const auto& Value) { return Value.ObjectiveId == Destination->ObjectiveId; });
        auto* Items = Game->GetSubsystem<UTunaSweeperItemDataSubsystem>();
        FTunaSweeperItemDefinition Item;
        if (!Objective || !Items || !Items->TryGetItemDefinition(Objective->ItemId, Item) || !Item.IsVolatile()) return false;
        // Submission can also count storage; navigation must only guide a parcel carried by the player.
        if (Game->CountCarriedItemById(Objective->ItemId) < FMath::Max(1, Objective->RequiredCount)) return false;
        OutItemId = Objective->ItemId;
        return true;
    }
}
