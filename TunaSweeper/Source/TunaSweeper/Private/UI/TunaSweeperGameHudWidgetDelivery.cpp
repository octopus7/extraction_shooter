#include "TunaSweeperGameHudWidgetShared.h"
#include "TunaSweeperDeliveryIndicator.h"

#include "Algo/BinarySearch.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/CapsuleComponent.h"
#include "EngineUtils.h"
#include "Engine/LocalPlayer.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "ProceduralMeshComponent.h"
#include "Quest/QuestGuidanceLocations.h"
#include "SceneView.h"
#include "Interaction/TunaSweeperDeliveryDestinationActor.h"
#include "Interaction/TunaSweeperInteractableComponent.h"
#include "Fonts/FontMeasure.h"
#include "Raid/TunaSweeperSupplyDropActor.h"
#include "Subsystem/TunaSweeperSupplyDropSubsystem.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#endif

namespace
{
    constexpr float QuestGuidanceRefreshSeconds = 0.25f;
    constexpr double TrailDotSpacing = 65.0;
    constexpr double TrailFirstDotDistance = 115.0;
    constexpr int32 TrailWindowStep = 8;
    constexpr int32 TrailWindowCount = 48;
    constexpr double MaxTrailDistance = 2150.0;

    TOptional<FVector> FindClosestQuestGuidanceLocation(const FVector& Origin, TConstArrayView<FVector> Locations)
    {
        TOptional<FVector> Closest;
        double ClosestDistanceSquared = TNumericLimits<double>::Max();
        for (const FVector& Location : Locations)
        {
            const double DistanceSquared = FVector::DistSquared(Origin, Location);
            if (DistanceSquared < ClosestDistanceSquared)
            {
                ClosestDistanceSquared = DistanceSquared;
                Closest = Location;
            }
        }
        return Closest;
    }

    bool TraceTrailGround(UWorld& World, const FVector& NearLocation, float WalkableFloorZ,
        const FCollisionQueryParams& Query, FHitResult& Floor)
    {
        return World.LineTraceSingleByChannel(Floor, NearLocation + FVector(0, 0, 55),
            NearLocation - FVector(0, 0, 85), ECC_Pawn, Query) && Floor.ImpactNormal.Z >= WalkableFloorZ;
    }

    void AppendRoutePoint(TArray<FVector>& Points, TArray<double>& Distances, const FVector& Point)
    {
        const double Length = Points.IsEmpty() ? 0.0 : FVector::Distance(Points.Last(), Point);
        if (!Points.IsEmpty() && Length < 1.0) return;
        Distances.Add(Points.IsEmpty() ? 0.0 : Distances.Last() + Length);
        Points.Add(Point);
    }

