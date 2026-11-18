#if WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Engine/StaticMesh.h"
#include "Widgets/SWindow.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Camera/CameraTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "ImageUtils.h"
#include "ImageCore.h"
#include "UnrealClient.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Interaction/TunaSweeperLadderTransferActor.h"
#include "Interaction/TunaSweeperRaidStorageActor.h"
#include "Scenario/TunaSweeperEndingActor.h"
#include "Scenario/SSTOLaunchCinematic.h"
#include "UI/Astra/EndingMonologueGPT62Astra.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace SSTOEndingPlay
{
template<class T> T* Find(UWorld* World, const TCHAR* Label = nullptr)
{
    if (World) for (TActorIterator<T> It(World); It; ++It)
        if (!Label || It->GetActorLabel() == Label) return *It;
    return nullptr;
}
void Capture(FAutomationTestBase* Test, const FString& Name)
{
    auto* View = GEditor->PlayWorld ? GEditor->PlayWorld->GetGameViewport() : nullptr;
    if (!Test->TestNotNull(TEXT("Rendered PIE viewport"), View)) return;
    const auto Window = View->GetWindow();
    if (!Test->TestTrue(TEXT("Rendered PIE window"), Window.IsValid())) return;
    TArray<FColor> Pixels;
    FIntVector Size;
    if (!Test->TestTrue(TEXT("Read rendered world and Slate UI"), FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size))) return;
    TArray64<uint8> Png;
    FImageUtils::PNGCompressImageArray(Size.X, Size.Y, TArrayView64<const FColor>(Pixels.GetData(), Pixels.Num()), Png);
    const FString Dir = FPaths::ProjectSavedDir() / TEXT("SSTOEnding");
    IFileManager::Get().MakeDirectory(*Dir, true);
    Test->TestTrue(TEXT("Saved gameplay screenshot"), FFileHelper::SaveArrayToFile(Png, *(Dir / (Name + TEXT(".png")))));
}
struct FRun
{
    FAutomationTestBase* Test;
    int32 Stage = 0;
    bool bTrue = false;
    bool bCapturedStory = false;
    bool bSawBlack = false;
    int32 CapturedShots = 0;
    int32 ObservedShot = INDEX_NONE;
    FTransform FixedCamera;
    FTransform ParkedShip;
    TWeakObjectPtr<UStaticMeshComponent> BoardingStair;
    TWeakObjectPtr<UStaticMeshComponent> PressureDoor;
    bool bSawStairLift = false;
    bool bDoorSwungShut = false;
    bool bSawIntermediatePitch = false;
    bool bReachedUpright = false;
    float PreviousHatchSample = -1.f;
    FVector PreviousStairFoot = FVector::ZeroVector;
    const bool bRecordFilm = FParse::Param(FCommandLine::Get(), TEXT("RecordSSTOEnding"));
    double LastFilmFrame = -1.0;
    FString FilmDirectory;
    double Changed = FPlatformTime::Seconds();
    double LastKey = 0;
    FGuid Carried;
    void Next(int32 Value)
    {
        Stage = Value; Changed = FPlatformTime::Seconds();
        Test->AddInfo(FString::Printf(TEXT("SSTO stage=%d true=%d"),Stage,bTrue));
    }
    bool Check(bool Value, const TCHAR* Message) { return Test->TestTrue(Message, Value); }
    bool RecordFilmFrame(UWorld* World)
    {
        const double Seconds = FPlatformTime::Seconds() - Changed;
        if (!bRecordFilm || Seconds - LastFilmFrame < 1.0 / 30.0) return true;
        LastFilmFrame = Seconds;
        if (FilmDirectory.IsEmpty())
        {
            FilmDirectory = FPaths::ProjectSavedDir() / TEXT("SSTOEnding/Film") / FDateTime::UtcNow().ToString(TEXT("%Y%m%d-%H%M%S"));
            if (!IFileManager::Get().MakeDirectory(*FilmDirectory, true)) return false;
        }
        auto* Viewport = World->GetGameViewport();
        if (!Viewport) return false;
        const auto Window = Viewport->GetWindow();
        TArray<FColor> Pixels;
        FIntVector Size(0, 0, 0);
        TArray64<uint8> Frame;
        if (!Window.IsValid() || !FSlateApplication::Get().TakeScreenshot(Window.ToSharedRef(), Pixels, Size)
            || Size.X <= 0 || Size.Y <= 0 || Pixels.Num() != Size.X * Size.Y) return false;
        const FString File = FilmDirectory / FString::Printf(TEXT("%08d.jpg"), FMath::RoundToInt(Seconds * 1000.0));
        return FImageUtils::CompressImage(Frame, TEXT("jpg"), FImageView(Pixels.GetData(), Size.X, Size.Y), 92)
            && FFileHelper::SaveArrayToFile(Frame, *File);
    }
    bool InspectHatch(ASSTOLaunchCinematic* Launch, int32 Shot)
    {
        if (!BoardingStair.IsValid() || !PressureDoor.IsValid())
        {
            TInlineComponentArray<UStaticMeshComponent*> Parts(Launch);
            for (auto* Part : Parts)
            {
                const FName Mesh = GetFNameSafe(Part->GetStaticMesh());
                if (Mesh == TEXT("SM_SSTO_StairTray") || Mesh == TEXT("SM_SSTO_Ladder")) BoardingStair = Part;
                else if (Mesh == TEXT("SM_SSTO_CinematicHatch")) PressureDoor = Part;
            }
            if (!Check(BoardingStair.IsValid() && PressureDoor.IsValid(), TEXT("Both moving boarding parts are present"))) return false;
        }
        if (!Check(BoardingStair->IsVisible() && !BoardingStair->bHiddenInGame,
            TEXT("The physical stair stays rendered across the mechanical cut"))) return false;
        const FVector Foot = BoardingStair->GetRelativeTransform().TransformPosition(FVector(395, -598, 0));
        const float Now = Launch->GetElapsedSeconds();
        if (Shot == 0)
        {
            if (PreviousHatchSample >= 0.f && !Check(FVector::Dist(PreviousStairFoot, Foot) <= 600. * (Now - PreviousHatchSample) + 2.,
                TEXT("The foot moves continuously while the camera shows it"))) return false;
            PreviousStairFoot = Foot;
            PreviousHatchSample = Now;
            bSawStairLift |= Foot.Z > 57. && Foot.Y < -400.;
        }
        else
        {
            if (!Check(bSawStairLift && Foot.Y > -300. && Foot.Z < 165., TEXT("After the cut the actual stair is inside the belly below the cabin deck"))) return false;
            if (PressureDoor->GetRelativeTransform().Equals(FTransform::Identity, .01f)) bDoorSwungShut = true;
        }
        return true;
    }
    bool InspectFlight(ASSTOLaunchCinematic* Launch, int32 Shot)
    {
        TInlineComponentArray<UStaticMeshComponent*> Parts(Launch);
        const UStaticMeshComponent* Hull = nullptr;
        TArray<const UStaticMeshComponent*, TInlineAllocator<4>> LiftCores;
        int32 WashCount = 0;
        for (const auto* Part : Parts)
        {
            if (GetFNameSafe(Part->GetStaticMesh()) == TEXT("SM_SSTO_LowHull")) Hull = Part;
            if (!Part->IsVisible()) continue;
            if (Part->ComponentHasTag(TEXT("SSTO_MainLiftCore")) || Part->ComponentHasTag(TEXT("SSTO_NoseLiftCore"))) LiftCores.Add(Part);
            if (Part->ComponentHasTag(TEXT("SSTO_GroundWash"))) ++WashCount;
        }
        if (!Check(Hull != nullptr, TEXT("Flight uses the modeled hull"))) return false;
        const double NoseUp = Hull->GetForwardVector().Z;
        if (Shot == 2 && Launch->GetElapsedSeconds() > 7.3f)
        {
            if (!Check(LiftCores.Num() == 4 && WashCount == 4 && FMath::Abs(NoseUp) < .12,
                TEXT("Both main and forward pairs fire together and disturb the ground during level lift"))) return false;
            const FVector Outlets[] = {{-810,-204,94},{-810,204,94},{752,-146,90},{752,146,90}};
            for (const FVector& Outlet : Outlets)
            {
                const FVector Lip = Hull->GetComponentTransform().TransformPosition(Outlet);
                const FName Type = Outlet.X < 0 ? TEXT("SSTO_MainLiftCore") : TEXT("SSTO_NoseLiftCore");
                if (!Check(LiftCores.ContainsByPredicate([&](const UStaticMeshComponent* Core)
                {
                    const FVector Center = Core->GetComponentLocation();
                    return Core->ComponentHasTag(Type) && FVector::Dist2D(Lip, Center) < 1. && Center.Z < Lip.Z;
                }), TEXT("A live jet points straight down from each modeled outlet"))) return false;
            }
        }
        if (Shot == 3)
        {
            bSawIntermediatePitch |= NoseUp > .25 && NoseUp < .85;
            bReachedUpright |= NoseUp > .995;
            if (!Check(Hull->GetComponentTransform().TransformPosition(FVector(-1145,0,300)).Z > ParkedShip.GetLocation().Z + 80.,
                TEXT("The tail clears the ground throughout pitch-up"))) return false;
        }
        if (Shot == 4)
        {
            FMinimalViewInfo View;
            Launch->CalcCamera(0.f, View);
            const FVector Up = View.Rotation.RotateVector(FVector::UpVector);
            const double VerticalTangent = FMath::Tan(FMath::DegreesToRadians(View.FOV * .5)) / View.AspectRatio;
            auto ScreenY = [&](const FVector& Local)
            {
                const FVector Ray = Hull->GetComponentTransform().TransformPosition(Local) - View.Location;
                return FVector::DotProduct(Ray, Up) / (FVector::DotProduct(Ray, View.Rotation.Vector()) * VerticalTangent);
            };
            const double Nose = ScreenY(FVector(1260,0,300)), Tail = ScreenY(FVector(-1145,0,300));
            return Check(bSawIntermediatePitch && bReachedUpright && NoseUp > .995,
                TEXT("The sky shot inherits the completed nose-up rotation")) &&
                Check(Nose < .98 && Tail > -.98 && (Nose-Tail)*.5 > .30 && (Nose-Tail)*.5 < .68,
                    TEXT("The entire upright craft reads at medium distance against the sky"));
        }
        return true;
    }
    bool InspectDeparture(UWorld* World, APlayerController* PC, UTunaSweeperGameInstance* Game)
    {
        if (!Check(RecordFilmFrame(World), TEXT("Save optional real-time review frame"))) return false;
        if (auto* Launch = Find<ASSTOLaunchCinematic>(World))
        {
            FMinimalViewInfo View;
            Launch->CalcCamera(0.f, View);
            const int32 Shot = Launch->GetShotIndex();
            if (Shot <= 1 && !InspectHatch(Launch, Shot)) return false;
            if (Shot >= 2 && !InspectFlight(Launch, Shot)) return false;
            const FTransform Camera(View.Rotation, View.Location);
            if (Shot != ObservedShot)
            {
                if (!Check(Shot == ObservedShot + 1, TEXT("Locked shots play in storyboard order"))) return false;
                if (Shot == 2 && !Check(bDoorSwungShut, TEXT("The fixed hatch shot holds until the door is seated"))) return false;
                if (ObservedShot != INDEX_NONE && !Check(!FixedCamera.Equals(Camera, 1.f), TEXT("The ellipsis and subsequent cuts change camera position"))) return false;
                ObservedShot = Shot;
                FixedCamera = Camera;
            }
            if (!Check(Camera.Equals(FixedCamera, .01), TEXT("Camera stays fixed during each shot"))) return false;
            const float CaptureTimes[] = {1.97f, 3.68f, 8.65f, 13.35f, 18.55f, 23.5f};
            const TCHAR* Names[] = {TEXT("True_00_Stair"), TEXT("True_01_Hatch"), TEXT("True_02_Hover"), TEXT("True_02_Pitch"),
                TEXT("True_03_Clouds"), TEXT("True_04_Orbit")};
            if (Shot == CapturedShots && Launch->GetElapsedSeconds() >= CaptureTimes[Shot])
            {
                if (!Check(PC->GetViewTarget() == Launch, TEXT("Game camera sees the real-time launch")) ||
                    !Check(UGameplayStatics::IsGamePaused(World), TEXT("Raid stays paused during the launch"))) return false;
                const auto* Ship = Find<AActor>(World,TEXT("TS_Plains_SSTOLander"));
                if (!Check(Ship && Ship->GetActorTransform().Equals(ParkedShip), TEXT("Gameplay ship stays on its landing site"))) return false;
                Capture(Test, Names[Shot]);
                ++CapturedShots;
            }
            return true;
        }
        TArray<UUserWidget*> Cards;
        UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Cards,UEndingMonologueGPT62Astra::StaticClass(),true);
        if (Cards.IsEmpty()) return true;
        auto* Text = Cast<UTextBlock>(Cards[0]->GetWidgetFromName(TEXT("MonologueText")));
        if (!Check(Text != nullptr,TEXT("Monologue has an editable text block"))) return false;
        if (!bSawBlack && Text->GetRenderOpacity() == 0.f)
        {
            Capture(Test, TEXT("True_05_Black"));
            bSawBlack = true;
        }
        if (!bCapturedStory && Text->GetRenderOpacity() >= .99f)
        {
            const FText Expected = Game->ResolveLocalizedText(TEXT("ui.freeplay_ending.true.monologue"), FText::GetEmpty());
            const auto& CardGeometry = Cards[0]->GetCachedGeometry();
            const auto& TextGeometry = Text->GetCachedGeometry();
            const FVector2D CardCenter = CardGeometry.LocalToAbsolute(CardGeometry.GetLocalSize() * .5f);
            const FVector2D TextCenter = TextGeometry.LocalToAbsolute(TextGeometry.GetLocalSize() * .5f);
            if (!Check(bSawBlack && CapturedShots == 6, TEXT("Clean black beat precedes the monologue after all six fixed shots")) ||
                !Check(!Expected.IsEmpty() && Expected.EqualTo(Text->GetText()), TEXT("Localized storyboard line is displayed")) ||
                !Check(FVector2D::Distance(CardCenter,TextCenter) < 4.0, TEXT("Monologue is centered in both axes"))) return false;
            Capture(Test, TEXT("True_06_Monologue"));
            bCapturedStory = true;
        }
        return true;
    }
    bool WalkToDevice(ACharacter* Pawn, AActor* Ship, AActor* Device)
    {
        UWorld* World = Pawn->GetWorld();
        const auto& Hull = Ship->GetActorTransform();
        const double Half = Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
        const auto Capsule = FCollisionShape::MakeCapsule(Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius(), Half);
        const FCollisionQueryParams Params(SCENE_QUERY_STAT(SSTODeviceApproach), false, Pawn);
        const FVector Front = Hull.InverseTransformPosition(Device->GetActorLocation() + 110.0 * Device->GetActorForwardVector());
        const FVector Feet = Pawn->GetActorLocation() - FVector(0, 0, Half);
        const double StartX = Hull.InverseTransformPosition(Feet).X;
        const FVector Route[] = { FVector(StartX, 0, Front.Z), FVector(Front.X, 0, Front.Z), Front };
        FVector Previous = Pawn->GetActorLocation() + FVector(0, 0, 8);
        FVector SegmentStart = Feet;
        for (const FVector& LocalPoint : Route)
        {
            const FVector End = Hull.TransformPosition(LocalPoint);
            const int32 Steps = FMath::Max(1, FMath::CeilToInt32(FVector::Dist2D(SegmentStart, End) / 40.0));
            for (int32 Step = 1; Step <= Steps; ++Step)
            {
                const FVector Floor = FMath::Lerp(SegmentStart, End, static_cast<double>(Step) / Steps);
                FHitResult Ground, Blocker;
                if (!World->LineTraceSingleByChannel(Ground, Floor + FVector(0,0,35), Floor - FVector(0,0,45), ECC_Pawn, Params)
                    || Ground.GetActor() != Ship || !Pawn->GetCharacterMovement()->IsWalkable(Ground))
                {
                    Test->AddError(FString::Printf(TEXT("Missing walkable deck approaching %s at %s"), *Device->GetActorLabel(), *Floor.ToString()));
                    return false;
                }
                const FVector NextPoint = Ground.ImpactPoint + FVector(0, 0, Half + 10);
                if (World->SweepSingleByChannel(Blocker, Previous, NextPoint, FQuat::Identity, ECC_Pawn, Capsule, Params))
                {
                    Test->AddError(FString::Printf(TEXT("Approach to %s blocked by %s at %s"),
                        *Device->GetActorLabel(), *GetNameSafe(Blocker.GetComponent()), *Blocker.Location.ToString()));
                    return false;
                }
                Previous = NextPoint;
            }
            SegmentStart = End;
        }
        return Check(Pawn->SetActorLocation(Previous), TEXT("Player reaches the tested approach"));
    }
    bool Tick()
    {
        UWorld* World = GEditor->PlayWorld;
        if (FPlatformTime::Seconds() - Changed > 90)
        {
            Test->AddError(FString::Printf(TEXT("SSTO ending timed out at stage %d, true=%d, map=%s"), Stage, bTrue, *GetNameSafe(World)));
            return true;
        }
        if (!World || !World->GetFirstPlayerController()) return false;
        auto* Game = World->GetGameInstance<UTunaSweeperGameInstance>();
        APlayerController* PC = World->GetFirstPlayerController();
        auto* Pawn = Cast<ACharacter>(PC->GetPawn());
        if (!Game) return false;
        if (Stage == 0 || Stage == 5)
        {
            auto* Ending = Find<ATunaSweeperEndingActor>(World);
            auto* Storage = Find<ATunaSweeperRaidStorageActor>(World);
            auto* Entry = Find<ATunaSweeperLadderTransferActor>(World, TEXT("TS_Plains_SSTO_Entry"));
            if (!Pawn || !Ending || !Storage || !Entry) return false;
            if (Stage == 0)
            {
                if (!Check(Game->ActivateSaveSlot(1, true), TEXT("Create isolated slot")) ||
                    !Check(Game->SetActiveSaveSlotDifficultyStage(1), TEXT("Set isolated slot difficulty"))) return true;
                // A real raid save has already completed the opening; this isolated fixture starts directly in RaidPlains.
                Game->MarkScenarioProgressFlag(TEXT("scenario.opening.awakening"), true);
                if (!Check(Game->AddItemToFirstAvailableInventorySlot(3004, 10), TEXT("Seed carried tuna"))) return true;
                for (const auto& Slot : Game->GetInventorySlots())
                {
                    FTunaSweeperItemInstance Item;
                    if (Game->TryGetItemInstance(Slot.ItemUid, Item) && Item.ItemId == 3004) { Carried = Slot.ItemUid; break; }
                }
            }
            if (!Check(Storage->CountLoadedStorageActors() == 1, TEXT("Exactly one shared storage placed"))) return true;
            int32 EndingCount = 0;
            for (TActorIterator<ATunaSweeperEndingActor> It(World); It; ++It) ++EndingCount;
            if (!Check(EndingCount == 1, TEXT("Exactly one ending placed"))) return true;
            const float Half = Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            Pawn->SetActorLocation(Entry->GetArrivalPoint()->GetComponentLocation() + FVector(0,0,Half+2));
            if (!Check(Entry->RequestInteraction(Pawn), TEXT("Enter SSTO through actual ladder interaction"))) return true;
            Next(1); return false;
        }
        if (Stage == 1 && FPlatformTime::Seconds() - Changed > 2)
        {
            auto* Ending = Find<ATunaSweeperEndingActor>(World);
            auto* Storage = Find<ATunaSweeperRaidStorageActor>(World);
            auto* Ship = Find<AActor>(World, TEXT("TS_Plains_SSTOLander"));
            if (!Check(Pawn && Ending && Storage && Ship, TEXT("Interior actors alive"))) return true;
            if (!WalkToDevice(Pawn, Ship, Storage) ||
                !Check(Storage->GetInteractableComponent()->IsWithinInteractionDistance(Pawn), TEXT("Storage reachable from aisle")) ||
                !Check(Storage->GetInteractableComponent()->RequestInteraction(Pawn), TEXT("Actual storage interaction opens"))) return true;
            Next(2); return false;
        }
        if (Stage == 2 && FPlatformTime::Seconds() - Changed > 1)
        {
            auto* Storage = Find<ATunaSweeperRaidStorageActor>(World);
            auto* Ending = Find<ATunaSweeperEndingActor>(World);
            if (!Check(Game->GetActiveLootContainerOwner() == Storage, TEXT("Storage UI uses persistent owner"))) return true;
            if (bTrue)
            {
                for (int32 SlotIndex = 1; SlotIndex < 10; ++SlotIndex)
                {
                    if (!Check(Game->AddItemToFirstAvailableInventorySlot(3004,10), TEXT("Seed full test stack"))) return true;
                    int32 InventoryIndex = INDEX_NONE;
                    const auto& Slots = Game->GetInventorySlots();
                    for (int32 I=0; I<Slots.Num(); ++I)
                    {
                        FTunaSweeperItemInstance Item;
                        if (Slots[I].ItemUid != Carried && Game->TryGetItemInstance(Slots[I].ItemUid,Item) && Item.ItemId==3004 && Item.Quantity==10)
                        { InventoryIndex=I; break; }
                    }
                    FTunaSweeperItemSlotReference From, To;
                    From.Source=ETunaSweeperItemSlotSource::Inventory; From.SlotIndex=InventoryIndex;
                    To.Source=ETunaSweeperItemSlotSource::LootContainer; To.SlotIndex=SlotIndex;
                    const FString SavePath = TunaSweeperBuildFlavor::GetSaveGameDirectory() / TEXT("TunaSweeperSave_Slot01.sav");
                    TArray<uint8> Before, After;
                    FFileHelper::LoadFileToArray(Before,*SavePath);
                    if (!Check(Game->MoveItemBetweenSlots(From,To), TEXT("Inventory transfer persists storage slot"))) return true;
                    // Inspect disk before closing the chest or invoking either ending save.
                    // RaidStorage.Persistence separately validates the saved envelope contents.
                    if (!Check(FFileHelper::LoadFileToArray(After,*SavePath) && !After.IsEmpty() && Before!=After,
                        TEXT("Every transfer immediately updates disk"))) return true;
                }
            }
            Check(Game->IsRaidStorageReadyForTrueEnding()==bTrue, TEXT("Live ending branch matches contents"));
            Check(Ending->GetInteractionDisplayName().EqualTo(Game->ResolveLocalizedText(
                bTrue ? TEXT("ui.freeplay_ending.true_prompt") : TEXT("ui.freeplay_ending.normal_prompt"), FText::GetEmpty())), TEXT("Live localized ending prompt"));
            TInlineComponentArray<UStaticMeshComponent*> Meshes(Storage);
            int32 LEDs=0;
            for (auto* Mesh : Meshes) for (int32 I=0;I<10;++I)
                if (Mesh->ComponentHasTag(FName(*FString::Printf(TEXT("RaidStorageLED_%d"),I))))
                {
                    auto* MID=Cast<UMaterialInstanceDynamic>(Mesh->GetMaterial(0));
                    Check(MID && MID->K2_GetVectorParameterValue(TEXT("IndicatorColor")).Equals((bTrue || I==0) ? Storage->FullTunaColor : FLinearColor::Black),TEXT("Live LED material matches contents"));
                    ++LEDs;
                }
            Check(LEDs==10,TEXT("All ten LED materials inspected"));
            Capture(Test,bTrue ? TEXT("True_Storage") : TEXT("Normal_Storage"));
            auto* Player = Cast<ATunaSweeperPlayerController>(PC);
            if (!Check(Player != nullptr, TEXT("Gameplay controller owns inventory UI"))) return true;
            Player->ToggleInventoryOnlyPanel();
            auto* Ship = Find<AActor>(World, TEXT("TS_Plains_SSTOLander"));
            if (!Check(Pawn && Ship && Ending, TEXT("Terminal and player present after closing storage")) ||
                !Check(Ending->LaunchLander == Ship, TEXT("Placed terminal references its SSTO")) ||
                !WalkToDevice(Pawn, Ship, Ending) ||
                !Check(Ending->IsWithinInteractionDistance(Pawn), TEXT("Ending reachable from aisle")) ||
                !Check(Ending->RequestInteraction(Pawn), TEXT("Actual ending interaction starts"))) return true;
            ParkedShip = Ship->GetActorTransform();
            Next(3); return false;
        }
        if (Stage == 3)
        {
            if (World->GetName().Contains(TEXT("IntroMap")))
            {
                if (bTrue) Check(bCapturedStory && bSawBlack && CapturedShots == 6, TEXT("True ending completed six fixed shots, blackout and monologue"));
                Check(Game->IsScenarioProgressFlagSet(bTrue ? TEXT("ending.freeplay.true_seen") : TEXT("ending.freeplay.normal_seen")), TEXT("Ending completion persisted"));
                Check(!Game->IsScenarioProgressFlagSet(TEXT("demo.ending.farewell_seen")), TEXT("Slot not retired"));
                Check(Game->GetSaveSlotSummary(1).bHasData,TEXT("Same save slot still exists"));
                Next(4); return false;
            }
            if (bTrue) return !InspectDeparture(World, PC, Game);
            if (!Check(!Find<ASSTOLaunchCinematic>(World), TEXT("Normal ending keeps its original presentation"))) return true;
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UTunaSweeperDialogueWidget::StaticClass(),true);
            if (Widgets.IsEmpty() || FPlatformTime::Seconds()-Changed < 2) return false;
            if (!bCapturedStory)
            {
                Capture(Test,bTrue ? TEXT("True_Story") : TEXT("Normal_Story"));
                bCapturedStory=true;
            }
            if (FPlatformTime::Seconds()-LastKey > .35)
            {
                Widgets[0]->SetKeyboardFocus();
                const FKeyEvent Key(EKeys::SpaceBar,FModifierKeysState(),0,false,32,32);
                FSlateApplication::Get().ProcessKeyDownEvent(Key);
                FSlateApplication::Get().ProcessKeyUpEvent(Key);
                LastKey=FPlatformTime::Seconds();
            }
            return false;
        }
        if (Stage == 4 && FPlatformTime::Seconds()-Changed > 2)
        {
            TArray<UUserWidget*> Widgets;
            UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World,Widgets,UTunaSweeperIntroMenuWidget::StaticClass(),true);
            if (Widgets.IsEmpty()) return false;
            const auto* Property=FindFProperty<FObjectPropertyBase>(UTunaSweeperIntroMenuWidget::StaticClass(),TEXT("StartButton"));
            auto* Start=Property ? Cast<UButton>(Property->GetObjectPropertyValue_InContainer(Widgets[0])) : nullptr;
            if (!Check(Start != nullptr,TEXT("Real title continue button exists"))) return true;
            Capture(Test,bTrue ? TEXT("True_Title") : TEXT("Normal_Title"));
            Start->OnClicked.Broadcast(); Next(6); return false;
        }
        if (Stage == 6 && !World->GetName().Contains(TEXT("IntroMap")) && Pawn && FPlatformTime::Seconds()-Changed > 3)
        {
            FTunaSweeperItemInstance Item;
            Check(Game->GetActiveSaveSlotIndex()==1,TEXT("Continue keeps selected slot"));
            Check(Game->TryGetItemInstance(Carried,Item) && Item.Quantity==10,TEXT("Continue preserves carried tuna"));
            Check(Game->IsRaidStorageReadyForTrueEnding()==bTrue,TEXT("Continue reloads persistent storage"));
            Check(!UGameplayStatics::IsGamePaused(World),TEXT("Free play resumes unpaused"));
            Check(UGameplayStatics::GetCurrentLevelName(World,true)==TunaSweeperBuildFlavor::GetBunkerLevelName().ToString(),TEXT("Continue returns to the correct base map"));
            auto* Player=Cast<ATunaSweeperPlayerController>(PC);
            Check(Player && !Player->IsDialogueSequenceActive() && !Player->IsMoveInputIgnored(),TEXT("Ending input lock released after continue"));
            Capture(Test,bTrue ? TEXT("True_Continue") : TEXT("Normal_Continue"));
            if (bTrue) return true;
            bTrue=true; bCapturedStory=false;
            UGameplayStatics::OpenLevel(World,TEXT("/Game/MainRaid/RaidPlains")); Next(5);
        }
        return false;
    }
};
}
DEFINE_LATENT_AUTOMATION_COMMAND_ONE_PARAMETER(FSSTOEndingPlayRun,TSharedPtr<SSTOEndingPlay::FRun>,Run);
bool FSSTOEndingPlayRun::Update() { return Run->Tick(); }
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTunaSweeperSSTOEndingPlayTest,"TunaSweeper.SSTO.EndingPlay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTunaSweeperSSTOEndingPlayTest::RunTest(const FString& Parameters)
{
    if (!FPaths::ProjectSavedDir().Contains(TEXT("SSTOEndingAutomation")))
    { AddError(TEXT("Requires isolated -UserDir=<project>/Saved/SSTOEndingAutomation/; never runs against player saves.")); return false; }
    // Existing title-map sample actor; tracked in demo_build_implementation_audit_2026-09-02.md.
    // Scope the allowance to BP_Morph only; all gameplay/test failures remain fatal.
    AddExpectedError(TEXT("Blueprint Runtime Error:.*Blueprint:  BP_Morph"), EAutomationExpectedErrorFlags::Contains, 0);
    if (!TestTrue(TEXT("Load saved RaidPlains"),FEditorFileUtils::LoadMap(FPaths::ProjectContentDir()/TEXT("MainRaid/RaidPlains.umap"),false,true))) return false;
    FRequestPlaySessionParams Play;
    Play.EditorPlaySettings=NewObject<ULevelEditorPlaySettings>(GetTransientPackage());
    Play.EditorPlaySettings->NewWindowWidth=1280; Play.EditorPlaySettings->NewWindowHeight=800;
    GEditor->RequestPlaySession(Play);
    auto Run=MakeShared<SSTOEndingPlay::FRun>(); Run->Test=this;
    ADD_LATENT_AUTOMATION_COMMAND(FSSTOEndingPlayRun(Run));
    ADD_LATENT_AUTOMATION_COMMAND(FEndPlayMapCommand());
    return true;
}
#endif
