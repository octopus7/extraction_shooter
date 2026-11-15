#if WITH_DEV_AUTOMATION_TESTS
#include "Editor.h"
#include "FileHelpers.h"
#include "Settings/LevelEditorPlaySettings.h"
#include "Tests/AutomationEditorCommon.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWindow.h"
#include "UObject/UnrealType.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "ImageUtils.h"
#include "UnrealClient.h"
#include "Game/TunaSweeperGameInstance.h"
#include "Settings/TunaSweeperBuildFlavor.h"
#include "Player/TunaSweeperPlayerController.h"
#include "Interaction/TunaSweeperLadderTransferActor.h"
#include "Interaction/TunaSweeperRaidStorageActor.h"
#include "Scenario/TunaSweeperEndingActor.h"
#include "UI/TunaSweeperIntroMenuWidget.h"
#include "GameFramework/Character.h"

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
    double Changed = FPlatformTime::Seconds();
    double LastKey = 0;
    FGuid Carried;
    void Next(int32 Value)
    {
        Stage = Value; Changed = FPlatformTime::Seconds();
        Test->AddInfo(FString::Printf(TEXT("SSTO stage=%d true=%d"),Stage,bTrue));
    }
    bool Check(bool Value, const TCHAR* Message) { return Test->TestTrue(Message, Value); }
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
            // Sweep the player's real upright capsule along the central aisle to the interaction position.
            const float Half = Pawn->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
            const float Radius = Pawn->GetCapsuleComponent()->GetScaledCapsuleRadius();
            FCollisionQueryParams Params(SCENE_QUERY_STAT(SSTOEndingWalk), false, Pawn);
            FVector Previous = Pawn->GetActorLocation() + FVector(0,0,8);
            for (int32 X = -300; X <= 365; X += 35)
            {
                const FVector Floor = Ship->GetActorTransform().TransformPosition(FVector(X,0,181));
                const FVector Point = Floor + FVector(0,0,Half+10);
                FHitResult Hit;
                const bool Blocked = World->SweepSingleByChannel(Hit, Previous, Point, FQuat::Identity, ECC_Pawn,
                    FCollisionShape::MakeCapsule(Radius, Half), Params);
                if (Blocked)
                {
                    Test->AddError(FString::Printf(TEXT("Interior aisle blocked by %s at %s"), *GetNameSafe(Hit.GetActor()), *Hit.Location.ToString()));
                    return true;
                }
                Previous = Point;
            }
            Pawn->SetActorLocation(Previous);
            if (!Check(Storage->GetInteractableComponent()->IsWithinInteractionDistance(Pawn), TEXT("Storage reachable from aisle")) ||
                !Check(Ending->IsWithinInteractionDistance(Pawn), TEXT("Ending reachable from aisle")) ||
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
            if (!Check(Ending->RequestInteraction(Pawn), TEXT("Actual ending interaction starts"))) return true;
            Next(3); return false;
        }
        if (Stage == 3)
        {
            if (World->GetName().Contains(TEXT("IntroMap")))
            {
                Check(Game->IsScenarioProgressFlagSet(bTrue ? TEXT("ending.freeplay.true_seen") : TEXT("ending.freeplay.normal_seen")), TEXT("Ending completion persisted"));
                Check(!Game->IsScenarioProgressFlagSet(TEXT("demo.ending.farewell_seen")), TEXT("Slot not retired"));
                Check(Game->GetSaveSlotSummary(1).bHasData,TEXT("Same save slot still exists"));
                Next(4); return false;
            }
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