    // Extend only the straight section that the character capsule can actually traverse.
    void ExtendFallback(APawn& Pawn, const FVector& Target, TArray<FVector>& Points, TArray<double>& Distances)
    {
        const auto* Character = Cast<ACharacter>(&Pawn);
        if (!Character || Points.IsEmpty()) return;
        const auto* Capsule = Character->GetCapsuleComponent();
        const auto* Movement = Character->GetCharacterMovement();
        const double Radius = Capsule->GetScaledCapsuleRadius();
        const double HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        const auto Shape = FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, HalfHeight - 2.0));
        const FVector Height(0, 0, HalfHeight + 3.0);
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(QuestTrailFallback), false, &Pawn);
        for (double Length = 0; Length < MaxTrailDistance; Length += TrailDotSpacing)
        {
            const FVector From = Points.Last();
            const double Remaining = FVector::Dist2D(From, Target);
            if (Remaining < 2.0) break;
            const FVector Sample = From + (Target - From).GetSafeNormal2D() * FMath::Min(Remaining, TrailDotSpacing);
            FHitResult Floor;
            if (!TraceTrailGround(*Pawn.GetWorld(), Sample, Movement->GetWalkableFloorZ(), Query, Floor) ||
                FMath::Abs(Floor.ImpactPoint.Z - From.Z) > Movement->MaxStepHeight ||
                Pawn.GetWorld()->SweepTestByChannel(From + Height, Floor.ImpactPoint + Height,
                    FQuat::Identity, ECC_Pawn, Shape, Query)) break;
            AppendRoutePoint(Points, Distances, Floor.ImpactPoint);
        }
    }

    double ProjectTrailProgress(const TArray<FVector>& Points, const TArray<double>& Distances,
        const FVector& Feet, int32& Segment, double& Error)
    {
        // Follow adjacent segments, including when backtracking. Do not jump across a hairpin.
        double Progress = 0.0;
        Error = TNumericLimits<double>::Max();
        auto Consider = [&](int32 Index)
        {
            const FVector Closest = FMath::ClosestPointOnSegment(Feet, Points[Index], Points[Index + 1]);
            const double Candidate = FVector::DistSquared(Feet, Closest);
            if (Candidate >= Error) return false;
            Error = Candidate;
            Segment = Index;
            Progress = Distances[Index] + FVector::Distance(Points[Index], Closest);
            return true;
        };
        Consider(Segment);
        while (Segment + 2 < Points.Num() && Consider(Segment + 1)) {}
        while (Segment > 0 && Consider(Segment - 1)) {}
        Error = FMath::Sqrt(Error);
        return Progress;
    }

    UProceduralMeshComponent* CreateQuestTrail(APawn& Pawn, UMaterialInstanceDynamic* Material)
    {
        auto* Trail = NewObject<UProceduralMeshComponent>(&Pawn, NAME_None, RF_Transient);
        Trail->SetMobility(EComponentMobility::Movable);
        Trail->SetCanEverAffectNavigation(false);
        Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Trail->SetGenerateOverlapEvents(false);
        Trail->SetCastShadow(false);
        Trail->SetOnlyOwnerSee(true);
        Trail->SetHiddenInSceneCapture(true);
        Trail->SetMaterial(0, Material);
        Trail->RegisterComponent();
        return Trail;
    }

    void BuildTrailWindow(APawn& Pawn, UProceduralMeshComponent& Trail, int32 First,
        const TArray<FVector>& Points, const TArray<double>& Distances, TMap<int32,FHitResult>& Ground)
    {
        TArray<FVector> Verts, Normals;
        TArray<int32> Indices;
        TArray<FVector2D> UV, RouteUV;
        const FVector Origin = Points[0];
        const auto* Character = Cast<ACharacter>(&Pawn);
        const float FloorZ = Character ? Character->GetCharacterMovement()->GetWalkableFloorZ() : 0.7f;
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(QuestTrailFloor), false, &Pawn);
        for (int32 Index = First; Index < First + TrailWindowCount; ++Index)
        {
            const double At = TrailFirstDotDistance + Index * TrailDotSpacing;
            if (At > Distances.Last()) break;
            const int32 Segment = FMath::Min(Algo::LowerBound(Distances, At), Points.Num() - 1) - 1;
            const FVector Direction = (Points[Segment + 1] - Points[Segment]).GetSafeNormal();
            if (!Ground.Contains(Index))
            {
                FHitResult Floor;
                if (!TraceTrailGround(*Pawn.GetWorld(), Points[Segment] + Direction * (At - Distances[Segment]),
                    FloorZ, Query, Floor)) Floor = FHitResult();
                Ground.Add(Index, Floor);
            }
            const auto& Floor = Ground.FindChecked(Index);
            if (!Floor.bBlockingHit) continue;
            const FVector N = Floor.ImpactNormal;
            const FVector F = FVector::VectorPlaneProject(Direction, N).GetSafeNormal();
            const FVector R = FVector::CrossProduct(N, F);
            const FVector Center = Floor.ImpactPoint - Origin + N * 2.0;
            const int32 Base = Verts.Num();
            for (const FVector2D Corner : {FVector2D(0,0), FVector2D(1,0), FVector2D(1,1), FVector2D(0,1)})
            {
                Verts.Add(Center + R * ((Corner.X - .5) * 40.) + F * ((Corner.Y - .5) * 24.));
                Normals.Add(N);
                UV.Add(Corner);
                RouteUV.Emplace(At, 0);
            }
            Indices.Append({Base,Base+1,Base+2,Base,Base+2,Base+3});
        }
        Trail.SetWorldLocation(Origin);
        Trail.CreateMeshSection(0, Verts, Indices, Normals, UV, RouteUV, {}, {}, {}, {}, false);
    }

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
    RefreshQuestGuidance(DeltaTime);
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

