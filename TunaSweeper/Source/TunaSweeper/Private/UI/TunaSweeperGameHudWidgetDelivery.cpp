#include "TunaSweeperGameHudWidgetShared.h"
#include "TunaSweeperDeliveryIndicator.h"

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
    constexpr double TrailDotRadius = 8.0;
    constexpr int32 MaxTrailDots = 32;
    constexpr double MaxTrailDistance = TrailFirstDotDistance + (MaxTrailDots - 1) * TrailDotSpacing;
    constexpr int32 TrailDotSides = 12;

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

    void BuildQuestTrail(APawn& Pawn, const FVector& Target, TArray<FHitResult>& OutDots)
    {
        UWorld& World = *Pawn.GetWorld();
        const auto* Character = Cast<ACharacter>(&Pawn);
        const auto* Movement = Character ? Character->GetCharacterMovement() : nullptr;
        const float WalkableFloorZ = Movement ? Movement->GetWalkableFloorZ() : 0.7f;
        const FVector Start = Pawn.GetNavAgentLocation();
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(QuestGuidanceTrail), false, &Pawn);

        auto* Navigation = UNavigationSystemV1::GetCurrent(&World);
        FNavLocation NavStart;
        const auto& Agent = Pawn.GetNavAgentPropertiesRef();
        if (Navigation && Navigation->ProjectPointToNavigation(Start, NavStart, FVector(75, 75, 135), &Agent))
        {
            FNavLocation NavEnd;
            if (!Navigation->ProjectPointToNavigation(Target, NavEnd, FVector(200, 200, 250), &Agent)) return;
            const auto* Path = UNavigationSystemV1::FindPathToLocationSynchronously(
                &World, NavStart.Location, NavEnd.Location, &Pawn);
            if (!Path || !Path->IsValid() || Path->IsPartial()) return;

            double SegmentStartDistance = 0.0;
            double DotDistance = TrailFirstDotDistance;
            for (int32 Index = 1; Index < Path->PathPoints.Num() && DotDistance <= MaxTrailDistance; ++Index)
            {
                const FVector& From = Path->PathPoints[Index - 1];
                const FVector& To = Path->PathPoints[Index];
                const double Length = FVector::Dist2D(From, To);
                if (Length <= UE_KINDA_SMALL_NUMBER) continue;
                while (DotDistance <= FMath::Min(SegmentStartDistance + Length, MaxTrailDistance))
                {
                    const FVector Sample = FMath::Lerp(From, To, (DotDistance - SegmentStartDistance) / Length);
                    FHitResult Floor;
                    if (TraceTrailGround(World, Sample, WalkableFloorZ, Query, Floor)) OutDots.Add(Floor);
                    DotDistance += TrailDotSpacing;
                }
                SegmentStartDistance += Length;
            }
            return;
        }

        // Maps without a navmesh can show the straight portion proven walkable by the player capsule.
        const UCapsuleComponent* Capsule = Character ? Character->GetCapsuleComponent() : nullptr;
        FHitResult PreviousFloor;
        if (!Capsule || !Movement || !TraceTrailGround(World, Start, WalkableFloorZ, Query, PreviousFloor)) return;
        const float Radius = Capsule->GetScaledCapsuleRadius();
        const float HalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        const FCollisionShape Clearance = FCollisionShape::MakeCapsule(Radius, FMath::Max(Radius, HalfHeight - 2.0f));
        const FVector Height(0, 0, HalfHeight + 3.0f);
        const FVector Direction = (Target - Start).GetSafeNormal2D();
        const double MaxDistance = FMath::Min(FVector::Dist2D(Start, Target), MaxTrailDistance);
        for (double Distance = TrailDotSpacing; Distance <= MaxDistance; Distance += TrailDotSpacing)
        {
            FVector Sample = Start + Direction * Distance;
            Sample.Z = PreviousFloor.ImpactPoint.Z;
            FHitResult Floor;
            if (!TraceTrailGround(World, Sample, WalkableFloorZ, Query, Floor) ||
                FMath::Abs(Floor.ImpactPoint.Z - PreviousFloor.ImpactPoint.Z) > Movement->MaxStepHeight ||
                World.SweepTestByChannel(PreviousFloor.ImpactPoint + Height, Floor.ImpactPoint + Height,
                    FQuat::Identity, ECC_Pawn, Clearance, Query))
            {
                break;
            }
            if (Distance >= TrailFirstDotDistance) OutDots.Add(Floor);
            PreviousFloor = Floor;
        }
    }

    UProceduralMeshComponent* CreateQuestTrail(APawn& Pawn)
    {
        auto* Trail = NewObject<UProceduralMeshComponent>(&Pawn, NAME_None, RF_Transient);
        Trail->SetMobility(EComponentMobility::Movable);
        Trail->SetCanEverAffectNavigation(false);
        Trail->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Trail->SetGenerateOverlapEvents(false);
        Trail->SetCastShadow(false);
        Trail->SetOnlyOwnerSee(true);
        Trail->SetHiddenInSceneCapture(true);
        if (auto* Surface = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/UI/WorldQuestMarker/M_QuestMarkerSurface.M_QuestMarkerSurface")))
        {
            auto* Material = UMaterialInstanceDynamic::Create(Surface, Trail);
            Material->SetVectorParameterValue(TEXT("MarkerColor"), FLinearColor(1.0f, 0.76f, 0.22f));
            Material->SetScalarParameterValue(TEXT("Metallic"), 0.0f);
            Material->SetScalarParameterValue(TEXT("Roughness"), 0.5f);
            Material->SetScalarParameterValue(TEXT("EmissiveStrength"), 0.75f);
            Trail->SetMaterial(0, Material);
        }
        Trail->RegisterComponent();
        return Trail;
    }

    void BuildQuestTrailMesh(const FVector& Origin, TConstArrayView<FHitResult> Dots,
        TArray<FVector>& Vertices, TArray<FVector>& Normals, TArray<int32>& Triangles)
    {
        Vertices.Reserve(Dots.Num() * (TrailDotSides + 1));
        Normals.Reserve(Vertices.Max());
        Triangles.Reserve(Dots.Num() * TrailDotSides * 3);

        for (const FHitResult& Dot : Dots)
        {
            const FVector Normal = Dot.ImpactNormal.GetSafeNormal();
            const FVector Center = Dot.ImpactPoint + Normal * 2.0 - Origin;
            const FQuat Rotation = FQuat::FindBetweenNormals(FVector::UpVector, Normal);
            const int32 Base = Vertices.Add(Center);
            Normals.Add(Normal);
            for (int32 Side = 0; Side < TrailDotSides; ++Side)
            {
                const double Angle = UE_DOUBLE_TWO_PI * Side / TrailDotSides;
                const FVector Radius(TrailDotRadius * FMath::Cos(Angle), TrailDotRadius * FMath::Sin(Angle), 0.0);
                Vertices.Add(Center + Rotation.RotateVector(Radius));
                Normals.Add(Normal);
                // Unreal's front-face order gives an outward normal from Edge2 cross Edge1.
                Triangles.Add(Base);
                Triangles.Add(Base + (Side + 1) % TrailDotSides + 1);
                Triangles.Add(Base + Side + 1);
            }
        }
    }

    void DrawQuestTrail(UProceduralMeshComponent& Trail, const FVector& Origin, const TArray<FHitResult>& Dots)
    {
        TArray<FVector> Vertices;
        TArray<FVector> Normals;
        TArray<int32> Triangles;
        BuildQuestTrailMesh(Origin, Dots, Vertices, Normals, Triangles);
        Trail.SetWorldLocation(Origin);
        Trail.CreateMeshSection(0, Vertices, Triangles, Normals, {}, {}, {}, false);
        Trail.SetVisibility(true);
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

void UTunaSweeperGameHudWidget::ResetQuestGuidance()
{
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
    if (QuestGuidanceScanCountdown > 0.0f) return;
    QuestGuidanceScanCountdown = QuestGuidanceRefreshSeconds;

    TArray<FVector> Locations;
    QuestGuidanceLocations::Get(Pawn->GetWorld(), Locations);
    const FVector Origin = Pawn->GetActorLocation();
    QuestGuidanceTarget = FindClosestQuestGuidanceLocation(Origin, Locations);

    TArray<FHitResult> Dots;
    if (QuestGuidanceTarget.IsSet())
    {
        if (!bQuestGuidanceIconLoadAttempted)
        {
            bQuestGuidanceIconLoadAttempted = true;
            QuestGuidanceIcon = LoadObject<UTexture2D>(nullptr, TEXT("/Game/UI/Quest/T_QuestGuidanceQ.T_QuestGuidanceQ"));
        }
        BuildQuestTrail(*Pawn, QuestGuidanceTarget.GetValue(), Dots);
    }
    if (Dots.IsEmpty())
    {
        if (auto* Trail = QuestGuidanceTrail.Get()) Trail->SetVisibility(false);
        return;
    }
    if (!QuestGuidanceTrail.IsValid()) QuestGuidanceTrail = CreateQuestTrail(*Pawn);
    DrawQuestTrail(*QuestGuidanceTrail.Get(), Origin, Dots);
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

    TArray<FHitResult> Dots;
    FHitResult& Flat = Dots.AddDefaulted_GetRef();
    Flat.ImpactPoint = FVector(300, 400, 500);
    Flat.ImpactNormal = FVector::UpVector;
    FHitResult& Slope = Dots.AddDefaulted_GetRef();
    Slope.ImpactPoint = FVector(550, 600, 650);
    Slope.ImpactNormal = FVector(0.3, -0.2, 1).GetSafeNormal();
    const FVector Origin(150, 200, 300);
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<int32> Triangles;
    BuildQuestTrailMesh(Origin, Dots, Vertices, Normals, Triangles);
    if (!TestEqual(TEXT("Both dots have a center and a complete rim"), Vertices.Num(), Dots.Num() * (TrailDotSides + 1)) ||
        !TestEqual(TEXT("Each vertex has a normal"), Normals.Num(), Vertices.Num()) ||
        !TestEqual(TEXT("Both dots have a complete triangle fan"), Triangles.Num(), Dots.Num() * TrailDotSides * 3))
    {
        return false;
    }
    for (int32 DotIndex = 0; DotIndex < Dots.Num(); ++DotIndex)
    {
        const FHitResult& Dot = Dots[DotIndex];
        const FVector ExpectedCenter = Dot.ImpactPoint + 2.0 * Dot.ImpactNormal - Origin;
        TestEqual(TEXT("Dot center follows the normal just above the ground"),
            Vertices[DotIndex * (TrailDotSides + 1)], ExpectedCenter);
        bool bAllFacesPointUp = true;
        for (int32 Side = 0; Side < TrailDotSides; ++Side)
        {
            const int32 Index = (DotIndex * TrailDotSides + Side) * 3;
            const FVector& A = Vertices[Triangles[Index]];
            const FVector& B = Vertices[Triangles[Index + 1]];
            const FVector& C = Vertices[Triangles[Index + 2]];
            bAllFacesPointUp &= FVector::DotProduct(FVector::CrossProduct(C - A, B - A), Dot.ImpactNormal) > 0.0;
        }
        TestTrue(TEXT("Every face winds toward the ground normal"), bAllFacesPointUp);
    }
    return true;
}
#endif