void UTunaSweeperGameHudWidget::ClearQuestTrailRoute()
{
    QuestTrailNavPath = nullptr;
    QuestTrailPoints.Reset();
    QuestTrailDistances.Reset();
    QuestTrailGround.Reset();
    QuestTrailSegment = 0;
    QuestTrailWindow = INDEX_NONE;
    QuestTrailRetryAt = 0;
    bQuestTrailFallback = false;
    if (auto* Trail = QuestGuidanceTrail.Get()) Trail->SetVisibility(false);
}

void UTunaSweeperGameHudWidget::ResetQuestGuidance()
{
    ClearQuestTrailRoute();
    QuestTrailMaterial = nullptr;
    QuestGuidanceTarget.Reset();
    QuestGuidancePawn.Reset();
    QuestGuidanceScanCountdown = 0.0f;
    if (auto* Trail = QuestGuidanceTrail.Get()) Trail->DestroyComponent();
    QuestGuidanceTrail.Reset();
}

void UTunaSweeperGameHudWidget::RefreshQuestGuidance(float DeltaTime)
{
    auto* Controller = GetOwningPlayer();
    APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
    if (QuestGuidancePawn.Get() != Pawn) ResetQuestGuidance();
    const auto* Character = Cast<ATunaSweeperTopDownCharacter>(Pawn);
    if (!Pawn || ActiveHudMode != ETunaSweeperHudMode::None || IsGameplayBottomHudSuppressed() ||
        (Character && Character->IsDead()))
    {
        ResetQuestGuidance();
        return;
    }
    QuestGuidancePawn = Pawn;
    QuestGuidanceScanCountdown -= FMath::Max(0.0f, DeltaTime);
    if (QuestGuidanceScanCountdown <= 0.0f)
    {
        QuestGuidanceScanCountdown = QuestGuidanceRefreshSeconds;
        TArray<FVector> Locations;
        QuestGuidanceLocations::Get(Pawn->GetWorld(), Locations);
        auto Target = FindClosestQuestGuidanceLocation(Pawn->GetActorLocation(), Locations);
        if (Target.IsSet() != QuestGuidanceTarget.IsSet() ||
            (Target.IsSet() && FVector::DistSquared(Target.GetValue(), QuestTrailTarget) > FMath::Square(75.0)))
        {
            ClearQuestTrailRoute();
            if (Target.IsSet()) QuestTrailTarget = Target.GetValue();
        }
        QuestGuidanceTarget = Target;
    }
    if (!QuestGuidanceTarget.IsSet()) return;
    if (!bQuestGuidanceIconLoadAttempted)
    {
        bQuestGuidanceIconLoadAttempted = true;
        QuestGuidanceIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Quest/T_QuestGuidanceQ.T_QuestGuidanceQ"));
    }

    const FVector Feet = Pawn->GetNavAgentLocation();
    const FVector Target = QuestGuidanceTarget.GetValue();
    const double Now = Pawn->GetWorld()->GetTimeSeconds();
    if (QuestTrailNavPath && !QuestTrailNavPath->IsValid()) ClearQuestTrailRoute();
    double Error = 0.0;
    double Progress = 0.0;
    if (QuestTrailPoints.Num() >= 2)
    {
        Progress = ProjectTrailProgress(QuestTrailPoints, QuestTrailDistances, Feet, QuestTrailSegment, Error);
        if (Error > 95.0 && FVector::DistSquared(Feet, QuestTrailLastQuery) > FMath::Square(35.0))
            ClearQuestTrailRoute();
    }
    if (QuestTrailPoints.Num() < 2)
    {
        if (Now < QuestTrailRetryAt && FVector::DistSquared(Feet, QuestTrailLastQuery) < FMath::Square(95.0)) return;
        ClearQuestTrailRoute();
        QuestTrailRetryAt = Now + 2.0;
        QuestTrailLastQuery = Feet;
        Progress = 0;
        auto* Nav = UNavigationSystemV1::GetCurrent(Pawn->GetWorld());
        const auto& Agent = Pawn->GetNavAgentPropertiesRef();
        FNavLocation Start, End;
        if (Nav && Nav->ProjectPointToNavigation(Feet, Start, FVector(75,75,135), &Agent))
        {
            if (!Nav->ProjectPointToNavigation(Target, End, FVector(200,200,250), &Agent)) return;
            auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(Pawn->GetWorld(), Start.Location, End.Location, Pawn);
            if (!Path || !Path->IsValid() || Path->IsPartial()) return;
            Path->EnableRecalculationOnInvalidation(ENavigationOptionFlag::Disable);
            QuestTrailNavPath = Path;
            for (const FVector& Point : Path->PathPoints) AppendRoutePoint(QuestTrailPoints, QuestTrailDistances, Point);
        }
        else
        {
            bQuestTrailFallback = true;
            AppendRoutePoint(QuestTrailPoints, QuestTrailDistances, Feet);
            ExtendFallback(*Pawn, Target, QuestTrailPoints, QuestTrailDistances);
        }
        if (QuestTrailPoints.Num() < 2) return;
    }

    if (bQuestTrailFallback && Now >= QuestTrailRetryAt &&
        QuestTrailDistances.Last() - Progress < 1300 && FVector::Dist2D(QuestTrailPoints.Last(), Target) > 2)
    {
        const int32 PreviousCount = QuestTrailPoints.Num();
        ExtendFallback(*Pawn, Target, QuestTrailPoints, QuestTrailDistances);
        if (PreviousCount != QuestTrailPoints.Num()) QuestTrailWindow = INDEX_NONE;
        QuestTrailRetryAt = Now + 2.0;
    }
    if (!QuestGuidanceTrail.IsValid())
    {
        auto* Surface = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/UI/Quest/M_QuestTrailChevron.M_QuestTrailChevron"));
        if (!Surface) return;
        QuestTrailMaterial = UMaterialInstanceDynamic::Create(Surface, this);
        QuestGuidanceTrail = CreateQuestTrail(*Pawn, QuestTrailMaterial);
    }
    // Overlap successive windows so the shader fades both ends before geometry is replaced.
    const int32 Window = FMath::Max(0, FMath::FloorToInt(Progress / (TrailDotSpacing * TrailWindowStep)) - 1) * TrailWindowStep;
    if (Window != QuestTrailWindow)
    {
        BuildTrailWindow(*Pawn, *QuestGuidanceTrail.Get(), Window, QuestTrailPoints, QuestTrailDistances, QuestTrailGround);
        QuestTrailWindow = Window;
        QuestGuidanceTrail->SetVisibility(true);
    }
    QuestTrailMaterial->SetScalarParameterValue(TEXT("Progress"), Progress);
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
    auto ProjectMarker = [&](const FVector& Location, TunaSweeperDeliveryIndicator::FPlacement& Placement,
        bool bOffscreenOnly = false)
    {
        FVector2D Projected;
        const bool bProjected = FSceneView::ProjectWorldToScreen(Location, Rect, ViewProjection, Projected);
        if (bProjected)
        {
            Projected = (Projected - FVector2D(Rect.Min)) * Size / FVector2D(Rect.Size());
            if (bOffscreenOnly && Projected.X >= 0 && Projected.Y >= 0 &&
                Projected.X <= Size.X && Projected.Y <= Size.Y) return false;
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
    auto ReserveMarkerCenter = [&](TunaSweeperDeliveryIndicator::FPlacement& Placement)
    {
        // If markers share a bearing, use separate inward lanes without rotating their arrows.
        for (int32 Attempt = 0; Attempt < DrawnCenters.Num(); ++Attempt)
        {
            if (!DrawnCenters.ContainsByPredicate([&](const FVector2D& Other) { return FVector2D::Distance(Other, Placement.Position) < 68; })) break;
            Placement.Position -= Placement.bAtEdge ? Placement.Direction * 68 : FVector2D(0,68);
            Placement.Position.X = FMath::Clamp(Placement.Position.X, 48.0, Size.X - 48.0);
            Placement.Position.Y = FMath::Clamp(Placement.Position.Y, 48.0, Size.Y - 48.0);
        }
        DrawnCenters.Add(Placement.Position);
    };
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

    TunaSweeperDeliveryIndicator::FPlacement QuestPlacement;
    if (ActiveHudMode == ETunaSweeperHudMode::None && QuestGuidanceIcon && QuestGuidanceTarget.IsSet() &&
        ProjectMarker(QuestGuidanceTarget.GetValue(), QuestPlacement, true))
    {
        ReserveMarkerCenter(QuestPlacement);
        TunaSweeperDeliveryIndicator::DrawMarker(QuestPlacement, QuestGuidanceIcon.Get(), Geometry, Elements, Layer);
    }

    for (const auto& Target : DeliveryIndicatorTargets)
    {
        const auto* Destination = Target.Destination.Get();
        int32 ItemId = 0;
        if (!TunaSweeperDeliveryIndicator::TryGetItemId(Destination, ItemId)) continue;
        const FVector Location = Destination->Interactable ? Destination->Interactable->GetInteractionLocation() : Destination->GetActorLocation();
        TunaSweeperDeliveryIndicator::FPlacement Placement;
        if (!ProjectMarker(Location, Placement)) continue;
        ReserveMarkerCenter(Placement);
        const auto* Icon = DeliveryIndicatorIcons.Find(ItemId);
        TunaSweeperDeliveryIndicator::DrawMarker(Placement, Icon ? Icon->Get() : nullptr, Geometry, Elements, Layer);
    }
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperQuestGuidancePresentationTest,
    "TunaSweeper.Quest.GuidancePresentation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTunaSweeperQuestGuidancePresentationTest::RunTest(const FString& Parameters)
{
    const FVector FirstTarget(-300, 0, 0);
    const FVector SecondTarget(300, 0, 0);
    const TArray<FVector> Targets = {FirstTarget, SecondTarget};
    TestEqual(TEXT("Player nearer the first target selects it"),
        FindClosestQuestGuidanceLocation(FVector(-100, 0, 0), Targets).Get(FVector::ZeroVector), FirstTarget);
    TestEqual(TEXT("Walking toward the other target switches guidance"),
        FindClosestQuestGuidanceLocation(FVector(100, 0, 0), Targets).Get(FVector::ZeroVector), SecondTarget);
    TestEqual(TEXT("An equal-distance tie selects the first candidate"),
        FindClosestQuestGuidanceLocation(FVector::ZeroVector, Targets).Get(FVector::ZeroVector), FirstTarget);
    TestFalse(TEXT("No destinations yields no guidance"),
        FindClosestQuestGuidanceLocation(FVector::ZeroVector, {}).IsSet());

    TArray<FVector> Points;
    TArray<double> Distances;
    for (auto Point : {FVector(0,0,0), FVector(200,0,0), FVector(200,300,0)})
        AppendRoutePoint(Points, Distances, Point);
    int32 Segment = 0;
    double Error = 0;
    TestEqual(TEXT("Progress follows a turn without replanning"),
        ProjectTrailProgress(Points, Distances, FVector(195,100,0), Segment, Error), 300.0);
    TestEqual(TEXT("Lateral deviation"), Error, 5.0);
    TestEqual(TEXT("Backtracking follows the same stored route"),
        ProjectTrailProgress(Points, Distances, FVector(80,0,0), Segment, Error), 80.0);
    TestNotNull(TEXT("Ground chevron material exists"), LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/UI/Quest/M_QuestTrailChevron.M_QuestTrailChevron")));
    return true;
}
#endif
